#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

struct DiversionSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class DiversionVoice : public juce::SynthesiserVoice
{
public:
    explicit DiversionVoice(juce::AudioProcessorValueTreeState& s)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto n = juce::String(i + 1);
            wave[i] = s.getRawParameterValue("wave" + n);
            level[i] = s.getRawParameterValue("level" + n);
            detune[i] = s.getRawParameterValue("detune" + n);
        }
        cutoff = s.getRawParameterValue("cutoff");
        res = s.getRawParameterValue("res");
        drive = s.getRawParameterValue("drive");
        atk = s.getRawParameterValue("attack");
        dec = s.getRawParameterValue("decay");
        sus = s.getRawParameterValue("sustain");
        rel = s.getRawParameterValue("release");
    }

    void prepare(double sr, int block)
    {
        juce::dsp::ProcessSpec spec{ sr, (juce::uint32) block, 1 };
        filter.prepare(spec);
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<DiversionSound*>(s) != nullptr;
    }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        note = midiNote;
        vel = velocity;
        for (auto& p : phase) p = 0.0;
        adsr.setSampleRate(getSampleRate());
        adsr.setParameters({ atk->load(), dec->load(), sus->load(), rel->load() });
        adsr.noteOn();
        filter.reset();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (!isVoiceActive()) return;

        const double sr = getSampleRate();
        filter.setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, cutoff->load()));
        filter.setResonance(juce::jmax(0.1f, res->load()));
        const float gain = juce::Decibels::decibelsToGain(drive->load());

        for (int i = 0; i < num; ++i)
        {
            float s = 0.0f;
            for (int o = 0; o < 4; ++o)
            {
                const float lv = level[o]->load();
                if (lv <= 0.0f) continue;
                const double semis = (note - 69) + detune[o]->load() / 100.0;
                const double freq = 440.0 * std::pow(2.0, semis / 12.0);
                const float p = (float) phase[o];
                float w = 0.0f;
                switch ((int) wave[o]->load())
                {
                    case 0: w = 2.0f * p - 1.0f; break;
                    case 1: w = p < 0.5f ? 1.0f : -1.0f; break;
                    case 2: w = std::sin(juce::MathConstants<float>::twoPi * p); break;
                    default: w = 4.0f * std::abs(p - 0.5f) - 1.0f; break;
                }
                s += w * lv;
                phase[o] += freq / sr;
                if (phase[o] >= 1.0) phase[o] -= 1.0;
            }
            s *= vel * adsr.getNextSample() * 0.3f;
            s = std::tanh(s * gain);
            s = filter.processSample(0, s);
            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.addSample(ch, start + i, s);
        }

        if (!adsr.isActive())
            clearCurrentNote();
    }

private:
    std::atomic<float> *wave[4], *level[4], *detune[4];
    std::atomic<float> *cutoff, *res, *drive, *atk, *dec, *sus, *rel;
    double phase[4] = { 0, 0, 0, 0 };
    int note = 60;
    float vel = 1.0f;
    juce::ADSR adsr;
    juce::dsp::StateVariableTPTFilter<float> filter;
};

class DiversionProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kNumPresets = 1912;

    DiversionProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DIVERSION Synthesizer"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return kNumPresets; }
    int getCurrentProgram() override { return current; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::Synthesiser synth;
    int current = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiversionProcessor)
};
