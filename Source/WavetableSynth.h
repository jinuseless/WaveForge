#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

// ---------------------------------------------------------------------------
// Wavetable: 16 frames (soft sine-ish -> bright saw, with odd/even harmonic
// morphing). 6 mip levels so high notes don't alias.
// ---------------------------------------------------------------------------
struct WaveTable
{
    static constexpr int numFrames = 16;
    static constexpr int size = 2048;
    static constexpr int numLevels = 6;
    static constexpr int maxHarmonics = 128;

    // [level][frame][sample] (+1 guard sample for interpolation)
    std::array<std::array<std::array<float, size + 1>, numFrames>, numLevels> data{};

    WaveTable() { build(); }

    void build()
    {
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int level = 0; level < numLevels; ++level)
        {
            const int maxH = maxHarmonics >> level;

            for (int f = 0; f < numFrames; ++f)
            {
                const float t = (float) f / (float) (numFrames - 1);
                const float tilt = 3.0f - 2.0f * t;                       // 3 (soft) -> 1 (saw)
                const float evenGain = 0.5f + 0.5f * std::cos(t * twoPi * 2.0f); // odd/even morph

                std::array<float, maxHarmonics + 1> amp{};
                for (int h = 1; h <= maxH; ++h)
                {
                    float a = 1.0f / std::pow((float) h, tilt);
                    if (h % 2 == 0) a *= evenGain;
                    amp[(size_t) h] = a;
                }

                float peak = 0.0f;
                for (int i = 0; i < size; ++i)
                {
                    float s = 0.0f;
                    const float ph = twoPi * (float) i / (float) size;
                    for (int h = 1; h <= maxH; ++h)
                        s += amp[(size_t) h] * std::sin(ph * (float) h);
                    data[(size_t) level][(size_t) f][(size_t) i] = s;
                    peak = std::max(peak, std::abs(s));
                }

                const float norm = peak > 0.0f ? 1.0f / peak : 1.0f;
                for (int i = 0; i < size; ++i)
                    data[(size_t) level][(size_t) f][(size_t) i] *= norm;
                data[(size_t) level][(size_t) f][size] = data[(size_t) level][(size_t) f][0];
            }
        }
    }
};

