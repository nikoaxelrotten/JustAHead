#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static float sanitize (float v)
{
    if (! std::isfinite (v)) return 0.0f;
    return juce::jlimit (-4.0f, 4.0f, v);
}

//==============================================================================
APVTS::ParameterLayout JustAHeadProcessor::createParameterLayout()
{
    using namespace juce;
    APVTS::ParameterLayout layout;

    auto addFloat = [&layout] (const char* id, const char* name, float min, float max, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
            NormalisableRange<float> (min, max, 0.1f), def,
            AudioParameterFloatAttributes().withLabel (name)));
    };

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "head", 1 }, "Head",
        StringArray { "British OD", "Modern 3Ch", "UK Iommi", "American Thick" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "pedal", 1 }, "Pedal",
        StringArray { "Off", "Overdrive", "Distortion" }, 0));

    addFloat ("gain",     "Gain",        0.0f, 10.0f,  6.0f);
    addFloat ("bass",     "Bass",        0.0f, 10.0f,  5.0f);
    addFloat ("mid",      "Mid",         0.0f, 10.0f,  5.0f);
    addFloat ("treble",   "Treble",      0.0f, 10.0f,  5.0f);
    addFloat ("presence", "Presence",    0.0f, 10.0f,  5.0f);
    addFloat ("drive",    "Drive",       0.0f, 10.0f,  5.0f);
    addFloat ("ptone",    "Pedal Tone",  0.0f, 10.0f,  5.0f);
    addFloat ("level",    "Pedal Level", -24.0f, 12.0f, 0.0f);
    addFloat ("master",   "Master",      -24.0f, 12.0f, 0.0f);

    // --- Efectos post-head ---
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mod", 1 }, "Mod",
        StringArray { "Off", "Chorus", "Flanger", "Phaser", "Tremolo", "Slicer", "Ring Mod" }, 0));

    auto addFx = [&layout] (const char* id, const char* name, float min, float max, float def, float step,
                            float skew, const String& unit)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
            NormalisableRange<float> (min, max, step, skew), def,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([unit, step] (float v, int)
                    { return String (step >= 1.0f ? String ((int) std::round (v)) : String (v, 1)) + unit; })
                .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));
    };
    addFx ("mrate",  "Mod Rate",     0.0f, 10.0f,   3.0f, 0.1f, 1.0f, "");
    addFx ("mdepth", "Mod Depth",    0.0f, 10.0f,   6.0f, 0.1f, 1.0f, "");
    addFx ("mmix",   "Mod Mix",      0.0f, 100.0f, 50.0f, 1.0f, 1.0f, "%");
    addFx ("dtime",  "Delay Time",  40.0f, 1200.0f, 380.0f, 1.0f, 0.5f, " ms");
    addFx ("dfb",    "Delay Feedback", 0.0f, 95.0f, 35.0f, 1.0f, 1.0f, "%");
    addFx ("dmix",   "Delay Mix",    0.0f, 100.0f,  0.0f, 1.0f, 1.0f, "%");
    addFx ("rsize",  "Reverb Size",  0.0f, 10.0f,   4.0f, 0.1f, 1.0f, "");
    addFx ("rdamp",  "Reverb Damp",  0.0f, 10.0f,   5.0f, 0.1f, 1.0f, "");
    addFx ("rmix",   "Reverb Mix",   0.0f, 100.0f,  0.0f, 1.0f, 1.0f, "%");

    return layout;
}

//==============================================================================
JustAHeadProcessor::JustAHeadProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

void JustAHeadProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    engine.prepare (sr, samplesPerBlock);
    fx.prepare (sr);
    setLatencySamples (engine.getLatency());
    mono.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    master.reset (sr, 0.02);
    master.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("master")->load()));
}

bool JustAHeadProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void JustAHeadProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    if (total <= 0 || buffer.getNumChannels() < 2) return;

    auto param = [this] (const char* id) -> float { return apvts.getRawParameterValue (id)->load(); };

    jah::Params p;
    p.amp = (int) param ("head");
    p.pedal = (int) param ("pedal");
    p.gain = param ("gain");
    p.bass = param ("bass");
    p.mid = param ("mid");
    p.treble = param ("treble");
    p.presence = param ("presence");
    p.drive = param ("drive");
    p.ptone = param ("ptone");
    p.levelDb = param ("level");
    engine.setParams (p);

    jah::FxParams fp;
    fp.mod = (int) param ("mod");
    fp.mrate = param ("mrate");
    fp.mdepth = param ("mdepth");
    fp.mmix = param ("mmix") / 100.0f;
    fp.dtimeMs = param ("dtime");
    fp.dfb = param ("dfb") / 100.0f;
    fp.dmix = param ("dmix") / 100.0f;
    fp.rsize = param ("rsize");
    fp.rdamp = param ("rdamp");
    fp.rmix = param ("rmix") / 100.0f;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) fp.bpm = *bpm;
            fp.playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { fp.ppq = *q; fp.hasPpq = true; }
        }
    const double fsHost = getSampleRate();
    master.setTargetValue (juce::Decibels::decibelsToGain (param ("master")));

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);
    const int chunk = (int) mono.size();

    // Un amp es mono: se suma la entrada, se procesa una vez y se copia a los dos canales.
    for (int start = 0; start < total; start += chunk)
    {
        const int n = juce::jmin (chunk, total - start);
        for (int i = 0; i < n; ++i)
            mono[(size_t) i] = 0.5f * (l[start + i] + r[start + i]);

        engine.process (mono.data(), n);

        jah::FxParams fpc = fp;
        if (fpc.hasPpq && fsHost > 0.0) fpc.ppq += (double) start / fsHost * fp.bpm / 60.0;
        fx.process (mono.data(), l + start, r + start, n, fpc);

        for (int i = 0; i < n; ++i)
        {
            const float g = master.getNextValue();
            l[start + i] = sanitize (l[start + i] * g);
            r[start + i] = sanitize (r[start + i] * g);
        }
    }
}

void JustAHeadProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void JustAHeadProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml.get() != nullptr)
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* JustAHeadProcessor::createEditor()
{
    return new JustAHeadEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JustAHeadProcessor();
}
