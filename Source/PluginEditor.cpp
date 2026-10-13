#include "PluginEditor.h"

TEKITAudioProcessorEditor::Knob::Knob(TEKITAudioProcessor& p, const juce::String& id, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff13bfff));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffd84cff));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff071126));
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa8dfff));
    addAndMakeVisible(slider); addAndMakeVisible(label);
    attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,id,slider));
}
void TEKITAudioProcessorEditor::Knob::resized()
{
    label.setBounds(0,0,getWidth(),20); slider.setBounds(0,18,getWidth(),getHeight()-18);
}
// Attachments are owned here so host automation and GUI remain in sync.
TEKITAudioProcessorEditor::TEKITAudioProcessorEditor(TEKITAudioProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setSize(1040,650);
    title.setText("TEK IT",juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(34.0f,juce::Font::bold)));
    title.setColour(juce::Label::textColourId,juce::Colour(0xff37c9ff));
    subtitle.setText("METALLIC LEADS  /  SUPERSAW  /  HYBRID SYNTHS",juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId,juce::Colour(0xffdf67ff));
    addAndMakeVisible(title); addAndMakeVisible(subtitle); addAndMakeVisible(presetBox);
    for(int i=0;i<processor.getNumPrograms();++i) presetBox.addItem(processor.getProgramName(i),i+1);
    presetBox.setSelectedId(1,juce::dontSendNotification);
    presetBox.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff071126));
    presetBox.setColour(juce::ComboBox::textColourId,juce::Colours::white);
    presetBox.onChange=[this]{ processor.setCurrentProgram(presetBox.getSelectedId()-1); };
    const std::pair<const char*,const char*> list[] = {
        {"wave","WAVE"},{"detune","DETUNE"},{"unison","UNISON"},{"sub","SUB"},
        {"cutoff","CUTOFF"},{"res","RESONANCE"},{"drive","DRIVE"},
        {"attack","ATTACK"},{"decay","DECAY"},{"sustain","SUSTAIN"},{"release","RELEASE"},
        {"pitch","PITCH"},{"fine","FINE"},{"eqLow","EQ LOW"},{"eqMid","EQ MID"},{"eqHigh","EQ HIGH"},
        {"reverb","REVERB"},{"delay","DELAY"},{"delayTime","DELAY TIME"},{"width","WIDTH"},{"volume","MASTER"}
    };
    for(auto& item:list) { auto k=std::make_unique<Knob>(processor,item.first,item.second); addAndMakeVisible(*k); knobs.push_back(std::move(k)); }
}
void TEKITAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff030817));
    g.setColour(juce::Colour(0xff087dff));
    g.drawRoundedRectangle(8.0f, 8.0f, (float) getWidth() - 16.0f,
                           (float) getHeight() - 16.0f, 10.0f, 1.5f);
    g.setColour(juce::Colour(0xffc13dff));
    g.drawLine(20.0f, 76.0f, (float) getWidth() - 20.0f, 76.0f, 1.0f);
}

void TEKITAudioProcessorEditor::resized()
{
    title.setBounds(24,12,250,50); subtitle.setBounds(285,25,480,25); presetBox.setBounds(780,24,230,30);
    const int left=22, top=95, gap=10, w=90, h=110, cols=10;
    for(size_t i=0;i<knobs.size();++i) {
        int row=(int)i/cols, col=(int)i%cols;
        knobs[i]->setBounds(left+col*(w+gap),top+row*(h+12),w,h);
    }
}