struct WTSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// ---------------------------------------------------------------------------
// Voice: wavetable osc with unison, lowpass filter, amp + filter envelopes, LFO
// ---------------------------------------------------------------------------
class WTVoice : public juce::SynthesiserVoice
{
public:
    WTVoice(const WaveTable& t, juce::AudioProcessorValueTreeState& a) : table(t), apvts(a) {}

    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<WTSound*>(s) != nullptr; }

    void setCurrentPlaybackSampleRate(double sr) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate(sr);
        if (sr <= 0.0) return;
        env.setSampleRate(sr);
        fenv.setSampleRate(sr);
        juce::dsp::ProcessSpec spec{ sr, 512, 1 };
        for (auto& f : filter)
        {
            f.prepare(spec);
            f.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        }
    }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override
    {
        baseFreq = juce::MidiMessage::getMidiNoteInHertz(midiNote);
        vel = velocity;
        bend = std::pow(2.0, ((pitchWheel - 8192) / 8192.0) * 2.0 / 12.0);

        juce::ADSR::Parameters ap{ get("attack"), get("decay"), get("sustain"), get("release") };
        env.setParameters(ap);
        fenv.setParameters(ap);
        env.noteOn();
        fenv.noteOn();

        for (auto& f : filter) f.reset();
        for (auto& p : phase) p = rng.nextFloat();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            env.noteOff();
            fenv.noteOff();
        }
        else
        {
            env.reset();
            fenv.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int v) override { bend = std::pow(2.0, ((v - 8192) / 8192.0) * 2.0 / 12.0); }
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (!env.isActive()) return;

        const double sr = getSampleRate();
        const int nu = juce::jlimit(1, maxUnison, (int) std::round(get("uni")));
        const float detune = get("detune");
        const float spread = get("spread");
        const float wtPos = get("wtpos");
        const float lfoRate = get("lforate");
        const float lfoDepth = get("lfodepth");
        const float cutoff = get("cutoff");
        const float res = get("res");
        const float envAmt = get("envamt");
        const float gain = get("gain");

        // Per-unison frequency, pan gains
        float gl[maxUnison], gr[maxUnison];
        double inc[maxUnison];
        double topFreq = baseFreq;
        for (int u = 0; u < nu; ++u)
        {
            const float off = nu == 1 ? 0.0f : (2.0f * (float) u / (float) (nu - 1)) - 1.0f; // -1..1
            const double f = baseFreq * bend * std::pow(2.0, (double) (detune * off) / 1200.0);
            inc[u] = f / sr;
            topFreq = std::max(topFreq, f);
            const float pan = off * spread;
            gl[u] = std::sqrt(0.5f * (1.0f - pan));
            gr[u] = std::sqrt(0.5f * (1.0f + pan));
        }

        int level = WaveTable::numLevels - 1;
        for (int l = 0; l < WaveTable::numLevels; ++l)
            if ((double) (WaveTable::maxHarmonics >> l) * topFreq < sr * 0.5) { level = l; break; }

        for (auto& f : filter) f.setResonance(res);

        auto* L = out.getWritePointer(0, start);
        auto* R = out.getNumChannels() > 1 ? out.getWritePointer(1, start) : nullptr;
        const float norm = 0.5f / std::sqrt((float) nu);

        for (int n = 0; n < num; ++n)
        {
            const float amp = env.getNextSample();
            const float fe = fenv.getNextSample();
            if (!env.isActive()) { clearCurrentNote(); break; }

            lfoPhase += lfoRate / (float) sr;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            const float pos = juce::jlimit(0.0f, 1.0f,
                wtPos + lfoDepth * std::sin(lfoPhase * juce::MathConstants<float>::twoPi));

            const float ff = pos * (float) (WaveTable::numFrames - 1);
            const int f0 = (int) ff;
            const int f1 = std::min(f0 + 1, WaveTable::numFrames - 1);
            const float fr = ff - (float) f0;

            float l = 0.0f, r = 0.0f;
            for (int u = 0; u < nu; ++u)
            {
                const float x = (float) phase[u] * (float) WaveTable::size;
                const int i = (int) x;
                const float k = x - (float) i;
                const auto& w0 = table.data[(size_t) level][(size_t) f0];
                const auto& w1 = table.data[(size_t) level][(size_t) f1];
                const float s0 = w0[(size_t) i] + k * (w0[(size_t) i + 1] - w0[(size_t) i]);
                const float s1 = w1[(size_t) i] + k * (w1[(size_t) i + 1] - w1[(size_t) i]);
                const float s = s0 + fr * (s1 - s0);
                l += s * gl[u];
                r += s * gr[u];
                phase[u] += inc[u];
                if (phase[u] >= 1.0) phase[u] -= 1.0;
            }

            if ((n & 7) == 0)
            {
                const float hz = juce::jlimit(20.0f, (float) (sr * 0.45),
                    cutoff * std::pow(2.0f, envAmt * 5.0f * fe));
                for (auto& f : filter) f.setCutoffFrequency(hz);
            }

            l = filter[0].processSample(0, l * norm);
            r = filter[1].processSample(0, r * norm);

            const float g = amp * vel * gain;
            L[n] += l * g;
            if (R) R[n] += r * g;
        }
    }

private:
    static constexpr int maxUnison = 7;

    float get(const char* id) const { return apvts.getRawParameterValue(id)->load(); }

    const WaveTable& table;
    juce::AudioProcessorValueTreeState& apvts;
    juce::ADSR env, fenv;
    juce::dsp::StateVariableTPTFilter<float> filter[2];
    juce::Random rng;
    double phase[maxUnison] = {};
    double baseFreq = 440.0, bend = 1.0;
    float vel = 1.0f, lfoPhase = 0.0f;
};
