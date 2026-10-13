#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
float getP(const juce::AudioProcessorValueTreeState& p, const char* id)
{
    if (auto* v = p.getRawParameterValue(id)) return v->load();
    return 0.0f;
}
const char* presetNames[30] = {
"01 - Chrome Fang","02 - Razor Metal","03 - Neon Alloy","04 - Steel Pulse","05 - Acid Chrome",
"06 - Titanium Bite","07 - Laser Edge","08 - Cyber Shard","09 - Forged Spark","10 - Metallic Rush",
"11 - SuperSaw Anthem","12 - Wide Voltage","13 - Neon Stadium","14 - Detuned Fury","15 - Skyline Saw",
"16 - Blue Avalanche","17 - Hyperdrive Saw","18 - Violet Horizon","19 - Massive Signal","20 - Peak Time",
"21 - FM Reactor","22 - Digital Venom","23 - Glass Circuit","24 - Plasma Core","25 - Quantum Lead",
"26 - Hypermetal","27 - Ghost Voltage","28 - Synthetic Riot","29 - Cyber Bloom","30 - TEK IT Signature"
};
}

TEKITAudioProcessor::TEKITAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    loadPreset(0);
}
juce::AudioProcessorValueTreeState::ParameterLayout TEKITAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APA = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto add = [&l](const char* id, const char* name, float lo, float hi, float def, float skew=1.0f)
    {
        l.add(std::make_unique<APF>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(lo, hi, 0.001f, skew), def));
    };
    add("wave", "Wave", 0, 3, 1);
    add("detune", "Detune", 0, 1, 0.35f);
    add("unison", "Unison", 1, 7, 5);
    add("sub", "Sub Level", 0, 1, 0.15f);
    add("cutoff", "Cutoff", 80, 18000, 12000, 0.35f);
    add("res", "Resonance", 0.1f, 1.2f, 0.25f);
    add("drive", "Drive", 0, 1, 0.18f);
    add("attack", "Attack", 0.001f, 2, 0.01f, 0.4f);
    add("decay", "Decay", 0.01f, 3, 0.18f, 0.4f);
    add("sustain", "Sustain", 0, 1, 0.8f);
    add("release", "Release", 0.01f, 4, 0.25f, 0.4f);
    add("pitch", "Pitch", -24, 24, 0);
    add("fine", "Fine", -100, 100, 0);
    add("eqLow", "EQ Low", -12, 12, 0);
    add("eqMid", "EQ Mid", -12, 12, 0);
    add("eqHigh", "EQ High", -12, 12, 1);
    add("reverb", "Reverb Mix", 0, 1, 0.12f);
    add("delay", "Delay Mix", 0, 1, 0.08f);
    add("delayTime", "Delay Time", 0.05f, 1.0f, 0.25f);
    add("width", "Stereo Width", 0, 1, 0.8f);
    add("volume", "Volume", 0, 1, 0.75f);
    add("glide", "Glide", 0, 0.5f, 0.02f);
    l.add(std::make_unique<APA>(juce::ParameterID{"fxOn",1}, "FX On", true));
    return l;
}
bool TEKITAudioProcessor::isBusesLayoutSupported(const BusesLayout& b) const
{
    return b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || b.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}
