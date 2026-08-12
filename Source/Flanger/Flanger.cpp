/*
  ==============================================================================

    Flanger.cpp

  ==============================================================================
*/

#include "Flanger.h"
#include <FxmeTools/dsp/Lfo.h>

namespace
{
    constexpr double kSmoothingSeconds = 0.05;

    // Safety net on the feedback path. A flanger at 95 % feedback resonates by
    // some 26 dB at the comb peaks, which is the sound; this only stops a hot
    // input from running the loop away, and is inaudible below +12 dBFS.
    constexpr float kFeedbackCeiling = 4.0f;
}

void Flanger::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused (samplesPerBlock, numChannels);

    currentSampleRate = sampleRate;

    const float maxSeconds = (kMaxDelayMs + kMaxSweepMs) * 0.001f + 0.01f;
    for (auto& line : lines)
        line.prepare (sampleRate, maxSeconds);

    lfo.prepare (sampleRate);
    lfo.setBpm (currentBPM);

    mixSmooth     .reset (sampleRate, kSmoothingSeconds);
    outSmooth     .reset (sampleRate, kSmoothingSeconds);
    baseSmooth    .reset (sampleRate, kSmoothingSeconds);
    sweepSmooth   .reset (sampleRate, kSmoothingSeconds);
    feedbackSmooth.reset (sampleRate, kSmoothingSeconds);

    mixSmooth.setCurrentAndTargetValue (on ? mix01 : 0.0f);
    if (! on)
        outSmooth.setCurrentAndTargetValue (1.0f);

    updateDelayTargets();
    baseSmooth .setCurrentAndTargetValue (baseSmooth.getTargetValue());
    sweepSmooth.setCurrentAndTargetValue (sweepSmooth.getTargetValue());

    linesActive = true;
}

void Flanger::updateDelayTargets()
{
    const float sr = (float) currentSampleRate;

    baseSmooth .setTargetValue (delayMs * 0.001f * sr);
    sweepSmooth.setTargetValue (depth01 * kMaxSweepMs * 0.001f * sr);
}

void Flanger::setBPM (double bpm)
{
    if (currentBPM != bpm)
    {
        currentBPM = bpm;
        lfo.setBpm (bpm);
    }
}

void Flanger::setTransport (double bpm, double ppqPosition, bool isPlaying)
{
    setBPM (bpm);
    hostPpq     = ppqPosition;
    hostPlaying = isPlaying;
}

void Flanger::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                             const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Flanger_On", 1 }, prefix + " Flanger On", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Flanger_Rate", 1 }, prefix + " Flanger Rate",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.001f, 0.3f), 0.25f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Flanger_Sync", 1 }, prefix + " Flanger Sync", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Flanger_Div", 1 }, prefix + " Flanger Division",
        fxme::Lfo::syncDivisionChoices(), fxme::Lfo::defaultSyncDivision));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Flanger_Shape", 1 }, prefix + " Flanger Shape",
        fxme::Lfo::shapeChoices(), fxme::Lfo::sine));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Flanger_Depth", 1 }, prefix + " Flanger Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Flanger_Delay", 1 }, prefix + " Flanger Delay",
        juce::NormalisableRange<float> (0.1f, kMaxDelayMs, 0.01f, 0.4f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Flanger_Feedback", 1 }, prefix + " Flanger Feedback",
        juce::NormalisableRange<float> (-95.0f, 95.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Flanger_Width", 1 }, prefix + " Flanger Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Flanger_Mix", 1 }, prefix + " Flanger Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Flanger_Out", 1 }, prefix + " Flanger Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
}

void Flanger::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam       = apvts.getRawParameterValue (prefix + "_Flanger_On");
    rateParam     = apvts.getRawParameterValue (prefix + "_Flanger_Rate");
    syncParam     = apvts.getRawParameterValue (prefix + "_Flanger_Sync");
    divParam      = apvts.getRawParameterValue (prefix + "_Flanger_Div");
    shapeParam    = apvts.getRawParameterValue (prefix + "_Flanger_Shape");
    depthParam    = apvts.getRawParameterValue (prefix + "_Flanger_Depth");
    delayParam    = apvts.getRawParameterValue (prefix + "_Flanger_Delay");
    feedbackParam = apvts.getRawParameterValue (prefix + "_Flanger_Feedback");
    widthParam    = apvts.getRawParameterValue (prefix + "_Flanger_Width");
    mixParam      = apvts.getRawParameterValue (prefix + "_Flanger_Mix");
    outParam      = apvts.getRawParameterValue (prefix + "_Flanger_Out");
}

