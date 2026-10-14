#include "PluginEditor.h"

namespace
{
const juce::Colour bg(0xff050817), panel(0xff0a1229), cyan(0xff20c8ff), violet(0xffd84cff), text(0xffdcecff);
}

TEKITAudioProcessorEditor::TEKITAudioProcessorEditor(TEKITAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1040, 650);
    setResizable(true, true);
    setResizeLimits(900, 560, 1500, 900);

    title.setText("TEK IT", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(34.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, cyan);
    subtitle.setText("METALLIC LEADS   /   SUPERSAW   /   HYBRID SYNTHS", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    subtitle.setColour(juce::Label::textColourId, violet);
    addAndMakeVisible(title);
    addAndMakeVisible(subtitle);

    addAndMakeVisible(presetBox);
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem(processor.getProgramName(i), i + 1);
    presetBox.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setColour(juce::ComboBox::backgroundColourId, panel);
    presetBox.setColour(juce::ComboBox::textColourId, text);
    presetBox.setColour(juce::ComboBox::outlineColourId, cyan.withAlpha(0.8f));
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0) processor.setCurrentProgram(index);
    };

    const std::pair<const char*, const char*> params[] = {
        {"wave", "WAVE"}, {"detune", "DETUNE"}, {"unison", "UNISON"}, {"sub", "SUB LEVEL"},
        {"pitch", "PITCH"}, {"fine", "FINE"}, {"glide", "GLIDE"},
        {"cutoff", "CUTOFF"}, {"res", "RESONANCE"}, {"drive", "DRIVE"},
        {"attack", "ATTACK"}, {"decay", "DECAY"}, {"sustain", "SUSTAIN"}, {"release", "RELEASE"},
        {"eqLow", "EQ LOW"}, {"eqMid", "EQ MID"}, {"eqHigh", "EQ HIGH"},
        {"reverb", "REVERB"}, {"delay", "DELAY"}, {"delayTime", "DELAY TIME"},
        {"width", "STEREO WIDTH"}, {"volume", "MASTER"}
    };
    for (const auto& item : params)
    {
        auto c = std::make_unique<Control>();
        c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
        c->slider.setColour(juce::Slider::rotarySliderFillColourId, cyan);
        c->slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff263a66));
        c->slider.setColour(juce::Slider::thumbColourId, violet);
        c->slider.setColour(juce::Slider::textBoxTextColourId, text);
        c->slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff060b1b));
        c->slider.setColour(juce::Slider::textBoxOutlineColourId, cyan.withAlpha(0.25f));
        c->label.setText(item.second, juce::dontSendNotification);
        c->label.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        c->label.setJustificationType(juce::Justification::centred);
        c->label.setColour(juce::Label::textColourId, text);
        addAndMakeVisible(c->slider);
        addAndMakeVisible(c->label);
        c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, item.first, c->slider);
        controls.push_back(std::move(c));
    }
}

void TEKITAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff111c3c), 0.0f, 0.0f, bg, 0.0f, (float)getHeight(), false));
    g.fillRect(getLocalBounds());
    g.setColour(cyan.withAlpha(0.9f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8.0f), 12.0f, 1.4f);
    g.setColour(violet.withAlpha(0.85f));
    g.drawLine(20.0f, 76.0f, (float)getWidth() - 20.0f, 76.0f, 1.2f);

    const int margin = 18, gap = 10, panelY = 94, panelBottom = getHeight() - 18;
    const int panelW = (getWidth() - 2 * margin - 2 * gap) / 3;
    const juce::String names[] = { "01   OSCILLATORS / VOICE", "02   FILTER / ENVELOPE", "03   EQ / FX / MASTER" };
    for (int i = 0; i < 3; ++i)
    {
        juce::Rectangle<float> r((float)(margin + i * (panelW + gap)), (float)panelY,
                                 (float)panelW, (float)(panelBottom - panelY));
        g.setColour(panel.withAlpha(0.98f));
        g.fillRoundedRectangle(r, 9.0f);
        g.setColour((i == 1 ? violet : cyan).withAlpha(0.85f));
        g.drawRoundedRectangle(r, 9.0f, 1.1f);
        g.setColour(i == 1 ? violet : cyan);
        g.fillRoundedRectangle(r.withHeight(3.0f).reduced(8.0f, 0.0f), 1.5f);
        g.setColour(text);
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(names[i], r.toNearestInt().withHeight(28).translated(9, 7), juce::Justification::centredLeft, false);
    }
}

void TEKITAudioProcessorEditor::resized()
{
    const int w = getWidth(), h = getHeight();
    title.setBounds(24, 10, 210, 48);
    subtitle.setBounds(235, 22, juce::jmax(220, w - 535), 26);
    presetBox.setBounds(w - 270, 20, 240, 34);

    const int margin = 16, gap = 10, panelY = 88, panelBottom = h - 14;
    const int panelW = (w - 2 * margin - 2 * gap) / 3;
    const int panelX[3] = { margin, margin + panelW + gap, margin + 2 * (panelW + gap) };
    const int top = panelY + 38;
    const int bottom = panelBottom - 8;
    const int innerW = panelW - 16;
    const int cols = 3;
    const int cellW = innerW / cols;
    const int cellH = (bottom - top) / 3;

    for (size_t i = 0; i < controls.size(); ++i)
    {
        const int group = i < 7 ? 0 : (i < 14 ? 1 : 2);
        const int local = group == 0 ? (int)i : (group == 1 ? (int)i - 7 : (int)i - 14);
        const int col = local % cols;
        const int row = local / cols;
        const int x = panelX[group] + 8 + col * cellW;
        const int y = top + row * cellH;
        const int cw = cellW - 4;
        auto& c = *controls[i];
        c.label.setBounds(x, y, cw, 20);
        // Reserve a predictable area for the rotary and its value textbox.
        c.slider.setBounds(x + 2, y + 20, cw - 4, juce::jmax(78, cellH - 23));
        c.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, juce::jmin(66, cw - 8), 17);
        c.slider.setVisible(true);
        c.label.setVisible(true);
        c.slider.toFront(false);
        c.label.toFront(false);
    }
}
