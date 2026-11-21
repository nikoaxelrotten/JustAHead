#pragma once
#include <array>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/** One IIR filter per channel, so stereo channels never share state. */
class StereoFilter
{
public:
    using Coeff = juce::dsp::IIR::Coefficients<float>;

    StereoFilter (Coeff::Ptr c) { setCoefficients (c); }

    void setCoefficients (Coeff::Ptr c)
    {
        for (auto& f : filters)
            *f.coefficients = *c;
    }

    void reset()
    {
        for (auto& f : filters)
            f.reset();
    }

    void process (juce::dsp::AudioBlock<float>& block) noexcept
    {
        for (size_t ch = 0; ch < block.getNumChannels() && ch < filters.size(); ++ch)
        {
            auto sub = block.getSubBlock (ch, 1);
            juce::dsp::ProcessContextReplacing<float> ctx (sub);
            filters[ch].process (ctx);
        }
    }

private:
    std::array<juce::dsp::IIR::Filter<float>, 2> filters { Coeff::makeLowPass (48000.0, 1000.0f),
                                                           Coeff::makeLowPass (48000.0, 1000.0f) };
};

//==============================================================================
/** A smooth asymmetric soft-clipping curve. */
static inline float shapingCurve (float x, float drive) noexcept
{
    x = juce::jlimit (-8.0f, 8.0f, x * drive);
    const float a = 0.65f;
    return juce::jlimit (-1.0f, 1.0f, (1.0f + a) * x / (1.0f + a * juce::abs (x)));
}

template <typename Fn>
static void forEachSample (juce::dsp::AudioBlock<float>& b, Fn&& fn) noexcept
{
    for (size_t ch = 0; ch < b.getNumChannels(); ++ch)
    {
        auto* d = b.getChannelPointer (ch);
        for (size_t i = 0; i < b.getNumSamples(); ++i)
            d[i] = fn (d[i]);
    }
}

//==============================================================================
/** DSP approximation of a guitar preamp head (no cabinet IR, no wah). */
class HeadModel
{
public:
    enum class Voicing { BritishOD, Modern3Ch, UKIommi, AmericanThick };

    void reset (double sr, int maxSamples);
    void setVoicing (Voicing v, double sr);
    void setParams (float gainDb, float bass, float mid, float treble, float presence, double sr);
    void process (juce::dsp::AudioBlock<float>& block) noexcept;

private:
    using Coeff = juce::dsp::IIR::Coefficients<float>;

    Voicing voicing = Voicing::BritishOD;
    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false };

    StereoFilter inHP    { Coeff::makeHighPass (48000.0, 35.0f) };
    StereoFilter lowS    { Coeff::makeLowShelf (48000.0, 100.0f, 0.707f, 1.0f) };
    StereoFilter midPk   { Coeff::makePeakFilter (48000.0, 800.0f, 1.0f, 1.0f) };
    StereoFilter highSh  { Coeff::makeHighShelf (48000.0, 3000.0f, 0.707f, 1.0f) };
    StereoFilter pres    { Coeff::makeHighShelf (48000.0, 6000.0f, 0.707f, 1.0f) };
    StereoFilter outLP   { Coeff::makeLowPass (48000.0, 12000.0f) };

    float preGain = 1.0f;
    float makeup  = 1.0f;
    float presenceDb = 0.0f;

    void updateVoicing (double sr);
};

//==============================================================================
/** Internal overdrive / distortion pedal. */
class PedalModel
{
public:
    enum class Type { Off, Overdrive, Distortion };

    void reset (double sr, int maxSamples);
    void setParams (Type type, float drive, float tone, float level, double sr);
    void process (juce::dsp::AudioBlock<float>& block) noexcept;

private:
    using Coeff = juce::dsp::IIR::Coefficients<float>;

    Type type = Type::Off;
    bool active = false;
    float preGain = 1.0f;
    float level = 1.0f;

    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false };

    StereoFilter inputHP { Coeff::makeHighPass (48000.0, 40.0f) };
    StereoFilter toneLP  { Coeff::makeLowPass  (48000.0, 4000.0f) };
    StereoFilter bodyHP  { Coeff::makeHighPass (48000.0, 120.0f) };
};

//==============================================================================
class JustAHeadProcessor : public juce::AudioProcessor
{
public:
    JustAHeadProcessor();
    ~JustAHeadProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "JustAHead"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    HeadModel head;
    PedalModel pedal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustAHeadProcessor)
};