void Flanger::checkParameters()
{
    if (rateParam && *rateParam != lastRate)
    {
        lfo.setFrequency (*rateParam);
        lastRate = *rateParam;
    }
    if (syncParam && *syncParam != lastSync)
    {
        lfo.setSynced (*syncParam > 0.5f);
        lastSync = *syncParam;
    }
    if (divParam && *divParam != lastDiv)
    {
        lfo.setSyncBeats (fxme::Lfo::syncDivisionBeats ((int) *divParam));
        lastDiv = *divParam;
    }
    if (shapeParam && *shapeParam != lastShape)
    {
        lfo.setShape ((int) *shapeParam);
        lastShape = *shapeParam;
    }
    if (depthParam && *depthParam != lastDepth)
    {
        depth01   = *depthParam * 0.01f;
        lastDepth = *depthParam;
        updateDelayTargets();
    }
    if (delayParam && *delayParam != lastDelay)
    {
        delayMs   = *delayParam;
        lastDelay = *delayParam;
        updateDelayTargets();
    }
    if (feedbackParam && *feedbackParam != lastFeedback)
    {
        feedbackSmooth.setTargetValue (*feedbackParam * 0.01f);
        lastFeedback = *feedbackParam;
    }
    if (widthParam && *widthParam != lastWidth)
    {
        widthOffset = *widthParam * 0.01f * 0.5f;
        lastWidth   = *widthParam;
    }
    if (mixParam && *mixParam != lastMix)
    {
        mix01   = *mixParam * 0.01f;
        lastMix = *mixParam;
        if (on)
            mixSmooth.setTargetValue (mix01);
    }
    if (outParam && *outParam != lastOut)
    {
        lastOut = *outParam;
        if (on)
            outSmooth.setTargetValue (juce::Decibels::decibelsToGain (outParam->load()));
    }
    if (onParam && *onParam != lastOn)
    {
        on     = *onParam > 0.5f;
        lastOn = *onParam;

        mixSmooth.setTargetValue (on ? mix01 : 0.0f);
        outSmooth.setTargetValue (on ? juce::Decibels::decibelsToGain (outParam != nullptr ? outParam->load() : 0.0f)
                                     : 1.0f);
        if (on)
            linesActive = true;
    }
}

void Flanger::process (juce::AudioBuffer<float>& buffer)
{
    if (! on && ! linesActive)
        return;

    if (lfo.isSynced() && hostPlaying)
        lfo.setPhaseFromPpq (hostPpq);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), (int) lines.size());
    if (numChannels <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix   = mixSmooth.getNextValue();
        const float outG  = outSmooth.getNextValue();
        const float base  = baseSmooth.getNextValue();
        const float sweep = sweepSmooth.getNextValue();
        const float fb    = feedbackSmooth.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float chOffset = (ch == 0 ? 0.0f : widthOffset);

            // Unipolar sweep: Delay is the shortest delay of the travel, Depth
            // how far above it the LFO reaches.
            const float mod   = 0.5f + 0.5f * lfo.valueAt (chOffset);
            const float wet   = lines[(size_t) ch].read (base + sweep * mod);

            auto* data    = buffer.getWritePointer (ch);
            const float x = data[i];
            lines[(size_t) ch].write (juce::jlimit (-kFeedbackCeiling, kFeedbackCeiling,
                                                    x + fb * wet));
            data[i] = (x + mix * (wet - x)) * outG;
        }

        lfo.advance();
    }

    if (! on && ! mixSmooth.isSmoothing() && ! outSmooth.isSmoothing()
        && mixSmooth.getCurrentValue() <= 0.0f)
    {
        for (auto& line : lines)
            line.reset();
        linesActive = false;
    }
}
