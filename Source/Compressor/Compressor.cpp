/*
  ==============================================================================

    Compressor.cpp

  ==============================================================================
*/

#include "Compressor.h"
#include <FxmeTools/dsp/Sanitise.h>

Compressor::Compressor()
{
    updateCoefficients();
}

void Compressor::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    envelope = 0.0f;
    rmsSquare = 0.0f;
    lastGainReductiondB = 0.0f;
    stress = 0.0f;
    grMeter.prepare (sampleRate);
    updateCoefficients();

    // At least a stereo key, whatever the input's width.
    const auto filters = (size_t) juce::jmax (2, numChannels);
    scHighPass.assign (filters, fxme::TptSvf());
    scLowPass.assign (filters, fxme::TptSvf());
    updateSidechainFilters();
}

void Compressor::updateSidechainFilters()
{
    constexpr double butterworthK = 1.4142135623730951;
    const bool highPassOn = scHighPassHz > kScHighPassOffHz + 0.5f;
    const bool lowPassOn  = scLowPassHz < kScLowPassOffHz - 1.0f;

    // Coming back on, the filter starts clean, not from where it was left.
    if (highPassOn && ! scHighPassOn) for (auto& f : scHighPass) f.reset();
    if (lowPassOn  && ! scLowPassOn)  for (auto& f : scLowPass)  f.reset();
    scHighPassOn = highPassOn;
    scLowPassOn  = lowPassOn;

    for (auto& f : scHighPass) f.setup (fxme::TptSvf::gFor (scHighPassHz, currentSampleRate), butterworthK);
    for (auto& f : scLowPass)  f.setup (fxme::TptSvf::gFor (scLowPassHz, currentSampleRate), butterworthK);
}

void Compressor::setSidechainFilters (float highPassHz, float lowPassHz)
{
    scHighPassHz = highPassHz;
    scLowPassHz  = lowPassHz;
    updateSidechainFilters();
}

void Compressor::updateCoefficients()
{
    attackCoef = juce::dsp::FastMathApproximations::exp (-1.0f / (attackTimeMs * 0.001f * (float) currentSampleRate));
    releaseCoef = juce::dsp::FastMathApproximations::exp (-1.0f / (releaseTimeMs * 0.001f * (float) currentSampleRate));
    rmsCoef = juce::dsp::FastMathApproximations::exp (-1.0f / (rmsTimeMs * 0.001f * (float) currentSampleRate));
    stressCoef = juce::dsp::FastMathApproximations::exp (-1.0f / (stressTimeMs * 0.001f * (float) currentSampleRate));
    makeUpGain = juce::Decibels::decibelsToGain (postGaindB);
}

void Compressor::setAttack (float attackMs)
{
    attackTimeMs = attackMs;
    updateCoefficients();
}

void Compressor::setRelease (float releaseMs)
{
    releaseTimeMs = releaseMs;
    updateCoefficients();
}

void Compressor::setThreshold (float threshold)
{
    thresholddB = threshold;
}

void Compressor::setRatio (float r)
{
    ratio = r;
}

void Compressor::setKnee (float kneedBNew)
{
    kneedB = juce::jmax (0.0f, kneedBNew);
}

void Compressor::setReleaseMode (int mode)
{
    releaseMode = (ReleaseMode) juce::jlimit (0, 2, mode);
}

void Compressor::setPeakRmsMix (float mix)
{
    peakRmsMix = juce::jlimit (0.0f, 1.0f, mix);
}

void Compressor::setPostGain (float gaindB)
{
    postGaindB = gaindB;
    updateCoefficients();
}

void Compressor::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

bool Compressor::isOn() const
{
    // The parameter when there is one: right in a mirror (FxmeNet), which
    // never processes, and no race with the audio thread.
    return onParam != nullptr ? onParam->load() > 0.5f : on;
}

