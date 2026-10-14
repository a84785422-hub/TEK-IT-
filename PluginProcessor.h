#pragma once
#include <JuceHeader.h>

class TEKITAudioProcessor final : public juce::AudioProcessor
{
public:
    TEKITAudioProcessor();
    ~TEKITAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TEK IT"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 30; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    void loadPreset(int index);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct Voice
    {
        juce::dsp::Oscillator<float> oscA, oscB, oscC;
        juce::ADSR ampEnv, filterEnv;
        juce::ADSR::Parameters envParams { 0.005f, 0.12f, 0.75f, 0.18f };
        juce::dsp::StateVariableTPTFilter<float> filter;
        juce::dsp::Gain<float> gain;
        juce::dsp::Reverb reverb;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delay { 192000 };
        juce::dsp::ProcessSpec spec {};
        int note = -1;
        float velocity = 0.0f, phase = 0.0f, age = 0.0f;
        bool active = false, releasing = false;
        void prepare(double sr, int block, int channels);
        void start(int midiNote, float vel, const juce::AudioProcessorValueTreeState& p);
        void stop();
        float render(const juce::AudioProcessorValueTreeState& p, double sr);
    };
    static constexpr int voiceCount = 16;
    std::array<Voice, voiceCount> voices;
    double currentSampleRate = 44100.0;
    int currentProgram = 0;
    juce::Reverb masterReverb;
    std::vector<float> delayLeft, delayRight;
    size_t delayWrite = 0;
    float eqLowStateL = 0.0f, eqLowStateR = 0.0f;
    float eqMidStateL = 0.0f, eqMidStateR = 0.0f;
    float widthState = 0.0f;
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TEKITAudioProcessor)
};
