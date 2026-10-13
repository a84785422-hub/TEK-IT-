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
    if (!active)
        return 0.0f;

    const float pitchSemitones = getP(p, "pitch");
    const float base = (float) juce::MidiMessage::getMidiNoteInHertz(note)
                       * std::pow(2.0f, pitchSemitones / 12.0f);
    const float fine = std::pow(2.0f, getP(p, "fine") / 1200.0f);
    const float detune = getP(p, "detune") * 0.025f;
    const int wave = juce::jlimit(0, 3, (int) getP(p, "wave"));
    const int unisonCount = juce::jlimit(1, 7, (int) std::round(getP(p, "unison")));

    auto shape = [wave](float phase)
    {
        if (wave == 0)
            return std::sin(phase);
        if (wave == 1)
            return std::sin(phase) + 0.35f * std::sin(phase * 2.0f);
        if (wave == 2)
            return (2.0f / juce::MathConstants<float>::pi) * std::asin(std::sin(phase));
        return std::tanh(2.5f * std::sin(phase) + 0.5f * std::sin(phase * 3.0f));
    };

    float signal = 0.0f;
    for (int oscillator = 0; oscillator < unisonCount; ++oscillator)
    {
        const float position = unisonCount == 1 ? 0.0f
            : (2.0f * (float) oscillator / (float) (unisonCount - 1) - 1.0f);
        const float frequency = base * fine * (1.0f + position * detune);
        const float phase = (float) std::fmod(age * frequency / sr
                                              * juce::MathConstants<double>::twoPi,
                                              juce::MathConstants<double>::twoPi);
        const float panWeight = 1.0f - 0.12f * std::abs(position);
        signal += shape(phase) * panWeight;
    }
    signal /= (float) unisonCount;
    const float subPhase = (float) std::fmod(age * base * 0.5 / sr
                                             * juce::MathConstants<double>::twoPi,
                                             juce::MathConstants<double>::twoPi);
    signal += std::sin(subPhase) * getP(p, "sub");

    filter.setCutoffFrequency(getP(p, "cutoff"));
    filter.setResonance(getP(p, "res"));
    signal = filter.processSample(0, signal);
    signal = std::tanh(signal * (1.0f + getP(p, "drive") * 10.0f));

    const float envelope = ampEnv.getNextSample();
    age += 1.0f;
    if (releasing && !ampEnv.isActive())
        active = false;

    return signal * envelope * velocity * 0.30f;
}
void TEKITAudioProcessor::prepareToPlay(double sr, int block)
{
    currentSampleRate = sr;
    for (auto& voice : voices)
        voice.prepare(sr, block, getTotalNumOutputChannels());

    masterReverb.reset();
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.72f;
    reverbParams.damping = 0.32f;
    reverbParams.wetLevel = 0.35f;
    reverbParams.dryLevel = 0.75f;
    reverbParams.width = 1.0f;
    masterReverb.setParameters(reverbParams);

    const auto delaySize = static_cast<size_t>(std::max(2.0, sr * 2.0));
    delayBufferL.assign(delaySize, 0.0f);
    delayBufferR.assign(delaySize, 0.0f);
    delayWritePosition = 0;
    eqLowStateL = eqLowStateR = 0.0f;
    eqMidStateL = eqMidStateR = 0.0f;
    eqHighStateL = eqHighStateR = 0.0f;
}

void TEKITAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            auto voice = std::find_if(voices.begin(), voices.end(),
                                      [](const auto& candidate) { return !candidate.active; });
            if (voice == voices.end())
                voice = std::min_element(voices.begin(), voices.end(),
                                         [](const auto& a, const auto& b) { return a.age > b.age; });
            voice->start(message.getNoteNumber(), message.getFloatVelocity(), parameters);
        }
        else if (message.isNoteOff())
        {
            for (auto& voice : voices)
                if (voice.active && voice.note == message.getNoteNumber())
                    voice.stop();
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            for (auto& voice : voices)
                voice.stop();
        }
    }

    const float master = getP(parameters, "volume");
    const float reverbMix = getP(parameters, "reverb");
    const float delayMix = getP(parameters, "delay");
    const float delaySeconds = juce::jlimit(0.05f, 1.0f, getP(parameters, "delayTime"));
    const float delayFeedback = 0.28f + delayMix * 0.42f;
    const float width = getP(parameters, "width");

    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.28f + reverbMix * 0.68f;
    reverbParams.damping = 0.25f;
    reverbParams.wetLevel = reverbMix * 0.48f;
    reverbParams.dryLevel = 1.0f - reverbMix * 0.20f;
    reverbParams.width = width;
    masterReverb.setParameters(reverbParams);

    const float lowGain = juce::Decibels::decibelsToGain(getP(parameters, "eqLow"));
    const float midGain = juce::Decibels::decibelsToGain(getP(parameters, "eqMid"));
    const float highGain = juce::Decibels::decibelsToGain(getP(parameters, "eqHigh"));
    const float lowCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 180.0f / (float) currentSampleRate);
    const float midCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 1200.0f / (float) currentSampleRate);
    const auto delaySamples = juce::jlimit<size_t>(1, delayBufferL.size() - 1,
        static_cast<size_t>(delaySeconds * currentSampleRate));

    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        float dry = 0.0f;
        for (auto& voice : voices)
            dry += voice.render(parameters, currentSampleRate);
        dry *= master;

        // Three-band tonal shaping with persistent low/mid states.
        eqLowStateL += lowCoeff * (dry - eqLowStateL);
        eqMidStateL += midCoeff * (dry - eqMidStateL);
        const float low = eqLowStateL;
        const float mid = eqMidStateL - eqLowStateL;
        const float high = dry - eqMidStateL;
        float shaped = low * lowGain + mid * midGain + high * highGain;
        shaped = std::tanh(shaped * 1.25f);

        const size_t readPosition = (delayWritePosition + delayBufferL.size() - delaySamples)
                                    % delayBufferL.size();
        const float echoL = delayBufferL[readPosition];
        const float echoR = delayBufferR[readPosition];
        delayBufferL[delayWritePosition] = shaped + echoR * delayFeedback;
        delayBufferR[delayWritePosition] = shaped + echoL * delayFeedback;
        delayWritePosition = (delayWritePosition + 1) % delayBufferL.size();

        const float delayedL = shaped * (1.0f - delayMix) + echoL * delayMix;
        const float delayedR = shaped * (1.0f - delayMix) + echoR * delayMix;

        // Width is applied to the stereo side signal, keeping the mono centre stable.
        const float mid = 0.5f * (delayedL + delayedR);
        const float side = 0.5f * (delayedL - delayedR) * (width * 2.0f);
        const float outputL = mid + side;
        const float outputR = mid - side;

        if (buffer.getNumChannels() >= 2)
        {
            buffer.setSample(0, sampleIndex, outputL);
            buffer.setSample(1, sampleIndex, outputR);
        }
        else if (buffer.getNumChannels() == 1)
        {
            buffer.setSample(0, sampleIndex, mid);
        }
    }

    if (buffer.getNumChannels() >= 2 && reverbMix > 0.0001f)
        masterReverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1),
                                   buffer.getNumSamples());
}
void TEKITAudioProcessor::setCurrentProgram(int i) { loadPreset(i); }
const juce::String TEKITAudioProcessor::getProgramName(int i) { return i>=0&&i<30 ? presetNames[i] : "TEK IT Preset"; }
void TEKITAudioProcessor::loadPreset(int i)
{
    currentProgram = juce::jlimit(0, 29, i);
    auto set = [this](const char* id, float value)
    {
        if (auto* parameter = parameters.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };

    const int family = currentProgram / 10;
    const int slot = currentProgram % 10;
    const float position = static_cast<float>(slot) / 9.0f;

    // Family 1: metallic, biting leads. Family 2: wide supersaws.
    // Family 3: digital/hybrid leads. Each slot has its own parameter fingerprint.
    const float wave = family == 0 ? (slot % 3 == 0 ? 3.0f : slot % 3 == 1 ? 2.0f : 1.0f)
                     : family == 1 ? (slot % 2 == 0 ? 1.0f : 0.0f)
                     : (slot % 4 == 0 ? 3.0f : slot % 4 == 1 ? 2.0f : slot % 4 == 2 ? 0.0f : 1.0f);
    set("wave", wave);
    set("detune", family == 0 ? 0.08f + position * 0.34f
                    : family == 1 ? 0.38f + position * 0.42f
                    : 0.16f + position * 0.62f);
    set("unison", family == 0 ? 2.0f + (slot % 4)
                    : family == 1 ? 5.0f + (slot % 3)
                    : 3.0f + (slot % 5));
    set("sub", family == 0 ? 0.02f + position * 0.22f
                 : family == 1 ? 0.12f + position * 0.35f
                 : 0.03f + ((slot * 3) % 8) * 0.055f);
    set("cutoff", family == 0 ? 3200.0f + slot * 1150.0f
                    : family == 1 ? 7500.0f + slot * 950.0f
                    : 2400.0f + ((slot * 7) % 10) * 1450.0f);
    set("res", 0.12f + ((slot * 3 + family) % 7) * 0.105f);
    set("drive", family == 0 ? 0.42f + (slot % 5) * 0.105f
                   : family == 1 ? 0.10f + (slot % 4) * 0.12f
                   : 0.22f + ((slot * 2) % 6) * 0.12f);
    set("attack", family == 0 ? 0.001f + (slot % 4) * 0.002f
                    : family == 1 ? 0.004f + (slot % 5) * 0.012f
                    : 0.002f + (slot % 6) * 0.025f);
    set("decay", 0.08f + ((slot * 2 + family) % 8) * 0.12f);
    set("sustain", family == 0 ? 0.62f + (slot % 4) * 0.095f
                     : family == 1 ? 0.72f + (slot % 3) * 0.09f
                     : 0.35f + ((slot * 3) % 7) * 0.10f);
    set("release", family == 0 ? 0.08f + (slot % 5) * 0.085f
                     : family == 1 ? 0.16f + (slot % 5) * 0.18f
                     : 0.12f + ((slot * 2) % 8) * 0.16f);
    set("pitch", (family == 2 && slot == 7) ? 12.0f : 0.0f);
    set("fine", ((slot * 7 + family * 3) % 17 - 8) * 2.5f);
    set("eqLow", family == 1 ? 1.0f + (slot % 4) * 0.6f : -2.0f + (slot % 5) * 0.6f);
    set("eqMid", family == 0 ? 1.0f + (slot % 5) * 1.1f
                   : family == 1 ? -1.5f + (slot % 4) * 0.4f
                   : -2.0f + ((slot * 3) % 8) * 0.75f);
    set("eqHigh", family == 0 ? 2.0f + (slot % 5) * 1.5f
                    : family == 1 ? 0.5f + (slot % 4) * 0.8f
                    : 1.0f + ((slot * 2) % 7) * 1.35f);
    set("reverb", family == 0 ? 0.04f + (slot % 5) * 0.045f
                    : family == 1 ? 0.08f + (slot % 4) * 0.055f
                    : 0.12f + (slot % 5) * 0.12f);
    set("delay", family == 0 ? 0.02f + (slot % 4) * 0.055f
                   : family == 1 ? 0.04f + (slot % 5) * 0.045f
                   : 0.10f + (slot % 5) * 0.13f);
    set("delayTime", 0.09f + ((slot * 3 + family) % 9) * 0.095f);
    set("width", family == 0 ? 0.35f + position * 0.45f
                   : family == 1 ? 0.75f + position * 0.25f
                   : 0.45f + ((slot * 2) % 6) * 0.09f);
    set("volume", 0.68f);
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
