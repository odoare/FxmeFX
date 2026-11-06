/*
  ==============================================================================

    Gate.cpp

  ==============================================================================
*/

#include "Gate.h"
#include <FxmeTools/dsp/Sanitise.h>
#include <cmath>

namespace
{
    constexpr double kOnFadeSeconds   = 0.02;
    constexpr double kOutSmoothSeconds = 0.05;
    constexpr float  kPeakFallSeconds = 0.010f;   // the detector envelope's fall
    constexpr double kButterworthK    = 1.4142135623730951;

    float timeCoef (float seconds, double sampleRate)
    {
        return seconds <= 0.0f ? 0.0f : (float) std::exp (-1.0 / ((double) seconds * sampleRate));
    }
}

void Gate::prepare (double sampleRate, int numChannels)
{
    currentSampleRate   = sampleRate > 0.0 ? sampleRate : 44100.0;
    numChannelsPrepared = juce::jlimit (1, 2, numChannels);

    delaySize = (int) std::ceil (kMaxLookaheadMs * 0.001 * currentSampleRate) + 2;
    for (auto& d : delay)
        d.assign ((size_t) delaySize, 0.0f);
    writePos = 0;
    lookaheadSamples = juce::jlimit (0, delaySize - 1, juce::roundToInt (lookaheadMs * 0.001 * currentSampleRate));
    latencySamples = lookaheadSamples;

    for (auto& f : scHighPass) f.reset();
    for (auto& f : scLowPass)  f.reset();
    updateFilters();

    stateMachine.prepare (currentSampleRate);
    stateMachine.setThresholds (thresholdDb, hysteresisDb);
    stateMachine.setHold (holdMs * 0.001f);
    updateComputer();
    updateTimes();

    peakEnv = 0.0f;
    gainDb  = 0.0f;

    onSmooth .reset (currentSampleRate, kOnFadeSeconds);
    outSmooth.reset (currentSampleRate, kOutSmoothSeconds);
    onSmooth .setCurrentAndTargetValue (on ? 1.0f : 0.0f);
    outSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outDb));
}

void Gate::updateFilters()
{
    for (auto& f : scHighPass) f.setup (fxme::TptSvf::gFor (hpHz, currentSampleRate), kButterworthK);
    for (auto& f : scLowPass)  f.setup (fxme::TptSvf::gFor (lpHz, currentSampleRate), kButterworthK);
}

void Gate::updateTimes()
{
    attackCoef  = timeCoef (attackMs * 0.001f, currentSampleRate);
    releaseCoef = timeCoef (releaseMs * 0.001f, currentSampleRate);
    peakFall    = timeCoef (kPeakFallSeconds, currentSampleRate);
}

void Gate::updateComputer()
{
    computer.set (fxme::DynamicsMode::expand, thresholdDb,
                  mode == Mode::gate ? fxme::GainComputer::infiniteRatio : ratio,
                  0.0f, rangeDb);
}

void Gate::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                          const juce::String& prefix)
{
    using Range = juce::NormalisableRange<float>;
    auto addFloat = [&] (const char* id, const char* name, Range range, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "_Gate_" + id, 1 }, prefix + " Gate " + name, range, def));
    };

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Gate_On", 1 }, prefix + " Gate On", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Gate_Mode", 1 }, prefix + " Gate Mode",
        juce::StringArray { "Gate", "Expander" }, 0));

    addFloat ("Thresh",    "Threshold",  Range (kMinLevelDb, 0.0f, 0.1f),          -40.0f);
    addFloat ("Hyst",      "Hysteresis", Range (0.0f, 20.0f, 0.1f),                  4.0f);
    addFloat ("Range",     "Range",      Range (kMinLevelDb, 0.0f, 0.1f),          -80.0f);
    addFloat ("Ratio",     "Ratio",      Range (1.1f, 10.0f, 0.01f, 0.5f),           2.0f);
    addFloat ("Attack",    "Attack",     Range (0.01f, 50.0f, 0.01f, 0.3f),          0.5f);
    addFloat ("Hold",      "Hold",       Range (0.0f, 500.0f, 0.1f, 0.5f),          20.0f);
    addFloat ("Release",   "Release",    Range (5.0f, 2000.0f, 0.1f, 0.3f),        150.0f);
    addFloat ("HighPass",  "SC High-pass", Range (20.0f, 2000.0f, 1.0f, 0.3f),      20.0f);
    addFloat ("LowPass",   "SC Low-pass",  Range (500.0f, 20000.0f, 1.0f, 0.3f), 20000.0f);

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Gate_Listen", 1 }, prefix + " Gate Listen", false));

    addFloat ("Lookahead", "Look-ahead", Range (0.0f, kMaxLookaheadMs, 0.01f),       0.0f);
    addFloat ("Out",       "Output",     Range (-24.0f, 24.0f, 0.1f),                0.0f);
}

void Gate::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    auto p = [&] (const char* id) { return apvts.getRawParameterValue (prefix + "_Gate_" + id); };
    onParam        = p ("On");
    modeParam      = p ("Mode");
    threshParam    = p ("Thresh");
    hystParam      = p ("Hyst");
    rangeParam     = p ("Range");
    ratioParam     = p ("Ratio");
    attackParam    = p ("Attack");
    holdParam      = p ("Hold");
    releaseParam   = p ("Release");
    hpParam        = p ("HighPass");
    lpParam        = p ("LowPass");
    listenParam    = p ("Listen");
    lookaheadParam = p ("Lookahead");
    outParam       = p ("Out");
}

