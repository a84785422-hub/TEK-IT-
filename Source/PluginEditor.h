#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class TEKITAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TEKITAudioProcessorEditor(TEKITAudioProcessor& processorRef);
    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    TEKITAudioProcessor& processor;
    juce::ComboBox presetBox;
    juce::Label title, subtitle;

    struct Knob final : public juce::Component
    {
        juce::Slider slider;
        juce::Label label;
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

        Knob(TEKITAudioProcessor& processorRef,
             const juce::String& parameterID,
             const juce::String& displayName);
        void resized() override;
    };

    std::vector<std::unique_ptr<Knob>> knobs;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TEKITAudioProcessorEditor)
};
