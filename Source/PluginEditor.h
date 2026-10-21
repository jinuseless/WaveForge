#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Live view of the current wavetable frame
class WaveView : public juce::Component, private juce::Timer
{
public:
    explicit WaveView(WaveForgeProcessor& p) : proc(p) { startTimerHz(30); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff0b0e14));
        g.fillRoundedRectangle(b, 8.0f);
        g.setColour(juce::Colour(0xff1c2230));
        g.drawLine(b.getX(), b.getCentreY(), b.getRight(), b.getCentreY());

        const float pos = proc.apvts.getRawParameterValue("wtpos")->load();
        const float ff = pos * (float) (WaveTable::numFrames - 1);
        const int f0 = (int) ff;
        const int f1 = std::min(f0 + 1, WaveTable::numFrames - 1);
        const float fr = ff - (float) f0;

        juce::Path path;
        auto area = b.reduced(8.0f);
        const int w = (int) area.getWidth();
        for (int x = 0; x < w; ++x)
        {
            const int i = juce::jlimit(0, WaveTable::size - 1, (int) ((float) x / (float) w * WaveTable::size));
            const auto& a = proc.table.data[2][(size_t) f0];
            const auto& c = proc.table.data[2][(size_t) f1];
            const float s = a[(size_t) i] + fr * (c[(size_t) i] - a[(size_t) i]);
            const float y = area.getCentreY() - s * area.getHeight() * 0.45f;
            if (x == 0) path.startNewSubPath(area.getX(), y);
            else path.lineTo(area.getX() + (float) x, y);
        }
        g.setColour(juce::Colour(0xff4cc9f0));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }

private:
    void timerCallback() override { repaint(); }
    WaveForgeProcessor& proc;
};

class WaveForgeEditor : public juce::AudioProcessorEditor
{
public:
    explicit WaveForgeEditor(WaveForgeProcessor&);
    ~WaveForgeEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider{ juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob(const juce::String& paramId, const juce::String& text);

    WaveForgeProcessor& proc;
    WaveView waveView;
    std::vector<std::unique_ptr<Knob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveForgeEditor)
};
