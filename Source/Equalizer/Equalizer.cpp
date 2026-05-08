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

float Equalizer::Biquad::process (float in)
{
    float out = b0 * in + z1;
    z1 = b1 * in + z2 - a1 * out;
    z2 = b2 * in - a2 * out;
    return out;
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
                s = b.process (s);
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
// Coefficient dispatch
// ---------------------------------------------------------------------------
void Equalizer::calcByType (Biquad& bq, const BandCache& bc)
{
    switch (bc.type)
    {
        case BandType::Lowpass:   calcLowpass   (bq, bc.f, bc.q);       break;
        case BandType::Highpass:  calcHighpass  (bq, bc.f, bc.q);       break;
        case BandType::Peak:      calcPeaking   (bq, bc.f, bc.q, bc.g); break;
        case BandType::Lowshelf:  calcLowShelf  (bq, bc.f, bc.g);       break;
        case BandType::Highshelf: calcHighShelf (bq, bc.f, bc.g);       break;
    }
}

// RBJ Cookbook formulas.
void Equalizer::calcLowpass (Biquad& bq, float f, float q)
{
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha  = sin_w0 / (2.0f * q);

    float b0 = (1.0f - cos_w0) * 0.5f;
    float b1 =  1.0f - cos_w0;
    float b2 = (1.0f - cos_w0) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}

void Equalizer::calcHighpass (Biquad& bq, float f, float q)
{
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha  = sin_w0 / (2.0f * q);

    float b0 =  (1.0f + cos_w0) * 0.5f;
    float b1 = -(1.0f + cos_w0);
    float b2 =  (1.0f + cos_w0) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}

void Equalizer::calcPeaking (Biquad& bq, float f, float q, float g)
{
    float A = std::pow (10.0f, g / 40.0f);
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha = sin_w0 / (2.0f * q);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cos_w0;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha / A;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}

void Equalizer::calcLowShelf (Biquad& bq, float f, float g)
{
    float A = std::pow (10.0f, g / 40.0f);
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha = sin_w0 / 2.0f * std::sqrt ((A + 1.0f / A) * (1.0f / 1.0f - 1.0f) + 2.0f); // S = 1

    float b0 =    A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha);
    float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    float b2 =    A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha);
    float a0 =        (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha;
    float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    float a2 =        (A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}

void Equalizer::calcHighShelf (Biquad& bq, float f, float g)
{
    float A = std::pow (10.0f, g / 40.0f);
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha = sin_w0 / 2.0f * std::sqrt ((A + 1.0f / A) * (1.0f / 1.0f - 1.0f) + 2.0f); // S = 1

    float b0 =    A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha);
    float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    float b2 =    A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha);
    float a0 =        (A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha;
    float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    float a2 =        (A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}