void Gate::checkParameters()
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

    float onF = on ? 1.0f : 0.0f;
    if (read (onParam, onF))
    {
        on = onF > 0.5f;
        onSmooth.setTargetValue (on ? 1.0f : 0.0f);
    }

    float listenF = listen ? 1.0f : 0.0f;
    if (read (listenParam, listenF))
        listen = listenF > 0.5f;

    bool computerChanged = false;
    float modeF = mode == Mode::expander ? 1.0f : 0.0f;
    if (read (modeParam, modeF))
    {
        mode = modeF > 0.5f ? Mode::expander : Mode::gate;
        computerChanged = true;
    }
    float rangeNeg = -rangeDb;
    if (read (rangeParam, rangeNeg)) { rangeDb = -rangeNeg; computerChanged = true; }
    if (read (ratioParam, ratio))    computerChanged = true;

    bool thresholdsChanged = false;
    if (read (threshParam, thresholdDb))  { thresholdsChanged = true; computerChanged = true; }
    if (read (hystParam, hysteresisDb))   thresholdsChanged = true;
    if (thresholdsChanged)
        stateMachine.setThresholds (thresholdDb, hysteresisDb);
    if (computerChanged)
        updateComputer();

    if (read (holdParam, holdMs))
        stateMachine.setHold (holdMs * 0.001f);

    const bool attackChanged = read (attackParam, attackMs);
    if (read (releaseParam, releaseMs) || attackChanged)
        updateTimes();

    const bool hpChanged = read (hpParam, hpHz);
    if (read (lpParam, lpHz) || hpChanged)
        updateFilters();

    if (read (lookaheadParam, lookaheadMs))
    {
        lookaheadSamples = juce::jlimit (0, delaySize - 1, juce::roundToInt (lookaheadMs * 0.001 * currentSampleRate));
        latencySamples = lookaheadSamples;
    }

    if (read (outParam, outDb))
        outSmooth.setTargetValue (juce::Decibels::decibelsToGain (outDb));
}

void Gate::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), (int) delay.size());
    if (numChannels <= 0 || numSamples <= 0)
        return;

    // The sidechain filters and the detector hold state: keep NaN and
    // infinities out of them (and of the look-ahead line).
    fxme::sanitiseBlock (buffer.getArrayOfWritePointers(), numChannels, numSamples, fxme::kMainPathLimit);

    auto* const* data = buffer.getArrayOfWritePointers();

    // Off, and the fade is over: only the look-ahead delay, so the latency
    // stays what the host was told.
    if (! on && ! onSmooth.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const int readPos = (writePos - lookaheadSamples + delaySize) % delaySize;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                delay[(size_t) ch][(size_t) writePos] = data[ch][i];
                data[ch][i] = delay[(size_t) ch][(size_t) readPos];
            }
            writePos = (writePos + 1) % delaySize;
        }
        stateMachine.reset();
        peakEnv = 0.0f;
        gainDb  = 0.0f;
        displayLevel.store (kMinLevelDb, std::memory_order_relaxed);
        displayGain .store (0.0f, std::memory_order_relaxed);
        displayState.store (0.0f, std::memory_order_relaxed);
        return;
    }

    float blockLevelDb = fxme::LevelDetector::floorDb;
    std::array<float, 2> sc { 0.0f, 0.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        // Detector: the sidechain-filtered input, linked.
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float s = scLowPass[(size_t) ch].lowPass (scHighPass[(size_t) ch].highPass (data[ch][i]));
            sc[(size_t) ch] = s;
            peak = juce::jmax (peak, std::abs (s));
        }
        peakEnv = peak > peakEnv ? peak : peakEnv * peakFall;
        const float levelDb = peakEnv > 1.0e-7f ? 20.0f * std::log10 (peakEnv) : fxme::LevelDetector::floorDb;
        blockLevelDb = juce::jmax (blockLevelDb, levelDb);

        // Gain: 0 dB unless closed; attack opening, release closing.
        const auto state = stateMachine.process (levelDb);
        const float target = state == fxme::GateStateMachine::State::closed ? computer.gainDb (levelDb) : 0.0f;
        const float coef = target > gainDb ? attackCoef : releaseCoef;
        gainDb = target + coef * (gainDb - target);
        const float gain = std::exp (gainDb * 0.11512925465f);   // ln (10) / 20

        const float onMix = onSmooth.getNextValue();
        const float outG  = outSmooth.getNextValue();

        const int readPos = (writePos - lookaheadSamples + delaySize) % delaySize;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& line = delay[(size_t) ch];
            line[(size_t) writePos] = data[ch][i];
            const float dry = line[(size_t) readPos];
            const float wet = (listen ? sc[(size_t) ch] : dry * gain) * outG;
            data[ch][i] = dry + onMix * (wet - dry);
        }
        writePos = (writePos + 1) % delaySize;
    }

    displayLevel.store (blockLevelDb, std::memory_order_relaxed);
    displayGain .store (gainDb, std::memory_order_relaxed);
    displayState.store ((float) (int) stateMachine.getState(), std::memory_order_relaxed);
}
