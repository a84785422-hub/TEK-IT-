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
        // OSCILLATORS / PITCH
        {"wave", "WAVE"}, {"detune", "DETUNE"}, {"unison", "UNISON"},
        {"sub", "SUB"}, {"pitch", "PITCH"}, {"fine", "FINE"}, {"glide", "GLIDE"},
        // FILTER / ENVELOPE
        {"cutoff", "CUTOFF"}, {"res", "RESONANCE"}, {"drive", "DRIVE"},
        {"attack", "ATTACK"}, {"decay", "DECAY"}, {"sustain", "SUSTAIN"},
        {"release", "RELEASE"},
        // EQ / EFFECTS / OUTPUT
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
    graphics.fillAll(juce::Colour(0xff05091a));

    // Subtle midnight-blue gradient and restrained neon frame.
    graphics.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff0b1a3a), 0.0f, 0.0f,
        juce::Colour(0xff10091f), (float)getWidth(), (float)getHeight(), false));
    graphics.fillRect(getLocalBounds());

    // Panel layout mirrors a hardware synth rather than a generic knob grid.
    const juce::Colour panel(0xff0a1228);
    const juce::Colour panelEdge(0xff174b91);
    const juce::Colour violet(0xffa83cff);
    auto drawPanel = [&graphics, &panel, &panelEdge, &violet](juce::Rectangle<float> r,
                                                               const juce::String& caption)
    {
        graphics.setColour(panel);
        graphics.fillRoundedRectangle(r, 8.0f);
        graphics.setColour(panelEdge);
        graphics.drawRoundedRectangle(r, 8.0f, 1.0f);
        graphics.setColour(violet);
        graphics.fillRect(r.getX() + 12.0f, r.getY() + 30.0f, r.getWidth() - 24.0f, 1.0f);
        graphics.setColour(juce::Colour(0xff8edfff));
        graphics.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        graphics.drawText(caption, r.withHeight(26.0f).toNearestInt(),
                          juce::Justification::centredLeft, false);
    };

    graphics.setColour(juce::Colour(0xff087dff));
    graphics.drawRoundedRectangle(8.0f, 8.0f, (float)getWidth() - 16.0f,
                                  (float)getHeight() - 16.0f, 10.0f, 1.5f);
    graphics.setColour(juce::Colour(0xffc13dff));
    graphics.drawLine(20.0f, 76.0f, (float)getWidth() - 20.0f, 76.0f, 1.2f);

    const float panelY = 88.0f;
    const float panelH = (float)getHeight() - panelY - 18.0f;
    const float gap = 10.0f;
    const float panelW = ((float)getWidth() - 44.0f - 2.0f * gap) / 3.0f;
    drawPanel({ 16.0f, panelY, panelW, panelH }, "01  OSCILLATORS");
    drawPanel({ 16.0f + panelW + gap, panelY, panelW, panelH }, "02  FILTER / ENVELOPE");
    drawPanel({ 16.0f + 2.0f * (panelW + gap), panelY, panelW, panelH }, "03  FX / MASTER");

    graphics.setColour(juce::Colour(0xff9c4dff));
    graphics.setFont(juce::Font(juce::FontOptions(9.0f)));
    graphics.drawText("TEK IT  •  METALLIC LEAD SYNTH", 22, getHeight() - 17,
                      getWidth() - 44, 12, juce::Justification::centredRight, false);
}

void TEKITAudioProcessorEditor::resized()
{
    title.setBounds(24, 12, 250, 50);
    subtitle.setBounds(285, 25, 480, 25);
    presetBox.setBounds(780, 24, 230, 30);

    // Three intentional groups: oscillator, tone/envelope, and effects/master.
    const int top = 132;
    const int knobWidth = 86;
    const int knobHeight = 112;
    const int x0 = 26;
    const int x1 = 359;
    const int x2 = 692;
    const int columnGap = 96;
    const int rowGap = 124;

    for (int index = 0; index < static_cast<int>(knobs.size()); ++index)
    {
        int group = 0;
        int withinGroup = index;
        if (index >= 7 && index < 14) { group = 1; withinGroup = index - 7; }
        else if (index >= 14) { group = 2; withinGroup = index - 14; }

        const int row = withinGroup / 3;
        const int column = withinGroup % 3;
        const int groupX = group == 0 ? x0 : (group == 1 ? x1 : x2);
        knobs[static_cast<size_t>(index)]->setBounds(
            groupX + column * columnGap, top + row * rowGap, knobWidth, knobHeight);
    }
}
