/*
  ==============================================================================

    Equalizer.cpp

  ==============================================================================
*/

#include "Equalizer.h"

Equalizer::BandConfig Equalizer::getBandConfig (int i, int numBands) noexcept
{
    numBands = juce::jlimit (2, MaxBands, numBands);
    i        = juce::jlimit (0, numBands - 1, i);

    BandConfig cfg;
    cfg.minFreq = 20.0f;
    cfg.maxFreq = 20000.0f;

    // Band 0 is a low shelf, the last band a high shelf, the rest peaks. The
    // suffixes (APVTS ID stems) must stay stable for preset back-compat, so the
    // peaks keep the historical "B1", "B2", … numbering by band index.
    if (i == 0)
    {
        cfg.suffix  = "LS";
        cfg.defType = BandType::Lowshelf;
    }
    else if (i == numBands - 1)
    {
        cfg.suffix  = "HS";
        cfg.defType = BandType::Highshelf;
    }
    else
    {
        cfg.suffix  = "B" + juce::String (i);
        cfg.defType = BandType::Peak;
    }

    // Default centre frequency. The historical 5-band layout is reproduced
    // exactly; any other band count is spread logarithmically over 100 Hz–8 kHz.
    if (numBands == 5)
    {
        static const float legacy[5] = { 100.0f, 500.0f, 2000.0f, 3500.0f, 5000.0f };
        cfg.defFreq = legacy[i];
    }
    else
    {
        const float lo = 100.0f, hi = 8000.0f;
        cfg.defFreq = lo * std::pow (hi / lo, (float) i / (float) (numBands - 1));
    }

    return cfg;
}

juce::StringArray Equalizer::getBandTypeNames()
{
    return { "Low Pass", "High Pass", "Peak", "Low Shelf", "High Shelf" };
}

Equalizer::Equalizer (int numBandsToUse, bool enableSpectrum)
    : numBands (juce::jlimit (2, MaxBands, numBandsToUse)),
      spectrumEnabled (enableSpectrum)
{
    for (int i = 0; i < numBands; ++i)
    {
        const auto cfg = getBandConfig (i, numBands);
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

    inputTap .setEnabled (spectrumEnabled);
    outputTap.setEnabled (spectrumEnabled);

    updateCoefficients();
}

void Equalizer::setOn (bool shouldBeOn) { on = shouldBeOn; }
bool Equalizer::isOn() const            { return on; }

void Equalizer::updateCoefficients()
{
    for (auto& ch : channels)
        for (int i = 0; i < numBands; ++i)
            calcByType (ch.band[i], bandCache[i]);
}

void Equalizer::process (juce::AudioBuffer<float>& buffer)
{
    // Feed the pre-EQ tap first, so it captures the untouched input even when
    // the EQ is bypassed (in which case output == input).
    if (spectrumEnabled)
        pushSum (inputTap, buffer);

    int numChannels = buffer.getNumChannels();
    int numSamples  = buffer.getNumSamples();

    if (on && numChannels <= (int) channels.size())
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data  = buffer.getWritePointer (ch);
            auto& strip = channels[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                float s = data[i];
                for (int bi = 0; bi < numBands; ++bi)
                    s = strip.band[bi].processSample (s);
                s *= postGain;
                data[i] = s;
            }
        }
    }

    if (spectrumEnabled)
        pushSum (outputTap, buffer);
}

void Equalizer::pushSum (fxme::SpectrumTap& tap, const juce::AudioBuffer<float>& buffer) noexcept
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    if (numChannels <= 0) return;

    // Sum the stereo (or N) channels into a mono signal, pushed in fixed chunks
    // so no allocation happens on the audio thread.
    constexpr int chunk = 256;
    float mono[chunk];

    for (int start = 0; start < numSamples; start += chunk)
    {
        const int n = juce::jmin (chunk, numSamples - start);
        for (int i = 0; i < n; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                s += buffer.getReadPointer (ch)[start + i];
            mono[i] = s;
        }
        tap.push (mono, n);
    }
}

void Equalizer::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam       = apvts.getRawParameterValue (prefix + "_EQ_On");
    postGainParam = apvts.getRawParameterValue (prefix + "_EQ_PostGain");

    for (int i = 0; i < numBands; ++i)
    {
        const auto cfg    = getBandConfig (i, numBands);
        juce::String pid  = prefix + "_EQ_" + cfg.suffix;
        bandParams[i].on   = apvts.getRawParameterValue (pid + "_On");
        bandParams[i].type = apvts.getRawParameterValue (pid + "_Type");
        bandParams[i].freq = apvts.getRawParameterValue (pid + "_Freq");
        bandParams[i].q    = apvts.getRawParameterValue (pid + "_Q");
        bandParams[i].gain = apvts.getRawParameterValue (pid + "_Gain");
    }
}

void Equalizer::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix, int numBands)
{
    numBands = juce::jlimit (2, MaxBands, numBands);

    params.push_back (std::make_unique<juce::AudioParameterBool>  (juce::ParameterID { prefix + "_EQ_On",       1 }, prefix + " EQ On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_EQ_PostGain", 1 }, prefix + " EQ Post Gain", -24.0f, 24.0f, 0.0f));

    juce::StringArray typeNames = getBandTypeNames();

    for (int i = 0; i < numBands; ++i)
    {
        const auto cfg    = getBandConfig (i, numBands);
        juce::String pid  = prefix + "_EQ_" + cfg.suffix;
        juce::String name = prefix + " EQ " + cfg.suffix;

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { pid + "_On", 1 }, name + " On", true));
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
    for (int i = 0; i < numBands; ++i)
    {
        auto& p = bandParams[i];
        auto& l = bandLast[i];
        auto& c = bandCache[i];

        if (p.on   && *p.on   != l.on)   { c.on   = (*p.on > 0.5f);           l.on   = *p.on;   changed = true; }
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
    if (! bc.on)
    {
        bq.c = fxme::BiquadCoeffs {}; // identity (pass-through) when band disabled
        return;
    }

    switch (bc.type)
    {
        case BandType::Lowpass:   bq.c = fxme::BiquadCoeffs::lowpass   (currentSampleRate, bc.f, bc.q);       break;
        case BandType::Highpass:  bq.c = fxme::BiquadCoeffs::highpass  (currentSampleRate, bc.f, bc.q);       break;
        case BandType::Peak:      bq.c = fxme::BiquadCoeffs::peaking   (currentSampleRate, bc.f, bc.q, bc.g); break;
        case BandType::Lowshelf:  bq.c = fxme::BiquadCoeffs::lowShelf  (currentSampleRate, bc.f, bc.g);       break;
        case BandType::Highshelf: bq.c = fxme::BiquadCoeffs::highShelf (currentSampleRate, bc.f, bc.g);       break;
    }
}
