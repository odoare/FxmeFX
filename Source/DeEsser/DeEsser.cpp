/*
  ==============================================================================

    DeEsser.cpp

  ==============================================================================
*/

#include "DeEsser.h"
#include <FxmeTools/dsp/Sanitise.h>
#include <cmath>

namespace
{
    constexpr double kOnFadeSeconds  = 0.02;
    constexpr float  kRmsSeconds     = 0.002f;
    constexpr float  kRatio          = 6.0f;
    constexpr float  kKneeDb         = 6.0f;
    constexpr float  kShelfQ         = 0.7071f;
    constexpr double kButterworthK   = 1.4142135623730951;
    constexpr float  kQuietFullDb    = -90.0f;   // relative mode: under this, nothing to compare

    // Speed 0 .. 1: attack 5 ms -> 0.2 ms, release 150 ms -> 20 ms (log).
    float attackSeconds (float s)  { return 0.005f * std::pow (0.2f / 5.0f, s); }
    float releaseSeconds (float s) { return 0.150f * std::pow (20.0f / 150.0f, s); }
}

void DeEsser::prepare (double sampleRate, int numChannels)
{
    juce::ignoreUnused (numChannels);
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    for (auto& f : detectorFilter)
        f.reset();
    bandDetector.prepare (currentSampleRate);
    fullDetector.prepare (currentSampleRate);
    for (auto* d : { &bandDetector, &fullDetector })
    {
        d->setMode (fxme::LevelDetector::Mode::rms);
        d->setRmsWindow (kRmsSeconds);
    }
    ballistics.prepare (currentSampleRate);
    bandEq.prepare (currentSampleRate);
    updateFilters (true);
    updateComputer();
    updateTimes();

    onSmooth.reset (currentSampleRate, kOnFadeSeconds);
    onSmooth.setCurrentAndTargetValue (on ? 1.0f : 0.0f);
}

void DeEsser::updateFilters (bool snap)
{
    const double g = fxme::TptSvf::gFor (freqHz, currentSampleRate);
    const double k = filter == Filter::bandPass ? 1.0 / juce::jmax (0.1f, q) : kButterworthK;
    for (auto& f : detectorFilter)
        f.setup (g, k);

    bandEq.setShape (filter == Filter::bandPass ? fxme::RegaliaMitraEq::Shape::bell
                                                : fxme::RegaliaMitraEq::Shape::highShelf);
    bandEq.setFrequencyAndQ (freqHz, filter == Filter::bandPass ? q : kShelfQ, snap);
}

void DeEsser::updateComputer()
{
    computer.set (fxme::DynamicsMode::compress, thresholdDb, kRatio, kKneeDb, rangeDb);
}

void DeEsser::updateTimes()
{
    ballistics.setTimes (attackSeconds (speed01), releaseSeconds (speed01));
}

