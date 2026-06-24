/*
  ==============================================================================

    Equalizer.cpp

  ==============================================================================
*/

#include "Equalizer.h"

const Equalizer::BandConfig& Equalizer::getBandConfig (int i) noexcept
{
    static const BandConfig configs[NumBands] = {
        { "LS",   20.0f,  20000.0f,  100.0f, BandType::Lowshelf  },
        { "B1",  20.0f,  20000.0f,  500.0f, BandType::Peak      },
        { "B2",  20.0f, 20000.0f, 2000.0f, BandType::Peak      },
        { "B3",  20.0f, 20000.0f, 3500.0f, BandType::Peak      },
        { "HS", 20.0f, 20000.0f, 5000.0f, BandType::Highshelf },
    };
    return configs[i];
}

juce::StringArray Equalizer::getBandTypeNames()
{
    return { "Low Pass", "High Pass", "Peak", "Low Shelf", "High Shelf" };
}

Equalizer::Equalizer()
{
    for (int i = 0; i < NumBands; ++i)
    {
        const auto& cfg = getBandConfig (i);
        bandCache[i].type = cfg.defType;
        bandCache[i].f    = cfg.defFreq;
        bandCache[i].q    = 1.0f;
        bandCache[i].g    = 0.0f;
    }
}

void Equalizer::prepare (double sampleRate, int /*numChannels*/)
{
    currentSampleRate = sampleRate;
    const int maxChannels = 4; // Pre-allocate to be RT-safe.
    channels.resize (maxChannels);

    for (auto& ch : channels)
        for (auto& b : ch.band)
            b.reset();

    updateCoefficients();
}

void Equalizer::setOn (bool shouldBeOn) { on = shouldBeOn; }
bool Equalizer::isOn() const            { return on; }

void Equalizer::updateCoefficients()
{
    for (auto& ch : channels)
        for (int i = 0; i < NumBands; ++i)
            calcByType (ch.band[i], bandCache[i]);
}

void Equalizer::process (juce::AudioBuffer<float>& buffer)
{
    if (! on) return;

    int numChannels = buffer.getNumChannels();
    int numSamples  = buffer.getNumSamples();

    if (numChannels > (int) channels.size()) return;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data  = buffer.getWritePointer (ch);
        auto& strip = channels[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float s = data[i];
            for (auto& b : strip.band)
                s = b.processSample (s);
            s *= postGain;
            data[i] = s;
        }
    }
}

void Equalizer::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam       = apvts.getRawParameterValue (prefix + "_EQ_On");
    postGainParam = apvts.getRawParameterValue (prefix + "_EQ_PostGain");

    for (int i = 0; i < NumBands; ++i)
    {
        const auto& cfg   = getBandConfig (i);
        juce::String pid  = prefix + "_EQ_" + cfg.suffix;
        bandParams[i].type = apvts.getRawParameterValue (pid + "_Type");
        bandParams[i].freq = apvts.getRawParameterValue (pid + "_Freq");
        bandParams[i].q    = apvts.getRawParameterValue (pid + "_Q");
        bandParams[i].gain = apvts.getRawParameterValue (pid + "_Gain");
    }
}

void Equalizer::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool>  (juce::ParameterID { prefix + "_EQ_On",       1 }, prefix + " EQ On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_EQ_PostGain", 1 }, prefix + " EQ Post Gain", -24.0f, 24.0f, 0.0f));

    juce::StringArray typeNames = getBandTypeNames();

    for (int i = 0; i < NumBands; ++i)
    {
        const auto& cfg = getBandConfig (i);
        juce::String pid  = prefix + "_EQ_" + cfg.suffix;
        juce::String name = prefix + " EQ " + cfg.suffix;

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { pid + "_Type", 1 }, name + " Type", typeNames, (int) cfg.defType));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { pid + "_Freq", 1 }, name + " Freq", cfg.minFreq, cfg.maxFreq, cfg.defFreq));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { pid + "_Q",    1 }, name + " Q",    0.1f, 10.0f, 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { pid + "_Gain", 1 }, name + " Gain", -24.0f, 24.0f, 0.0f));
    }
}

void Equalizer::checkParameters()
{
    if (onParam && *onParam != lastOn)
    {
        setOn (*onParam > 0.5f);
        lastOn = *onParam;
    }

    if (postGainParam && *postGainParam != lastPostGain)
    {
        postGain = juce::Decibels::decibelsToGain (postGainParam->load());
        lastPostGain = *postGainParam;
    }

    bool changed = false;
    for (int i = 0; i < NumBands; ++i)
    {
        auto& p = bandParams[i];
        auto& l = bandLast[i];
        auto& c = bandCache[i];

        if (p.type && *p.type != l.type) { c.type = (BandType) (int) *p.type; l.type = *p.type; changed = true; }
        if (p.freq && *p.freq != l.freq) { c.f    = *p.freq;                  l.freq = *p.freq; changed = true; }
        if (p.q    && *p.q    != l.q)    { c.q    = *p.q;                     l.q    = *p.q;    changed = true; }
        if (p.gain && *p.gain != l.gain) { c.g    = *p.gain;                  l.gain = *p.gain; changed = true; }
    }

    if (changed)
        updateCoefficients();
}

// ---------------------------------------------------------------------------
// Coefficient dispatch — RBJ cookbook coefficients come from fxme::BiquadCoeffs
// (shelves use slope S = 1, matching the previous in-class implementation).
// ---------------------------------------------------------------------------
void Equalizer::calcByType (fxme::Biquad& bq, const BandCache& bc)
{
    switch (bc.type)
    {
        case BandType::Lowpass:   bq.c = fxme::BiquadCoeffs::lowpass   (currentSampleRate, bc.f, bc.q);       break;
        case BandType::Highpass:  bq.c = fxme::BiquadCoeffs::highpass  (currentSampleRate, bc.f, bc.q);       break;
        case BandType::Peak:      bq.c = fxme::BiquadCoeffs::peaking   (currentSampleRate, bc.f, bc.q, bc.g); break;
        case BandType::Lowshelf:  bq.c = fxme::BiquadCoeffs::lowShelf  (currentSampleRate, bc.f, bc.g);       break;
        case BandType::Highshelf: bq.c = fxme::BiquadCoeffs::highShelf (currentSampleRate, bc.f, bc.g);       break;
    }
}
