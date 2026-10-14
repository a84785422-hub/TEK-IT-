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
    l.add(std::make_unique<APF>(juce::ParameterID{"wave", 1}, "Wave", juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 1.0f));
    add("detune", "Detune", 0, 1, 0.35f);
    l.add(std::make_unique<APF>(juce::ParameterID{"unison", 1}, "Unison", juce::NormalisableRange<float>(1.0f, 7.0f, 1.0f), 5.0f));
    add("sub", "Sub Level", 0, 1, 0.15f);
    add("osc2mix", "Osc B Mix", 0, 1, 0.32f);
    add("octave2", "Osc B Octave", -2, 2, 0);
    add("fm", "FM Amount", 0, 1, 0);
    add("noise", "Noise Level", 0, 0.35f, 0);
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
    const bool legatoReuse = active && getP(p, "glide") > 0.001f;
    if (!legatoReuse) midiPitch = (float)n;
    note=n; velocity=vel; active=true; releasing=false; age=0; phase=0.0f; phase2=0.137f; phaseSub=0.0f;
    envParams.attack=getP(p,"attack"); envParams.decay=getP(p,"decay");
    envParams.sustain=getP(p,"sustain"); envParams.release=getP(p,"release");
    ampEnv.setParameters(envParams); ampEnv.noteOn();
}
void TEKITAudioProcessor::Voice::stop() { if (active) { ampEnv.noteOff(); releasing=true; } }
float TEKITAudioProcessor::Voice::render(const juce::AudioProcessorValueTreeState& p, double sr)
{
    if (!active || sr <= 0.0) return 0.0f;

    const float glideSeconds = getP(p, "glide");
    if (glideSeconds > 0.001f)
        midiPitch += ((float)note - midiPitch) * (1.0f - std::exp(-1.0f / (float)(glideSeconds * sr)));
    else midiPitch = (float)note;
    const float base = (float)juce::MidiMessage::getMidiNoteInHertz((int)std::round(midiPitch + getP(p, "pitch")));
    const float fine = std::pow(2.0f, getP(p, "fine") / 1200.0f);
    const float detune = getP(p, "detune");
    const int unisonCount = juce::jlimit(1, 7, (int)std::round(getP(p, "unison")));
    const int wave = (int)getP(p, "wave");
    const float phaseNow = phase;
    auto shape = [wave](float cycles)
    {
        const float x = cycles - std::floor(cycles);
        const float sine = std::sin(juce::MathConstants<float>::twoPi * x);
        if (wave == 0) return sine;
        if (wave == 1) return 2.0f * x - 1.0f; // saw
        if (wave == 2) return 1.0f - 4.0f * std::abs(x - 0.5f); // triangle
        return std::tanh(3.5f * sine); // bright clipped pulse-like tone
    };

    // Oscillator A: unison stack. The stack is intentionally dry; space effects
    // are user-controlled sends and default to zero in every factory patch.
    float stack = 0.0f;
    const float fmAmount = getP(p, "fm");
    const float oscBHz = base * fine * std::pow(2.0f, getP(p, "octave2"));
    const float oscB = shape(phase2 + fmAmount * std::sin(juce::MathConstants<float>::twoPi * phaseNow) * 0.12f);
    for (int u = 0; u < unisonCount; ++u)
    {
        const float position = unisonCount == 1 ? 0.0f : (2.0f * (float)u / (float)(unisonCount - 1) - 1.0f);
        const float ratio = 1.0f + position * detune * 0.018f;
        stack += shape(phaseNow * ratio);
    }
    stack /= std::sqrt((float)unisonCount);
    float v = stack * (1.0f - getP(p, "osc2mix")) + oscB * getP(p, "osc2mix");
    v += shape(phaseSub) * getP(p, "sub") * 0.65f;
    // Deterministic noise component, with inharmonic motion so metallic patches
    // can be fundamentally different without relying on reverb or delay.
    const float noisePhase = phase * 173.31f + phase2 * 71.7f;
    v += std::sin(noisePhase * 6.2831853f) * getP(p, "noise");

    phase += (float)(base * fine / sr);
    phase2 += (float)(oscBHz / sr);
    phaseSub += (float)(base * fine * 0.5 / sr);
    phase -= std::floor(phase); phase2 -= std::floor(phase2); phaseSub -= std::floor(phaseSub);

    filter.setCutoffFrequency(getP(p, "cutoff"));
    filter.setResonance(getP(p, "res"));
    v = filter.processSample(0, v);
    v = std::tanh(v * (1.0f + getP(p, "drive") * 12.0f));
    const float env = ampEnv.getNextSample();
    age += 1.0f;
    if (releasing && !ampEnv.isActive()) active = false;
    return v * env * velocity * 0.24f;
}
void TEKITAudioProcessor::prepareToPlay(double sr, int block)
{
    currentSampleRate = sr;
    for (auto& v : voices) v.prepare(sr, block, getTotalNumOutputChannels());
    const auto delayCapacity = static_cast<size_t>(std::max(2.0, sr * 1.1));
    delayLeft.assign(delayCapacity, 0.0f);
    delayRight.assign(delayCapacity, 0.0f);
    delayWrite = 0;
    eqLowStateL = eqLowStateR = eqMidStateL = eqMidStateR = widthState = 0.0f;
    masterReverb.reset();
}
void TEKITAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            auto it = std::find_if(voices.begin(), voices.end(), [](auto& v) { return !v.active; });
            if (it == voices.end())
                it = std::min_element(voices.begin(), voices.end(), [](auto& a, auto& b) { return a.age > b.age; });
            it->start(msg.getNoteNumber(), msg.getFloatVelocity(), parameters);
        }
        else if (msg.isNoteOff())
        {
            for (auto& v : voices)
                if (v.active && v.note == msg.getNoteNumber()) v.stop();
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            for (auto& v : voices) v.stop();
        }
    }

    const float volume = getP(parameters, "volume");
    const bool fxEnabled = parameters.getRawParameterValue("fxOn")->load() > 0.5f;
    const float rv = fxEnabled ? getP(parameters, "reverb") : 0.0f;
    const float dl = fxEnabled ? getP(parameters, "delay") : 0.0f;
    const float delaySeconds = getP(parameters, "delayTime");
    const float width = getP(parameters, "width");
    const float lowGain = juce::Decibels::decibelsToGain(getP(parameters, "eqLow"));
    const float midGain = juce::Decibels::decibelsToGain(getP(parameters, "eqMid"));
    const float highGain = juce::Decibels::decibelsToGain(getP(parameters, "eqHigh"));
    const float lowCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 180.0f / (float)currentSampleRate);
    const float midCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 1200.0f / (float)currentSampleRate);
    const auto delaySize = delayLeft.size();
    const size_t delaySamples = delaySize > 1 ? juce::jlimit<size_t>(1, delaySize - 1,
        static_cast<size_t>(delaySeconds * currentSampleRate)) : 1;

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.68f; rp.damping = 0.32f;
    rp.wetLevel = rv * 0.65f; rp.dryLevel = 1.0f - rv * 0.25f;
    rp.width = width;
    masterReverb.setParameters(rp);

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float dry = 0.0f;
        for (auto& v : voices) dry += v.render(parameters, currentSampleRate);
        dry *= volume;
        dry = std::tanh(dry * 1.25f);

        // Three-band tone shaping using stable one-pole low-pass states.
        eqLowStateL += lowCoeff * (dry - eqLowStateL);
        eqMidStateL += midCoeff * (dry - eqMidStateL);
        const float low = eqLowStateL;
        const float mid = eqMidStateL - eqLowStateL;
        const float high = dry - eqMidStateL;
        const float toned = low * lowGain + mid * midGain + high * highGain;

        float delayedL = 0.0f, delayedR = 0.0f;
        if (delaySize > 1)
        {
            const size_t read = (delayWrite + delaySize - delaySamples) % delaySize;
            delayedL = delayLeft[read]; delayedR = delayRight[read];
            delayLeft[delayWrite] = juce::jlimit(-1.5f, 1.5f, toned + delayedR * 0.28f * dl);
            delayRight[delayWrite] = juce::jlimit(-1.5f, 1.5f, toned + delayedL * 0.28f * dl);
            delayWrite = (delayWrite + 1) % delaySize;
        }
        const float left = toned * (1.0f - dl * 0.35f) + delayedL * dl;
        const float right = toned * (1.0f - dl * 0.35f) + delayedR * dl;
        outL[i] = left;
        outR[i] = right;
    }

    if (buffer.getNumChannels() > 1 && rv > 0.001f)
        masterReverb.processStereo(outL, outR, buffer.getNumSamples());
    else if (buffer.getNumChannels() == 1 && rv > 0.001f)
        masterReverb.processMono(outL, buffer.getNumSamples());

    // Mid/side width control: 0 is mono, 1 preserves the stereo effect image.
    if (buffer.getNumChannels() > 1)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float mid = 0.5f * (outL[i] + outR[i]);
            const float side = 0.5f * (outL[i] - outR[i]) * (width * 2.0f);
            outL[i] = std::tanh(mid + side);
            outR[i] = std::tanh(mid - side);
        }
}
void TEKITAudioProcessor::setCurrentProgram(int i) { loadPreset(i); }
const juce::String TEKITAudioProcessor::getProgramName(int i) { return i>=0&&i<30 ? presetNames[i] : "TEK IT Preset"; }
void TEKITAudioProcessor::loadPreset(int i)
{
    currentProgram = juce::jlimit(0, 29, i);
    auto set = [this](const char* id, float v)
    {
        if (auto* parameter = parameters.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
    };
    // Each row is an intentional sound-design starting point, not a shared patch
    // with a different effects send. Columns: wave, detune, unison, sub, cutoff,
    // resonance, drive, attack, decay, sustain, release, pitch, fine.
    struct Core { float wave, detune, unison, sub, cutoff, res, drive, attack, decay, sustain, release, pitch, fine; };
    static constexpr Core cores[30] = {
        {3,.08f,2,.05f,5100,.82f,.68f,.001f,.08f,.18f,.10f,0,-7},
        {2,.16f,3,.02f,7200,.42f,.78f,.001f,.14f,.30f,.12f,0,4},
        {3,.23f,4,.10f,4300,.70f,.56f,.003f,.22f,.48f,.18f,0,-3},
        {1,.12f,2,.14f,2600,.92f,.62f,.008f,.30f,.62f,.20f,0,6},
        {3,.31f,3,.00f,8900,.55f,.88f,.001f,.06f,.12f,.08f,0,-11},
        {2,.20f,4,.24f,3500,.72f,.72f,.002f,.18f,.40f,.16f,-12,2},
        {3,.05f,2,.03f,11200,.34f,.48f,.001f,.09f,.22f,.09f,0,9},
        {1,.28f,3,.08f,6400,.64f,.80f,.004f,.12f,.26f,.14f,0,-5},
        {3,.38f,4,.01f,4700,1.02f,.94f,.001f,.20f,.16f,.11f,12,3},
        {2,.18f,5,.12f,7800,.48f,.74f,.002f,.26f,.55f,.22f,0,-2},
        {1,.56f,7,.12f,12500,.20f,.24f,.012f,.42f,.86f,.38f,0,0},
        {1,.72f,7,.18f,9400,.28f,.32f,.025f,.55f,.78f,.52f,0,-4},
        {1,.44f,7,.06f,15000,.16f,.20f,.004f,.24f,.92f,.24f,0,2},
        {1,.88f,7,.20f,6900,.40f,.44f,.002f,.34f,.72f,.30f,0,7},
        {1,.35f,5,.08f,10500,.22f,.28f,.035f,.68f,.88f,.60f,0,-1},
        {1,.64f,7,.24f,8200,.32f,.38f,.008f,.28f,.66f,.44f,0,4},
        {1,.92f,7,.10f,13200,.18f,.34f,.001f,.18f,.84f,.28f,0,-6},
        {1,.50f,6,.03f,5600,.52f,.30f,.06f,.80f,.74f,.72f,0,1},
        {1,.78f,7,.16f,11800,.24f,.42f,.003f,.32f,.94f,.34f,0,-3},
        {1,.60f,7,.22f,7600,.36f,.50f,.001f,.22f,.58f,.18f,0,5},
        {3,.10f,2,.02f,4200,.90f,.82f,.001f,.10f,.10f,.09f,12,0},
        {3,.26f,3,.04f,6800,.76f,.96f,.001f,.16f,.28f,.14f,0,11},
        {0,.14f,2,.00f,9800,.38f,.22f,.018f,.48f,.44f,.66f,0,-8},
        {3,.46f,4,.12f,3100,1.08f,.86f,.002f,.24f,.36f,.20f,-12,5},
        {2,.32f,5,.05f,14500,.26f,.54f,.004f,.36f,.60f,.32f,12,-4},
        {3,.40f,4,.20f,5300,.84f,.92f,.001f,.12f,.20f,.12f,0,8},
        {0,.22f,3,.01f,8700,.46f,.30f,.045f,.72f,.35f,.90f,0,-2},
        {2,.54f,5,.18f,3900,.68f,.76f,.002f,.20f,.50f,.22f,-12,3},
        {3,.66f,6,.06f,10100,.58f,.64f,.006f,.44f,.82f,.46f,0,-9},
        {1,.70f,7,.14f,10800,.30f,.46f,.003f,.30f,.88f,.40f,0,0}
    };
    const auto& c = cores[currentProgram];
    const int family = currentProgram / 10;
    const int slot = currentProgram % 10;
    set("wave", c.wave); set("detune", c.detune); set("unison", c.unison); set("sub", c.sub);
    set("cutoff", c.cutoff); set("res", c.res); set("drive", c.drive);
    set("attack", c.attack); set("decay", c.decay); set("sustain", c.sustain); set("release", c.release);
    set("pitch", c.pitch); set("fine", c.fine);
    // Tone and space are finishing touches; identity comes primarily from the core patch above.
    // Keep tone controls neutral too: all presets are dry, un-EQ'd starting sounds.
    set("eqLow", 0.0f); set("eqMid", 0.0f); set("eqHigh", 0.0f);
    // Factory patches are completely dry: effects are opt-in after loading.
    set("reverb", 0.0f);
    set("delay", 0.0f);
    set("delayTime", 0.09f + (slot % 6) * 0.13f);
    set("width", family == 1 ? 0.72f + (slot % 3) * 0.08f : family == 0 ? 0.42f + (slot % 4) * 0.12f : 0.55f + (slot % 4) * 0.10f);
    set("fxOn", 0.0f);
    // Dedicated oscillator architecture varies by family and by patch. These
    // parameters shape the raw tone before any optional effects are applied.
    static constexpr float oscBMix[30] = { .18f,.08f,.34f,.25f,.06f,.38f,.12f,.22f,.04f,.31f, .30f,.24f,.16f,.28f,.12f,.34f,.20f,.18f,.32f,.26f, .72f,.62f,.55f,.78f,.46f,.66f,.52f,.74f,.58f,.80f };
    static constexpr float octaveB[30] = { 0,0,1,0,1,-1,1,0,2,0, 0,0,0,0,0,0,0,0,0,0, 1,0,1,-1,2,1,-1,0,2,1 };
    static constexpr float fmAmt[30] = { .12f,.04f,.22f,.08f,.30f,.18f,.05f,.24f,.34f,.10f, .02f,.03f,.01f,.02f,.01f,.04f,.02f,.01f,.03f,.02f, .82f,.68f,.28f,.74f,.55f,.90f,.38f,.76f,.44f,.88f };
    static constexpr float noiseAmt[30] = { .02f,.04f,.01f,.05f,.03f,.02f,.01f,.04f,.06f,.02f, .00f,.00f,.00f,.00f,.00f,.00f,.00f,.00f,.00f,.00f, .04f,.08f,.02f,.06f,.01f,.10f,.03f,.07f,.02f,.05f };
    set("osc2mix", oscBMix[currentProgram]); set("octave2", octaveB[currentProgram]);
    set("fm", fmAmt[currentProgram]); set("noise", noiseAmt[currentProgram]);
    set("volume", 0.72f);
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