void DeEsser::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                             const juce::String& prefix)
{
    using Range = juce::NormalisableRange<float>;
    auto id = [&] (const char* s) { return juce::ParameterID { prefix + "_DeEss_" + s, 1 }; };
    auto name = [&] (const char* s) { return prefix + " DeEss " + s; };

    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("On"), name ("On"), true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("Mode"), name ("Mode"),
                          juce::StringArray { "Wideband", "Split band" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("Filter"), name ("Filter"),
                          juce::StringArray { "Band-pass", "High-pass" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Freq"), name ("Frequency"),
                          Range (kMinFreqHz, kMaxFreqHz, 1.0f, 0.5f), 6000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Q"), name ("Q"),
                          Range (0.5f, 8.0f, 0.01f, 0.5f), 1.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Thresh"), name ("Threshold"),
                          Range (kMinLevelDb, 0.0f, 0.1f), -30.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Range"), name ("Range"),
                          Range (-kMaxRangeDb, 0.0f, 0.1f), -12.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Speed"), name ("Speed"),
                          Range (0.0f, 100.0f, 0.1f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("Relative"), name ("Relative"), false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("Stereo"), name ("Stereo"),
                          juce::StringArray { "Linked", "Mid" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("Listen"), name ("Listen"), false));
}

void DeEsser::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    auto p = [&] (const char* id) { return apvts.getRawParameterValue (prefix + "_DeEss_" + id); };
    onParam       = p ("On");
    modeParam     = p ("Mode");
    filterParam   = p ("Filter");
    freqParam     = p ("Freq");
    qParam        = p ("Q");
    threshParam   = p ("Thresh");
    rangeParam    = p ("Range");
    speedParam    = p ("Speed");
    relativeParam = p ("Relative");
    stereoParam   = p ("Stereo");
    listenParam   = p ("Listen");
}

void DeEsser::checkParameters()
{
    // Reads one parameter into `value`; true when it changed.
    auto read = [] (std::atomic<float>* param, float& value)
    {
        if (param == nullptr)
            return false;
        const float v = param->load();
        if (v == value)
            return false;
        value = v;
        return true;
    };
    auto readBool = [] (std::atomic<float>* param, bool& value)
    {
        if (param == nullptr || (param->load() > 0.5f) == value)
            return false;
        value = ! value;
        return true;
    };

    if (readBool (onParam, on))
        onSmooth.setTargetValue (on ? 1.0f : 0.0f);
    readBool (relativeParam, relative);
    readBool (listenParam, listen);

    if (modeParam != nullptr)
        mode = modeParam->load() > 0.5f ? Mode::splitBand : Mode::wideband;
    if (stereoParam != nullptr)
        stereo = stereoParam->load() > 0.5f ? Stereo::mid : Stereo::linked;

    bool filtersChanged = false;
    if (filterParam != nullptr)
    {
        const auto f = filterParam->load() > 0.5f ? Filter::highPass : Filter::bandPass;
        if (f != filter) { filter = f; filtersChanged = true; }
    }
    if (read (freqParam, freqHz)) filtersChanged = true;
    if (read (qParam, q))         filtersChanged = true;
    if (filtersChanged)
        updateFilters (false);

    const bool threshChanged = read (threshParam, thresholdDb);
    float rangeNeg = -rangeDb;
    if (read (rangeParam, rangeNeg))
    {
        rangeDb = -rangeNeg;
        updateComputer();
    }
    else if (threshChanged)
        updateComputer();

    float speedPct = speed01 * 100.0f;
    if (read (speedParam, speedPct))
    {
        speed01 = speedPct * 0.01f;
        updateTimes();
    }
}

void DeEsser::pushSum (fxme::SpectrumTap& tap, const juce::AudioBuffer<float>& buffer, int numChannels) noexcept
{
    if (! tap.isEnabled())
        return;

    constexpr int chunk = 256;
    float mono[chunk];
    const int numSamples = buffer.getNumSamples();
    const float scale = 1.0f / (float) numChannels;

    for (int start = 0; start < numSamples; start += chunk)
    {
        const int n = juce::jmin (chunk, numSamples - start);
        for (int i = 0; i < n; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                s += buffer.getReadPointer (ch)[start + i];
            mono[i] = s * scale;
        }
        tap.push (mono, n);
    }
}

void DeEsser::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    if (numChannels <= 0 || numSamples <= 0)
        return;

    // The detector filters, the RMS averages and the band EQ hold state:
    // keep NaN and infinities out of them.
    fxme::sanitiseBlock (buffer.getArrayOfWritePointers(), numChannels, numSamples, fxme::kMainPathLimit);
    pushSum (inputTap, buffer, numChannels);

    if (! on && ! onSmooth.isSmoothing())
    {
        ballistics.reset();
        displayLevel.store (kMinLevelDb, std::memory_order_relaxed);
        displayGain .store (0.0f, std::memory_order_relaxed);
        pushSum (outputTap, buffer, numChannels);
        return;
    }

    auto* const* data = buffer.getArrayOfWritePointers();
    const bool stereoPair = numChannels == 2;
    const bool midOnly = stereo == Stereo::mid && stereoPair;
    float blockLevelDb = fxme::LevelDetector::floorDb;

    for (int i = 0; i < numSamples; ++i)
    {
        const float xl = data[0][i];
        const float xr = stereoPair ? data[1][i] : xl;
        const float mid = 0.5f * (xl + xr), side = 0.5f * (xl - xr);

        // Detector: the band's level (and the whole signal's, for Relative).
        float bl, br, levelDb, fullDb;
        auto bandOf = [this] (int ch, float x)
        {
            auto& f = detectorFilter[(size_t) ch];
            return filter == Filter::bandPass ? f.bandPass (x) : f.highPass (x);
        };
        if (midOnly || ! stereoPair)
        {
            const float x = midOnly ? mid : xl;
            bl = br = bandOf (0, x);
            levelDb = bandDetector.process (bl);
            fullDb  = relative ? fullDetector.process (x) : 0.0f;
        }
        else
        {
            bl = bandOf (0, xl);
            br = bandOf (1, xr);
            levelDb = bandDetector.processLinked (bl, br);
            fullDb  = relative ? fullDetector.processLinked (xl, xr) : 0.0f;
        }
        if (relative)
            levelDb = fullDb > kQuietFullDb ? levelDb - fullDb : fxme::LevelDetector::floorDb;
        blockLevelDb = juce::jmax (blockLevelDb, levelDb);

        const float gainDb = ballistics.process (computer.gainDb (levelDb));
        const float gain = std::exp (gainDb * 0.11512925465f);   // ln (10) / 20
        bandEq.advance (gain);

        float yl, yr;
        if (listen)
        {
            yl = bl;
            yr = br;
        }
        else if (mode == Mode::wideband)
        {
            if (midOnly) { const float m = mid * gain; yl = m + side; yr = m - side; }
            else         { yl = xl * gain; yr = xr * gain; }
        }
        else
        {
            if (midOnly) { const float m = bandEq.process (0, mid); yl = m + side; yr = m - side; }
            else         { yl = bandEq.process (0, xl); yr = stereoPair ? bandEq.process (1, xr) : yl; }
        }

        const float onMix = onSmooth.getNextValue();
        data[0][i] = xl + onMix * (yl - xl);
        if (stereoPair)
            data[1][i] = xr + onMix * (yr - xr);
    }

    displayLevel.store (blockLevelDb, std::memory_order_relaxed);
    displayGain .store (ballistics.getCurrentDb(), std::memory_order_relaxed);
    pushSum (outputTap, buffer, numChannels);
}
