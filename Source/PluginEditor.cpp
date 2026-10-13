#include "PluginEditor.h"

TEKITAudioProcessorEditor::Knob::Knob(TEKITAudioProcessor& processorRef,
                                     const juce::String& parameterID,
                                     const juce::String& displayName)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff13bfff));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffd84cff));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff071126));

    label.setText(displayName, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa8dfff));

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
    attachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.parameters, parameterID, slider));
}

void TEKITAudioProcessorEditor::Knob::resized()
{
    label.setBounds(0, 0, getWidth(), 20);
    slider.setBounds(0, 18, getWidth(), getHeight() - 18);
}

TEKITAudioProcessorEditor::TEKITAudioProcessorEditor(TEKITAudioProcessor& processorRef)
    : AudioProcessorEditor(&processorRef), processor(processorRef)
{
    setSize(1040, 650);

    title.setText("TEK IT", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(34.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, juce::Colour(0xff37c9ff));

    subtitle.setText("METALLIC LEADS  /  SUPERSAW  /  HYBRID SYNTHS",
                     juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colour(0xffdf67ff));

    addAndMakeVisible(title);
    addAndMakeVisible(subtitle);
    addAndMakeVisible(presetBox);

    for (int index = 0; index < processor.getNumPrograms(); ++index)
        presetBox.addItem(processor.getProgramName(index), index + 1);

    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff071126));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    presetBox.onChange = [this]
    {
        processor.setCurrentProgram(presetBox.getSelectedId() - 1);
    };

    const std::pair<const char*, const char*> parameterList[] = {
        {"wave", "WAVE"}, {"detune", "DETUNE"}, {"unison", "UNISON"}, {"sub", "SUB"},
        {"cutoff", "CUTOFF"}, {"res", "RESONANCE"}, {"drive", "DRIVE"},
        {"attack", "ATTACK"}, {"decay", "DECAY"}, {"sustain", "SUSTAIN"},
        {"release", "RELEASE"}, {"pitch", "PITCH"}, {"fine", "FINE"},
        {"eqLow", "EQ LOW"}, {"eqMid", "EQ MID"}, {"eqHigh", "EQ HIGH"},
        {"reverb", "REVERB"}, {"delay", "DELAY"}, {"delayTime", "DELAY TIME"},
        {"width", "WIDTH"}, {"volume", "MASTER"}
    };

    for (const auto& parameter : parameterList)
    {
        auto knob = std::make_unique<Knob>(processor, parameter.first, parameter.second);
        addAndMakeVisible(*knob);
        knobs.push_back(std::move(knob));
    }
}

void TEKITAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff030817));
    graphics.setColour(juce::Colour(0xff087dff));
    graphics.drawRoundedRectangle(
        juce::Rectangle<float>(8.0f, 8.0f,
                              static_cast<float>(getWidth() - 16),
                              static_cast<float>(getHeight() - 16)),
        10.0f, 1.5f);
    graphics.setColour(juce::Colour(0xffc13dff));
    graphics.drawLine(20.0f, 76.0f, static_cast<float>(getWidth() - 20),
                      76.0f, 1.0f);
}

void TEKITAudioProcessorEditor::resized()
{
    title.setBounds(24, 12, 250, 50);
    subtitle.setBounds(285, 25, 480, 25);
    presetBox.setBounds(780, 24, 230, 30);

    const int left = 22;
    const int top = 95;
    const int gap = 10;
    const int knobWidth = 90;
    const int knobHeight = 110;
    const int columns = 10;

    for (int index = 0; index < static_cast<int>(knobs.size()); ++index)
    {
        const int row = index / columns;
        const int column = index % columns;
        knobs[static_cast<size_t>(index)]->setBounds(
            left + column * (knobWidth + gap),
            top + row * (knobHeight + 12),
            knobWidth, knobHeight);
    }
}