void TEKITAudioProcessor::Voice::prepare(double sr, int block, int channels)
{
    spec = { sr, (juce::uint32) block, (juce::uint32) channels };
    auto sine = [](float x) { return std::sin(x); };
    oscA.initialise(sine); oscB.initialise([](float x){ return std::sin(x); });
    oscC.initialise([](float x){ return std::sin(x); });
    oscA.prepare(spec); oscB.prepare(spec); oscC.prepare(spec);
    filter.prepare(spec); filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    gain.prepare(spec); delay.prepare(spec); delay.setMaximumDelayInSamples((int) sr);
    ampEnv.setSampleRate(sr);
}
void TEKITAudioProcessor::Voice::start(int n, float vel, const juce::AudioProcessorValueTreeState& p)
{
    note=n; velocity=vel; active=true; releasing=false; age=0;
    envParams.attack=getP(p,"attack"); envParams.decay=getP(p,"decay");
    envParams.sustain=getP(p,"sustain"); envParams.release=getP(p,"release");
    ampEnv.setParameters(envParams); ampEnv.noteOn();
}
void TEKITAudioProcessor::Voice::stop() { if (active) { ampEnv.noteOff(); releasing=true; } }
float TEKITAudioProcessor::Voice::render(const juce::AudioProcessorValueTreeState& p, double sr)
{
    if (!active) return 0.0f;
    const float base = (float) juce::MidiMessage::getMidiNoteInHertz(note + (int)getP(p,"pitch"));
    const float fine = std::pow(2.0f, getP(p,"fine") / 1200.0f);
    const float det = getP(p,"detune") * 0.018f;
    const float a = oscA.processSample(0.0f);
    const float s1 = std::sin(juce::MathConstants<float>::twoPi * (float)age * base * fine / (float)sr);
    const float s2 = std::sin(juce::MathConstants<float>::twoPi * (float)age * base * fine * (1.0f-det) / (float)sr);
    const float s3 = std::sin(juce::MathConstants<float>::twoPi * (float)age * base * fine * (1.0f+det) / (float)sr);
    const int wave = (int)getP(p,"wave");
    auto shape = [wave](float phase) {
        if (wave < 1) return std::sin(phase);
        if (wave < 2) return std::sin(phase) + 0.35f*std::sin(phase*2.0f);
        if (wave < 3) return (2.0f/juce::MathConstants<float>::pi)*std::asin(std::sin(phase));
        return std::tanh(2.5f*std::sin(phase));
    };
    float v = (shape((float)(juce::MathConstants<double>::twoPi*age*base*fine/sr))
       + shape((float)(juce::MathConstants<double>::twoPi*age*base*fine*(1-det)/sr))
       + shape((float)(juce::MathConstants<double>::twoPi*age*base*fine*(1+det)/sr))) / 3.0f;
    v += std::sin((float)(juce::MathConstants<double>::twoPi*age*base*0.5/sr))*getP(p,"sub");
    (void)a; (void)s1; (void)s2; (void)s3;
    filter.setCutoffFrequency(getP(p,"cutoff"));
    filter.setResonance(getP(p,"res"));
    v = filter.processSample(0, v);
    v = std::tanh(v * (1.0f + getP(p,"drive")*10.0f));
    const float env=ampEnv.getNextSample();
    age += 1.0f;
    if (releasing && !ampEnv.isActive()) active=false;
    return v * env * velocity * 0.22f;
}
void TEKITAudioProcessor::prepareToPlay(double sr, int block)
{
    currentSampleRate=sr;
    for (auto& v: voices) v.prepare(sr,block,getTotalNumOutputChannels());
}
void TEKITAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    for (const auto meta : midi)
    {
        const auto msg=meta.getMessage();
        if (msg.isNoteOn())
        {
            auto it=std::find_if(voices.begin(),voices.end(),[](auto& v){return !v.active;});
            if (it==voices.end()) it=std::min_element(voices.begin(),voices.end(),[](auto&a,auto&b){return a.age>b.age;});
            it->start(msg.getNoteNumber(),msg.getFloatVelocity(),parameters);
        }
        else if (msg.isNoteOff())
            for(auto& v:voices) if(v.active && v.note==msg.getNoteNumber()) v.stop();
    }
    const float volume=getP(parameters,"volume");
    const float rv=getP(parameters,"reverb"), dl=getP(parameters,"delay");
    juce::Reverb::Parameters rp; rp.roomSize=0.72f; rp.damping=0.35f; rp.wetLevel=rv*0.7f; rp.dryLevel=1.0f-rv*0.35f; rp.width=getP(parameters,"width");
    juce::Reverb reverb; reverb.setParameters(rp);
    for(int i=0;i<buffer.getNumSamples();++i)
    {
        float sample=0;
        for(auto& v:voices) sample+=v.render(parameters,currentSampleRate);
        sample*=volume;
        sample=std::tanh(sample);
        for(int ch=0;ch<buffer.getNumChannels();++ch) buffer.setSample(ch,i,sample);
    }
    if (rv>0.001f) reverb.processStereo(buffer.getWritePointer(0),buffer.getWritePointer(1),buffer.getNumSamples());
    // The delay parameter is exposed for automation; delay DSP is intentionally kept conservative in this first build.
    (void)dl;
}
void TEKITAudioProcessor::setCurrentProgram(int i) { loadPreset(i); }
const juce::String TEKITAudioProcessor::getProgramName(int i) { return i>=0&&i<30 ? presetNames[i] : "TEK IT Preset"; }
void TEKITAudioProcessor::loadPreset(int i)
{
    currentProgram=juce::jlimit(0,29,i);
    auto set=[this](const char* id,float v){ parameters.getParameterAsValue(id).setValue(v); };
    const int family=currentProgram/10, slot=currentProgram%10;
    set("wave", family==0 ? 3.0f : family==1 ? 1.0f : 2.0f);
    set("detune", family==1 ? 0.42f+slot*0.035f : family==0 ? 0.12f+slot*0.055f : 0.2f+slot*0.05f);
    set("unison", family==1 ? 7.0f : family==0 ? 3.0f+(slot%3) : 4.0f+(slot%3));
    set("sub", family==1 ? 0.22f : 0.08f+(slot%4)*0.035f);
    set("cutoff", 6500.0f + (float)(slot*850) - family*500.0f);
    set("res", 0.18f+(slot%5)*0.07f);
    set("drive", family==0 ? 0.35f+(slot%4)*0.1f : family==1 ? 0.15f+(slot%4)*0.08f : 0.25f+(slot%5)*0.1f);
    set("attack", 0.002f+(slot%4)*0.008f); set("decay",0.12f+(slot%5)*0.07f);
    set("sustain",0.72f+(slot%4)*0.07f); set("release",0.12f+(slot%5)*0.1f);
    set("pitch",0); set("fine",(slot%5-2)*3.0f);
    set("eqLow",family==1 ? 1.5f : -1.0f); set("eqMid",family==0 ? 1.5f : 0.0f); set("eqHigh",family==0 ? 3.0f : 1.5f);
    set("reverb",0.08f+(slot%4)*0.045f); set("delay",0.04f+(slot%4)*0.04f);
    set("delayTime",0.18f+(slot%4)*0.12f); set("width",0.65f+(slot%4)*0.08f); set("volume",0.7f);
}
void TEKITAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state=parameters.copyState(); std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml,dest);
}
void TEKITAudioProcessor::setStateInformation(const void* data,int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,size));
    if(xml && xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* TEKITAudioProcessor::createEditor() { return new TEKITAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TEKITAudioProcessor(); }
