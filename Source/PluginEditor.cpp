#include "PluginEditor.h"

WaveForgeEditor::WaveForgeEditor(WaveForgeProcessor& p)
    : AudioProcessorEditor(&p), proc(p), waveView(p)
{
    addAndMakeVisible(waveView);

    // Row 1: oscillator / unison / LFO
    addKnob("wtpos", "WT Pos");
    addKnob("uni", "Unison");
    addKnob("detune", "Detune");
    addKnob("spread", "Spread");
    addKnob("lforate", "LFO Rate");
    addKnob("lfodepth", "LFO Depth");
    addKnob("gain", "Gain");
    // Row 2: filter / envelope
    addKnob("cutoff", "Cutoff");
    addKnob("res", "Reso");
    addKnob("envamt", "Filt Env");
    addKnob("attack", "Attack");
    addKnob("decay", "Decay");
    addKnob("sustain", "Sustain");
    addKnob("release", "Release");

    setSize(760, 470);
}

void WaveForgeEditor::addKnob(const juce::String& paramId, const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    k->slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4cc9f0));
    k->slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1c2230));
    k->slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    k->slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    k->label.setText(text, juce::dontSendNotification);
    k->label.setJustificationType(juce::Justification::centred);
    k->label.setColour(juce::Label::textColourId, juce::Colour(0xff9aa4b8));
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, paramId, k->slider);
    addAndMakeVisible(k->slider);
    addAndMakeVisible(k->label);
    knobs.push_back(std::move(k));
}

void WaveForgeEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff141821));
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("WAVEFORGE", 16, 8, 300, 30, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9aa4b8));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Wavetable Synth", 16, 32, 300, 16, juce::Justification::centredLeft);
}

void WaveForgeEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(40);
    waveView.setBounds(area.removeFromTop(150));
    area.removeFromTop(12);

    const int perRow = 7;
    const int rowH = area.getHeight() / 2;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = (int) i / perRow;
        const int col = (int) i % perRow;
        const int cw = area.getWidth() / perRow;
        juce::Rectangle<int> cell(area.getX() + col * cw, area.getY() + row * rowH, cw, rowH);
        knobs[i]->label.setBounds(cell.removeFromTop(18));
        knobs[i]->slider.setBounds(cell.reduced(4));
    }
}