void Compressor::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam = apvts.getRawParameterValue (prefix + "_Comp_On");
    preGainParam = apvts.getRawParameterValue (prefix + "_Comp_PreGain");
    attackParam = apvts.getRawParameterValue (prefix + "_Comp_Attack");
    releaseParam = apvts.getRawParameterValue (prefix + "_Comp_Release");
    threshParam = apvts.getRawParameterValue (prefix + "_Comp_Thresh");
    ratioParam = apvts.getRawParameterValue (prefix + "_Comp_Ratio");
    kneeParam = apvts.getRawParameterValue (prefix + "_Comp_Knee");
    relModeParam = apvts.getRawParameterValue (prefix + "_Comp_RelMode");
    peakRmsParam = apvts.getRawParameterValue (prefix + "_Comp_PeakRms");
    gainParam = apvts.getRawParameterValue (prefix + "_Comp_Gain");
    scHighPassParam = apvts.getRawParameterValue (prefix + "_Comp_ScHighPass");
    scLowPassParam = apvts.getRawParameterValue (prefix + "_Comp_ScLowPass");
    extSidechainParam = apvts.getRawParameterValue (prefix + "_Comp_ExtSc");   // null without the switch
}

void Compressor::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix,
                                bool withSidechain)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { prefix + "_Comp_On", 1 }, prefix + " Comp On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_PreGain", 1 }, prefix + " Comp Pre Gain", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Attack", 1 }, prefix + " Comp Attack", 0.1f, 100.0f, 10.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Release", 1 }, prefix + " Comp Release", 10.0f, 1000.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Thresh", 1 }, prefix + " Comp Thresh", -60.0f, 0.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Ratio", 1 }, prefix + " Comp Ratio", 1.0f, 20.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Knee", 1 }, prefix + " Comp Knee", 0.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { prefix + "_Comp_RelMode", 1 }, prefix + " Comp Release Mode", juce::StringArray { "Linear", "Opto", "Vintage" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_PeakRms", 1 }, prefix + " Comp Peak/RMS", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Gain", 1 }, prefix + " Comp Gain", -24.0f, 24.0f, 0.0f));

    // Added in 0.6.0: at the end (the Pure Data externals' inlets follow this
    // order), version 2 (for hosts that order parameters by it). At their
    // defaults they change nothing.
    using Range = juce::NormalisableRange<float>;
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_ScHighPass", 2 }, prefix + " Comp SC High-pass",
                                                                   Range (kScHighPassOffHz, 2000.0f, 1.0f, 0.3f), kScHighPassOffHz));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_ScLowPass", 2 }, prefix + " Comp SC Low-pass",
                                                                   Range (500.0f, kScLowPassOffHz, 1.0f, 0.3f), kScLowPassOffHz));
    if (withSidechain)
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { prefix + "_Comp_ExtSc", 2 }, prefix + " Comp External Sidechain", false));
}

void Compressor::checkParameters()
{
    if (onParam && *onParam != lastOn) { setOn (*onParam > 0.5f); lastOn = *onParam; }

    if (preGainParam && *preGainParam != lastPreGain) { preGain = juce::Decibels::decibelsToGain (preGainParam->load()); lastPreGain = *preGainParam; }
    
    bool changed = false;
    if (attackParam && *attackParam != lastAttack) { attackTimeMs = *attackParam; lastAttack = attackTimeMs; changed = true; }
    if (releaseParam && *releaseParam != lastRelease) { releaseTimeMs = *releaseParam; lastRelease = releaseTimeMs; changed = true; }
    if (gainParam && *gainParam != lastGain) { postGaindB = *gainParam; lastGain = postGaindB; changed = true; }
    
    if (changed)
        updateCoefficients();

    if (kneeParam && *kneeParam != lastKnee) { setKnee (*kneeParam); lastKnee = *kneeParam; }
    if (relModeParam && *relModeParam != lastRelMode) { setReleaseMode ((int) *relModeParam); lastRelMode = *relModeParam; }
    if (peakRmsParam && *peakRmsParam != lastPeakRms) { setPeakRmsMix (*peakRmsParam); lastPeakRms = *peakRmsParam; }
    if ((scHighPassParam && *scHighPassParam != scHighPassHz) || (scLowPassParam && *scLowPassParam != scLowPassHz))
        setSidechainFilters (scHighPassParam != nullptr ? scHighPassParam->load() : scHighPassHz,
                             scLowPassParam != nullptr ? scLowPassParam->load() : scLowPassHz);
    if (extSidechainParam) externalSidechain = *extSidechainParam > 0.5f;

    // These don't require coefficient update
    if (threshParam) thresholddB = *threshParam;
    if (ratioParam) ratio = *ratioParam;
}

