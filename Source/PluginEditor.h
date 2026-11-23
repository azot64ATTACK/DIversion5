#pragma once
#include "PluginProcessor.h"

class DiversionEditor : public juce::AudioProcessorEditor
{
public:
    explicit DiversionEditor(DiversionProcessor&);
    void paint(juce::Graphics&) override;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SA> att;
    };
    struct WaveBox
    {
        juce::ComboBox box;
        std::unique_ptr<CA> att;
    };

    void addKnob(const juce::String& id, const juce::String& text, int x, int y);
    void showPreset();

    DiversionProcessor& proc;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<WaveBox>> waves;
    juce::TextButton prev{ "<" }, next{ ">" };
    juce::Label presetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiversionEditor)
};
