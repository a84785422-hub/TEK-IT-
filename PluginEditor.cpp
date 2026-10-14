#include "PluginEditor.h"

namespace
{
const juce::Colour bg(0xff050817), panel(0xff0a1229), panelAlt(0xff0d1733), cyan(0xff20c8ff), violet(0xffd84cff), text(0xffe5f2ff), muted(0xff8ca7cc);
struct ParamDef { const char* id; const char* label; };
constexpr ParamDef defs[] = {
 {"wave","WAVEFORM"},{"unison","UNISON VOICES"},{"detune","DETUNE"},{"sub","SUB LEVEL"},{"osc2mix","OSC B MIX"},{"octave2","OSC B OCTAVE"},{"fm","FM AMOUNT"},{"noise","NOISE"},{"pitch","PITCH SEMI"},
 {"cutoff","FILTER CUTOFF"},{"res","RESONANCE"},{"drive","DRIVE"},{"attack","ATTACK"},{"decay","DECAY"},{"sustain","SUSTAIN"},{"release","RELEASE"},{"glide","GLIDE"},{"fine","FINE TUNE"},
 {"eqLow","EQ LOW"},{"eqMid","EQ MID"},{"eqHigh","EQ HIGH"},{"reverb","REVERB MIX"},{"delay","DELAY MIX"},{"delayTime","DELAY TIME"},{"width","STEREO WIDTH"},{"volume","MASTER LEVEL"}
};
}

TEKITAudioProcessorEditor::TEKITAudioProcessorEditor(TEKITAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1120, 760);
    setResizable(true, true);
    setResizeLimits(1000, 680, 1600, 1000);
    startTimerHz(8);

    title.setText("TEK IT", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(38.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, cyan);
    subtitle.setText("METALLIC LEADS  /  SUPERSAW  /  HYBRID SYNTHS", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    subtitle.setColour(juce::Label::textColourId, violet);
    addAndMakeVisible(title); addAndMakeVisible(subtitle);

    presetBox.setTextWhenNothingSelected("SELECT PATCH");
    for (int i = 0; i < processor.getNumPrograms(); ++i) presetBox.addItem(processor.getProgramName(i), i + 1);
    presetBox.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setColour(juce::ComboBox::backgroundColourId, panelAlt);
    presetBox.setColour(juce::ComboBox::textColourId, text);
    presetBox.setColour(juce::ComboBox::outlineColourId, cyan);
    presetBox.setColour(juce::ComboBox::arrowColourId, violet);
    presetBox.onChange = [this] { const int n = presetBox.getSelectedId() - 1; if (n >= 0) processor.setCurrentProgram(n); };
    addAndMakeVisible(presetBox);

    fxToggle.setColour(juce::ToggleButton::textColourId, text);
    fxToggle.setColour(juce::ToggleButton::tickColourId, cyan);
    fxToggle.setColour(juce::ToggleButton::tickDisabledColourId, muted);
    addAndMakeVisible(fxToggle);
    fxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, "fxOn", fxToggle);

    for (const auto& def : defs)
    {
        auto c = std::make_unique<Control>();
        c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
        c->slider.setPopupDisplayEnabled(true, true, this);
        c->slider.setNumDecimalPlacesToDisplay(2);
        c->slider.setColour(juce::Slider::rotarySliderFillColourId, cyan);
        c->slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff253b70));
        c->slider.setColour(juce::Slider::thumbColourId, violet);
        c->slider.setColour(juce::Slider::textBoxTextColourId, text);
        c->slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff050a19));
        c->slider.setColour(juce::Slider::textBoxOutlineColourId, cyan.withAlpha(0.65f));
        c->label.setText(def.label, juce::dontSendNotification);
        c->label.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        c->label.setJustificationType(juce::Justification::centred);
        c->label.setColour(juce::Label::textColourId, text);
        c->label.setInterceptsMouseClicks(false, false);
        c->slider.setVisible(true); c->label.setVisible(true);
        addAndMakeVisible(c->slider); addAndMakeVisible(c->label);
        c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, def.id, c->slider);
        controls.push_back(std::move(c));
    }
}

void TEKITAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff101b3a), 0, 0, bg, 0, (float)getHeight(), false));
    g.fillRect(getLocalBounds());
    auto outer = getLocalBounds().toFloat().reduced(7.0f);
    g.setColour(cyan.withAlpha(0.9f)); g.drawRoundedRectangle(outer, 13.0f, 1.5f);
    g.setColour(violet.withAlpha(0.8f)); g.drawLine(20.0f, 84.0f, (float)getWidth()-20.0f, 84.0f, 1.4f);

    const int margin = 18, gap = 12, top = 105, bottom = getHeight() - 18;
    const int panelW = (getWidth() - 2 * margin - 2 * gap) / 3;
    const juce::String names[] = {"01  OSCILLATORS / VOICE", "02  FILTER / ENVELOPE", "03  TONE / FX / MASTER"};
    for (int i=0; i<3; ++i)
    {
        juce::Rectangle<float> r((float)(margin+i*(panelW+gap)), (float)top, (float)panelW, (float)(bottom-top));
        g.setColour(i==1 ? panelAlt : panel); g.fillRoundedRectangle(r, 10.0f);
        g.setColour((i==1 ? violet : cyan).withAlpha(0.9f)); g.drawRoundedRectangle(r, 10.0f, 1.2f);
        g.setColour(i==1 ? violet : cyan); g.fillRoundedRectangle(r.reduced(9.0f).withHeight(3.0f), 1.5f);
        g.setColour(text); g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(names[i], r.toNearestInt().withHeight(30).translated(10, 6), juce::Justification::centredLeft, false);
    }
}

void TEKITAudioProcessorEditor::resized()
{
    const int w=getWidth(), h=getHeight();
    title.setBounds(24, 8, 205, 58);
    subtitle.setBounds(230, 25, juce::jmax(250, w-610), 25);
    presetBox.setBounds(w-292, 18, 260, 34);
    fxToggle.setBounds(w-292, 55, 180, 24);

    const int margin=18, gap=12, panelTop=105, panelBottom=h-18;
    const int panelW=(w-2*margin-2*gap)/3;
    const int panelX[3]={margin, margin+panelW+gap, margin+2*(panelW+gap)};
    const int areaTop=panelTop+44, areaBottom=panelBottom-10;
    const int innerW=panelW-18, cellW=innerW/3, cellH=(areaBottom-areaTop)/3;
    for (size_t i=0;i<controls.size();++i)
    {
        const int group = (int)i < 9 ? 0 : ((int)i < 18 ? 1 : 2);
        const int local = group==0 ? (int)i : (group==1 ? (int)i-9 : (int)i-18);
        const int col=local%3, row=local/3;
        const int x=panelX[group]+9+col*cellW;
        const int y=areaTop+row*cellH;
        const int cw=cellW-3;
        auto& c=*controls[i];
        c.label.setBounds(x, y, cw, 22);
        const int sliderH=juce::jmax(94, cellH-25);
        c.slider.setBounds(x+1, y+21, cw-2, sliderH);
        c.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, juce::jmin(76,cw-6), 20);
        c.label.setVisible(true); c.slider.setVisible(true);
        c.label.toFront(false); c.slider.toFront(false);
    }
}

void TEKITAudioProcessorEditor::timerCallback()
{
    const int wanted = processor.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != wanted)
        presetBox.setSelectedId(wanted, juce::dontSendNotification);
}
