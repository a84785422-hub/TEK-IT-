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
    struct Control
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<Control>> controls;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TEKITAudioProcessorEditor)
};
