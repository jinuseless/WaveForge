#include "PluginProcessor.h"
#include "PluginEditor.h"

using Param = juce::AudioParameterFloat;
using Range = juce::NormalisableRange<float>;

juce::AudioProcessorValueTreeState::ParameterLayout WaveForgeProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<Param>("wtpos", "WT Position", Range(0.f, 1.f, 0.001f), 0.f));
    p.push_back(std::make_unique<Param>("lforate", "LFO Rate", Range(0.05f, 20.f, 0.01f, 0.4f), 2.f));
    p.push_back(std::make_unique<Param>("lfodepth", "LFO Depth", Range(0.f, 1.f, 0.001f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterInt>("uni", "Unison", 1, 7, 1));
    p.push_back(std::make_unique<Param>("detune", "Detune", Range(0.f, 50.f, 0.1f), 12.f));
    p.push_back(std::make_unique<Param>("spread", "Spread", Range(0.f, 1.f, 0.001f), 0.6f));
    p.push_back(std::make_unique<Param>("cutoff", "Cutoff", Range(20.f, 20000.f, 1.f, 0.3f), 8000.f));
    p.push_back(std::make_unique<Param>("res", "Resonance", Range(0.5f, 8.f, 0.01f, 0.5f), 0.8f));
    p.push_back(std::make_unique<Param>("envamt", "Filter Env", Range(0.f, 1.f, 0.001f), 0.f));
    p.push_back(std::make_unique<Param>("attack", "Attack", Range(0.001f, 5.f, 0.001f, 0.4f), 0.005f));
    p.push_back(std::make_unique<Param>("decay", "Decay", Range(0.001f, 5.f, 0.001f, 0.4f), 0.3f));
    p.push_back(std::make_unique<Param>("sustain", "Sustain", Range(0.f, 1.f, 0.001f), 0.7f));
    p.push_back(std::make_unique<Param>("release", "Release", Range(0.001f, 5.f, 0.001f, 0.4f), 0.4f));
    p.push_back(std::make_unique<Param>("gain", "Gain", Range(0.f, 1.f, 0.001f), 0.7f));

    return { p.begin(), p.end() };
}

WaveForgeProcessor::WaveForgeProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    synth.addSound(new WTSound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new WTVoice(table, apvts));
}

void WaveForgeProcessor::prepareToPlay(double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
}

void WaveForgeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* WaveForgeProcessor::createEditor()
{
    return new WaveForgeEditor(*this);
}

void WaveForgeProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void WaveForgeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveForgeProcessor();
}
