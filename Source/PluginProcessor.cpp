#include "PluginProcessor.h"
#include "PluginEditor.h"

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

static const char* kCats[] = { "Bass", "Lead", "Pad", "Pluck", "Keys", "FX", "Arp",
                               "Sequence", "Drums", "Ambient", "Brass_Strings", "Vocal" };
static const int kCounts[] = { 260, 240, 220, 180, 150, 200, 160, 120, 90, 140, 80, 72 };

Layout DiversionProcessor::createLayout()
{
    Layout l;
    const juce::StringArray waves{ "Saw", "Square", "Sine", "Triangle" };
    for (int i = 1; i <= 4; ++i)
    {
        auto n = juce::String(i);
        l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "wave" + n, 1 }, "Osc" + n + " Wave", waves, 0));
        l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "level" + n, 1 }, "Osc" + n + " Level",
              juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), i == 1 ? 0.8f : 0.0f));
        l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "detune" + n, 1 }, "Osc" + n + " Detune",
              juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f), 0.0f));
    }
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "cutoff", 1 }, "Cutoff",
          juce::NormalisableRange<float>(20.0f, 20000.0f, 0.01f, 0.3f), 20000.0f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "res", 1 }, "Resonance",
          juce::NormalisableRange<float>(0.5f, 10.0f, 0.001f, 0.5f), 0.707f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "drive", 1 }, "Drive",
          juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f), 0.0f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "attack", 1 }, "Attack",
          juce::NormalisableRange<float>(0.001f, 5.0f, 0.0001f, 0.4f), 0.005f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "decay", 1 }, "Decay",
          juce::NormalisableRange<float>(0.001f, 5.0f, 0.0001f, 0.4f), 0.2f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "sustain", 1 }, "Sustain",
          juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.8f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "release", 1 }, "Release",
          juce::NormalisableRange<float>(0.001f, 10.0f, 0.0001f, 0.4f), 0.15f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "master", 1 }, "Master",
          juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.7f));
    return l;
}

DiversionProcessor::DiversionProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new DiversionVoice(apvts));
    synth.addSound(new DiversionSound());
}

void DiversionProcessor::prepareToPlay(double sr, int block)
{
    synth.setCurrentPlaybackSampleRate(sr);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<DiversionVoice*>(synth.getVoice(i)))
            v->prepare(sr, block);
}

void DiversionProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    buffer.applyGain(apvts.getRawParameterValue("master")->load());
}

const juce::String DiversionProcessor::getProgramName(int index)
{
    if (index <= 0) return "Default";
    int n = index - 1;
    for (int c = 0; c < 12; ++c)
    {
        if (n < kCounts[c]) return juce::String(kCats[c]) + " " + juce::String(n + 1);
        n -= kCounts[c];
    }
    return "Preset";
}

void DiversionProcessor::setCurrentProgram(int index)
{
    current = juce::jlimit(0, kNumPresets - 1, index);

    if (current == 0)
    {
        for (auto* p : getParameters())
            p->setValueNotifyingHost(p->getDefaultValue());
        return;
    }

    auto set = [this](const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v));
    };

    juce::Random r(1912 + current * 7919);
    for (int i = 1; i <= 4; ++i)
    {
        auto n = juce::String(i);
        set("wave" + n, r.nextFloat());
        float lv = i == 1 ? 0.6f + 0.4f * r.nextFloat() : r.nextFloat();
        set("level" + n, (i > 1 && lv < 0.4f) ? 0.0f : lv * 0.8f);
        set("detune" + n, i == 1 ? 0.5f : 0.5f + (r.nextFloat() - 0.5f) * 0.3f);
    }
    set("cutoff", 0.35f + 0.65f * r.nextFloat());
    set("res", 0.25f * r.nextFloat());
    set("drive", 0.4f * r.nextFloat());
    set("attack", 0.25f * r.nextFloat());
    set("decay", 0.3f + 0.4f * r.nextFloat());
    set("sustain", 0.3f + 0.7f * r.nextFloat());
    set("release", 0.2f + 0.5f * r.nextFloat());
}

void DiversionProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void DiversionProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* DiversionProcessor::createEditor()
{
    return new DiversionEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DiversionProcessor();
}
