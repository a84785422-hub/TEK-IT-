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
    const auto bg = juce::Colour(0xff050a1b);
    const auto panel = juce::Colour(0xff0a1430);
    const auto cyan = juce::Colour(0xff17c8ff);
    const auto violet = juce::Colour(0xffd34cff);
    graphics.fillAll(bg);
    graphics.setGradientFill(juce::ColourGradient(juce::Colour(0xff101b42), 0.0f, 0.0f,
        bg, 0.0f, (float)getHeight(), false));
    graphics.fillRect(getLocalBounds());
    graphics.setColour(cyan.withAlpha(0.9f));
    graphics.drawRoundedRectangle(juce::Rectangle<float>(8.0f, 8.0f,
        (float)getWidth() - 16.0f, (float)getHeight() - 16.0f), 12.0f, 1.4f);
    graphics.setColour(violet.withAlpha(0.8f));
    graphics.drawLine(20.0f, 76.0f, (float)getWidth() - 20.0f, 76.0f, 1.0f);

    const int panelY = 92, panelH = getHeight() - panelY - 18;
    const int panelW = (getWidth() - 52) / 3;
    const juce::String sectionNames[] = { "01  OSCILLATORS", "02  FILTER / ENVELOPE", "03  EQ / EFFECTS / OUTPUT" };
    for (int i = 0; i < 3; ++i)
    {
        auto r = juce::Rectangle<float>((float)(18 + i * (panelW + 8)), (float)panelY,
                                        (float)panelW, (float)panelH);
        graphics.setColour(panel.withAlpha(0.94f));
        graphics.fillRoundedRectangle(r, 9.0f);
        graphics.setColour((i == 1 ? violet : cyan).withAlpha(0.75f));
        graphics.drawRoundedRectangle(r, 9.0f, 1.0f);
        graphics.setColour(juce::Colour(0xffd8eaff));
        graphics.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        graphics.drawText(sectionNames[i], r.toNearestInt().withHeight(24).translated(8, 6),
                          juce::Justification::centredLeft, false);
    }
}

void TEKITAudioProcessorEditor::resized()
{
    title.setBounds(24, 12, 250, 50);
    subtitle.setBounds(285, 25, 480, 25);
    presetBox.setBounds(780, 24, 230, 30);

    const int panelW = (getWidth() - 52) / 3;
    const int panelX[] = { 28, 28 + panelW + 8, 28 + 2 * (panelW + 8) };
    const int top = 128;
    const int knobW = 92;
    const int knobH = 142;
    const int xGap = 7;
    const int yGap = 3;
    for (int index = 0; index < (int)knobs.size(); ++index)
    {
        const int group = index < 7 ? 0 : (index < 14 ? 1 : 2);
        const int local = index - group * 7;
        const int row = local / 3;
        const int col = local % 3;
        const int x = panelX[group] + col * (knobW + xGap);
        const int y = top + row * (knobH + yGap);
        knobs[(size_t)index]->setBounds(x, y, knobW, knobH);
    }
}