void Compressor::process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain)
{
    if (! on)
        return;

    // An infinity would stay in the level detector for good (and the gain
    // with it, at zero): out first, NaN too. Nothing else changes.
    fxme::sanitiseBlock (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), fxme::kMainPathLimit);
    buffer.applyGain (preGain);

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    auto* channelData = buffer.getArrayOfWritePointers();

    const float oneMinusAttack = 1.0f - attackCoef;
    const float oneMinusRelease = 1.0f - releaseCoef;
    const float oneMinusRms = 1.0f - rmsCoef;
    const float oneMinusStress = 1.0f - stressCoef;

    // What the detector listens to: the input (after Pre Gain), or the key.
    const bool useKey = externalSidechain && sidechain != nullptr && sidechain->getNumChannels() > 0
                        && sidechain->getNumSamples() >= numSamples;
    auto* const* detectorData = useKey ? sidechain->getArrayOfReadPointers() : buffer.getArrayOfReadPointers();
    const int detectorChannels = useKey ? sidechain->getNumChannels() : numChannels;
    const int numFilters = (int) scHighPass.size();

    for (int i = 0; i < numSamples; ++i)
    {
        // Sidechain: Max of all channels (Linked operation)
        float inputAbs = 0.0f;
        for (int ch = 0; ch < detectorChannels; ++ch)
        {
            // The input was cleaned above; the key was not: nothing bad into
            // the filters and detectors, which hold state.
            float x = useKey ? fxme::sanitiseSample (detectorData[ch][i], fxme::kMainPathLimit) : detectorData[ch][i];
            if (ch < numFilters)
            {
                if (scHighPassOn) x = scHighPass[(size_t) ch].highPass (x);
                if (scLowPassOn)  x = scLowPass[(size_t) ch].lowPass (x);
            }
            float s = std::abs (x);
            if (s > inputAbs) inputAbs = s;
        }

        // RMS estimate (running mean of squared input)
        rmsSquare = rmsCoef * rmsSquare + oneMinusRms * inputAbs * inputAbs;
        const float rmsLevel = std::sqrt (rmsSquare);

        // Blend Peak / RMS detector
        const float detect = inputAbs + (rmsLevel - inputAbs) * peakRmsMix;

        // Update stress integrator (used by Vintage release mode)
        const float grAbs = -lastGainReductiondB; // dB, positive when compressing
        stress = stressCoef * stress + oneMinusStress * grAbs;

        // Envelope Follower with program-dependent release
        if (detect > envelope)
        {
            envelope = attackCoef * envelope + oneMinusAttack * detect;
        }
        else
        {
            float effOneMinusRelease = oneMinusRelease;
            if (releaseMode == ReleaseMode::Opto)
            {
                // Slow release as instantaneous GR grows.
                const float scale = 1.0f + 0.3f * grAbs;
                effOneMinusRelease = oneMinusRelease / scale;
            }
            else if (releaseMode == ReleaseMode::Vintage)
            {
                // Slow release proportional to integrated GR ("auto" feel).
                const float scale = 1.0f + 0.5f * stress;
                effOneMinusRelease = oneMinusRelease / scale;
            }
            envelope = (1.0f - effOneMinusRelease) * envelope + effOneMinusRelease * detect;
        }

        // Gain Computer with soft knee
        const float envdB = juce::Decibels::gainToDecibels (envelope);
        float gainReductiondB = 0.0f;

        if (kneedB <= 0.0f)
        {
            if (envdB > thresholddB)
                gainReductiondB = (thresholddB - envdB) * (1.0f - 1.0f / ratio);
        }
        else
        {
            const float halfKnee = kneedB * 0.5f;
            if (envdB >= thresholddB + halfKnee)
            {
                gainReductiondB = (thresholddB - envdB) * (1.0f - 1.0f / ratio);
            }
            else if (envdB > thresholddB - halfKnee)
            {
                const float x = envdB - thresholddB + halfKnee; // 0..kneedB
                gainReductiondB = (1.0f / ratio - 1.0f) * x * x / (2.0f * kneedB);
            }
        }

        lastGainReductiondB = gainReductiondB;

        float grLinear = juce::Decibels::decibelsToGain (gainReductiondB);
        grMeter.process (&grLinear, 1);

        float gain = grLinear * makeUpGain;

        // Apply Gain
        for (int ch = 0; ch < numChannels; ++ch)
        {
            channelData[ch][i] *= gain;
        }
    }
}