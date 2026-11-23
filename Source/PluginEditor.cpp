#include "PluginEditor.h"

DiversionEditor::DiversionEditor(DiversionProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setSize(736, 523);

    auto& lf = getLookAndFeel();
    lf.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white);
    lf.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff444444));
    lf.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    lf.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    lf.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff555555));
    lf.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222222));
    lf.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    lf.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff777777));
    lf.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));

    for (int i = 0; i < 4; ++i)
    {
        auto n = juce::String(i + 1);
        int x = 20 + i * 175;
        auto w = std::make_unique<WaveBox>();
        w->box.addItemList({ "Saw", "Square", "Sine", "Triangle" }, 1);
        w->box.setBounds(x, 70, 150, 24);
        addAndMakeVisible(w->box);
        w->att = std::make_unique<CA>(proc.apvts, "wave" + n, w->box);
        waves.push_back(std::move(w));
        addKnob("level" + n, "Level " + n, x, 105);
        addKnob("detune" + n, "Detune " + n, x + 80, 105);
    }

    addKnob("cutoff", "Cutoff", 20, 265);
    addKnob("res", "Resonance", 100, 265);
    addKnob("drive", "Drive", 180, 265);
    addKnob("attack", "Attack", 320, 265);
    addKnob("decay", "Decay", 400, 265);
    addKnob("sustain", "Sustain", 480, 265);
    addKnob("release", "Release", 560, 265);
    addKnob("master", "Master", 650, 265);

    addAndMakeVisible(prev);
    addAndMakeVisible(next);
    addAndMakeVisible(presetLabel);
    prev.setBounds(20, 440, 34, 30);
    presetLabel.setBounds(60, 440, 300, 30);
    next.setBounds(366, 440, 34, 30);
    presetLabel.setJustificationType(juce::Justification::centred);
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    presetLabel.setColour(juce::Label::backgroundColourId, juce::Colours::white);

    prev.onClick = [this] {
        proc.setCurrentProgram((proc.getCurrentProgram() + DiversionProcessor::kNumPresets - 1) % DiversionProcessor::kNumPresets);
        showPreset();
    };
    next.onClick = [this] {
        proc.setCurrentProgram((proc.getCurrentProgram() + 1) % DiversionProcessor::kNumPresets);
        showPreset();
    };
    showPreset();
}

void DiversionEditor::addKnob(const juce::String& id, const juce::String& text, int x, int y)
{
    auto k = std::make_unique<Knob>();
    k->label.setText(text, juce::dontSendNotification);
    k->label.setJustificationType(juce::Justification::centred);
    k->label.setFont(juce::FontOptions(12.0f));
    k->label.setColour(juce::Label::textColourId, juce::Colours::white);
    k->label.setBounds(x, y, 72, 16);
    k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    k->slider.setMouseDragSensitivity(60);   // very sensitive: 60 px for the full range
    k->slider.setVelocityBasedMode(false);
    k->slider.setBounds(x, y + 16, 72, 84);
    addAndMakeVisible(k->label);
    addAndMakeVisible(k->slider);
    k->att = std::make_unique<SA>(proc.apvts, id, k->slider);
    knobs.push_back(std::move(k));
}

void DiversionEditor::showPreset()
{
    int i = proc.getCurrentProgram();
    presetLabel.setText(juce::String(i + 1) + " / " + juce::String(DiversionProcessor::kNumPresets)
                        + "   " + proc.getProgramName(i), juce::dontSendNotification);
}

void DiversionEditor::paint(juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3a3a), 0, 0, juce::Colour(0xff0c0c0c), 0, (float) getHeight(), false));
    g.fillAll();
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("DIVERSION SYNTHESIZER", 20, 10, 400, 30, juce::Justification::left);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Oscillators", 20, 46, 200, 16, juce::Justification::left);
    g.drawText("Filter", 20, 240, 200, 16, juce::Justification::left);
    g.drawText("Envelope", 320, 240, 200, 16, juce::Justification::left);
    g.setColour(juce::Colour(0xff777777));
    g.drawRect(10, 40, 716, 190, 1);
    g.drawRect(10, 235, 716, 180, 1);
    g.drawRect(10, 425, 716, 90, 1);
}
