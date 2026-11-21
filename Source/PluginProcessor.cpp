#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
using Coeff = juce::dsp::IIR::Coefficients<float>;

static float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }

//==============================================================================
void HeadModel::reset (double sr, int maxSamples)
{
    oversampling.initProcessing ((size_t) maxSamples);
    oversampling.reset();
    inHP.reset(); lowS.reset(); midPk.reset(); highSh.reset(); pres.reset(); outLP.reset();
    updateVoicing (sr);
    setParams (18.0f, 5.0f, 5.0f, 5.0f, 5.0f, sr);
}

void HeadModel::setVoicing (Voicing v, double sr)
{
    if (v == voicing) return;
    voicing = v;
    updateVoicing (sr);
}

void HeadModel::updateVoicing (double sr)
{
    float hpf = 35.0f, lpf = 12000.0f;
    switch (voicing)
    {
        case Voicing::BritishOD:     hpf = 35.0f;  lpf = 12000.0f; break;
        case Voicing::Modern3Ch:     hpf = 25.0f;  lpf = 14000.0f; break;
        case Voicing::UKIommi:       hpf = 40.0f;  lpf = 10000.0f; break;
        case Voicing::AmericanThick: hpf = 30.0f;  lpf = 13000.0f; break;
    }
    inHP.setCoefficients (Coeff::makeHighPass (sr, hpf));
    outLP.setCoefficients (Coeff::makeLowPass (sr, lpf));
}

void HeadModel::setParams (float gainDb, float bass, float mid, float treble, float p, double sr)
{
    preGain = dbToGain (gainDb) * 1.15f;
    presenceDb = p;
    makeup = dbToGain (-6.0f);

    float bassF = 100.0f, midF = 820.0f, trebF = 3000.0f;
    switch (voicing)
    {
        case Voicing::BritishOD:     bassF = 100.0f; midF = 820.0f;  trebF = 3000.0f; break;
        case Voicing::Modern3Ch:     bassF = 80.0f;  midF = 900.0f;  trebF = 3200.0f; break;
        case Voicing::UKIommi:       bassF = 110.0f; midF = 700.0f;  trebF = 2600.0f; break;
        case Voicing::AmericanThick: bassF = 90.0f;  midF = 750.0f;  trebF = 3000.0f; break;
    }

    lowS.setCoefficients  (Coeff::makeLowShelf  (sr, bassF, 0.707f, dbToGain (bass)));
    midPk.setCoefficients (Coeff::makePeakFilter (sr, midF, 1.0f, dbToGain (mid)));
    highSh.setCoefficients(Coeff::makeHighShelf (sr, trebF, 0.707f, dbToGain (treble)));
    pres.setCoefficients  (Coeff::makeHighShelf (sr, 6000.0f, 0.707f, dbToGain (p)));
}

void HeadModel::process (juce::dsp::AudioBlock<float>& block) noexcept
{
    const size_t n = block.getNumSamples();
    if (n == 0) return;

    juce::HeapBlock<char> heap;
    auto tmp = juce::dsp::AudioBlock<float> (heap, 2, n);
    tmp.copyFrom (block);

    inHP.process (tmp);
    forEachSample (tmp, [this] (float x) { return shapingCurve (x * preGain, 1.0f); });

    auto up = oversampling.processSamplesUp (tmp);
    const float drive = voicing == Voicing::Modern3Ch ? 1.45f :
                        voicing == Voicing::UKIommi ? 1.15f : 1.25f;
    forEachSample (up, [drive] (float x) { return shapingCurve (x, drive); });
    oversampling.processSamplesDown (tmp);

    lowS.process (tmp);
    midPk.process (tmp);
    highSh.process (tmp);
    pres.process (tmp);
    outLP.process (tmp);
    forEachSample (tmp, [this] (float x) { return x * makeup; });

    block.copyFrom (tmp);
}

//==============================================================================
void PedalModel::reset (double sr, int maxSamples)
{
    oversampling.initProcessing ((size_t) maxSamples);
    oversampling.reset();
    inputHP.reset(); toneLP.reset(); bodyHP.reset();
    setParams (Type::Off, 12.0f, 5.0f, 0.0f, sr);
}

void PedalModel::setParams (Type t, float driveDb, float tone, float levelDb, double sr)
{
    type = t;
    active = (t != Type::Off);
    preGain = dbToGain (driveDb);
    level = dbToGain (levelDb);

    inputHP.setCoefficients (Coeff::makeHighPass (sr, t == Type::Distortion ? 90.0f : 40.0f));
    toneLP.setCoefficients  (Coeff::makeLowPass  (sr, 1500.0f + tone * 650.0f));
    bodyHP.setCoefficients  (Coeff::makeHighPass (sr, t == Type::Distortion ? 200.0f : 120.0f));
}

void PedalModel::process (juce::dsp::AudioBlock<float>& block) noexcept
{
    if (! active) return;

    const size_t n = block.getNumSamples();
    juce::HeapBlock<char> heap;
    auto tmp = juce::dsp::AudioBlock<float> (heap, 2, n);
    tmp.copyFrom (block);

    inputHP.process (tmp);
    forEachSample (tmp, [this] (float x) { return x * preGain; });

    auto up = oversampling.processSamplesUp (tmp);
    const float drive = type == Type::Distortion ? 2.6f : 1.5f;
    forEachSample (up, [drive] (float x) { return shapingCurve (x, drive); });
    oversampling.processSamplesDown (tmp);

    toneLP.process (tmp);
    bodyHP.process (tmp);
    forEachSample (tmp, [this] (float x) { return x * level; });

    block.copyFrom (tmp);
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

    addFloat ("gain",     "Gain",     0.0f,  30.0f, 18.0f);
    addFloat ("bass",     "Bass",     0.0f,  10.0f,  5.0f);
    addFloat ("mid",      "Mid",      0.0f,  10.0f,  5.0f);
    addFloat ("treble",   "Treble",   0.0f,  10.0f,  5.0f);
    addFloat ("presence", "Presence", 0.0f,  10.0f,  5.0f);
    addFloat ("drive",    "Drive",    0.0f,  24.0f, 12.0f);
    addFloat ("ptone",    "Pedal Tone", 0.0f, 10.0f, 5.0f);
    addFloat ("level",    "Pedal Level", -24.0f, 12.0f, 0.0f);
    addFloat ("master",   "Master",   -24.0f, 12.0f, -6.0f);

    return layout;
}

//==============================================================================
JustAHeadProcessor::JustAHeadProcessor()
    : apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

void JustAHeadProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    head.reset (sr, samplesPerBlock);
    pedal.reset (sr, samplesPerBlock);
}

bool JustAHeadProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void JustAHeadProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n <= 0) return;

    auto choice = [this] (const char* id) -> int   { return (int) apvts.getRawParameterValue (id)->load(); };
    auto param  = [this] (const char* id) -> float { return apvts.getRawParameterValue (id)->load(); };

    head.setVoicing ((HeadModel::Voicing) choice ("head"), getSampleRate());
    head.setParams (param ("gain"), param ("bass"), param ("mid"), param ("treble"), param ("presence"), getSampleRate());
    pedal.setParams ((PedalModel::Type) choice ("pedal"), param ("drive"), param ("ptone"), param ("level"), getSampleRate());

    auto block = juce::dsp::AudioBlock<float> (buffer);
    pedal.process (block);
    head.process (block);

    const float master = dbToGain (param ("master"));
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            d[i] = juce::jlimit (-1.0f, 1.0f, d[i] * master);
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
