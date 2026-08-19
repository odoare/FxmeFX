/*
  ==============================================================================

    Phaser.cpp

  ==============================================================================
*/

#include "Phaser.h"
#include <FxmeTools/dsp/Lfo.h>
#include <cmath>

namespace
{
    constexpr double kSmoothingSeconds = 0.05;
}

void Phaser::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused (samplesPerBlock, numChannels);

    currentSampleRate = sampleRate;

    for (auto& chain : chains)
        chain.prepare (sampleRate);
    feedbackState.fill (0.0f);

    lfo.prepare (sampleRate);
    lfo.setBpm (currentBPM);

    mixSmooth     .reset (sampleRate, kSmoothingSeconds);
    outSmooth     .reset (sampleRate, kSmoothingSeconds);
    centreSmooth  .reset (sampleRate, kSmoothingSeconds);
    depthSmooth   .reset (sampleRate, kSmoothingSeconds);
    feedbackSmooth.reset (sampleRate, kSmoothingSeconds);

    mixSmooth.setCurrentAndTargetValue (on ? mix01 : 0.0f);
    if (! on)
        outSmooth.setCurrentAndTargetValue (1.0f);

    centreSmooth.setCurrentAndTargetValue (freqHz);
    depthSmooth .setCurrentAndTargetValue (depthOct);

    chainsActive = true;
}

void Phaser::setBPM (double bpm)
{
    if (currentBPM != bpm)
    {
        currentBPM = bpm;
        lfo.setBpm (bpm);
    }
}

void Phaser::setTransport (double bpm, double ppqPosition, bool isPlaying)
{
    setBPM (bpm);
    hostPpq     = ppqPosition;
    hostPlaying = isPlaying;
}

void Phaser::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                            const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Phaser_On", 1 }, prefix + " Phaser On", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Phaser_Rate", 1 }, prefix + " Phaser Rate",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.001f, 0.3f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Phaser_Sync", 1 }, prefix + " Phaser Sync", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Phaser_Div", 1 }, prefix + " Phaser Division",
        juce::StringArray (fxme::Lfo::syncDivisionNames, fxme::Lfo::numSyncDivisions), fxme::Lfo::defaultSyncDivision));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Phaser_Shape", 1 }, prefix + " Phaser Shape",
        juce::StringArray (fxme::Lfo::shapeNames, fxme::Lfo::numShapes), fxme::Lfo::sine));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Phaser_Stages", 1 }, prefix + " Phaser Stages",
        stageChoices(), 1));   // "4" — the familiar pedal voice

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Phaser_Depth", 1 }, prefix + " Phaser Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Phaser_Freq", 1 }, prefix + " Phaser Frequency",
        juce::NormalisableRange<float> (60.0f, 4000.0f, 1.0f, 0.32f), 500.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Phaser_Feedback", 1 }, prefix + " Phaser Feedback",
        juce::NormalisableRange<float> (-95.0f, 95.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Phaser_Width", 1 }, prefix + " Phaser Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Phaser_Mix", 1 }, prefix + " Phaser Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Phaser_Out", 1 }, prefix + " Phaser Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
}

void Phaser::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam       = apvts.getRawParameterValue (prefix + "_Phaser_On");
    rateParam     = apvts.getRawParameterValue (prefix + "_Phaser_Rate");
    syncParam     = apvts.getRawParameterValue (prefix + "_Phaser_Sync");
    divParam      = apvts.getRawParameterValue (prefix + "_Phaser_Div");
    shapeParam    = apvts.getRawParameterValue (prefix + "_Phaser_Shape");
    stagesParam   = apvts.getRawParameterValue (prefix + "_Phaser_Stages");
    depthParam    = apvts.getRawParameterValue (prefix + "_Phaser_Depth");
    freqParam     = apvts.getRawParameterValue (prefix + "_Phaser_Freq");
    feedbackParam = apvts.getRawParameterValue (prefix + "_Phaser_Feedback");
    widthParam    = apvts.getRawParameterValue (prefix + "_Phaser_Width");
    mixParam      = apvts.getRawParameterValue (prefix + "_Phaser_Mix");
    outParam      = apvts.getRawParameterValue (prefix + "_Phaser_Out");
}

void Phaser::checkParameters()
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
    if (stagesParam && *stagesParam != lastStages)
    {
        const int stages = stagesForIndex ((int) *stagesParam);
        for (auto& chain : chains)
            chain.setNumStages (stages);
        lastStages = *stagesParam;
    }
    if (depthParam && *depthParam != lastDepth)
    {
        depthOct  = *depthParam * 0.01f * kSweepOctaves;
        lastDepth = *depthParam;
        depthSmooth.setTargetValue (depthOct);
    }
    if (freqParam && *freqParam != lastFreq)
    {
        freqHz   = *freqParam;
        lastFreq = *freqParam;
        centreSmooth.setTargetValue (freqHz);
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
            chainsActive = true;
    }
}

void Phaser::process (juce::AudioBuffer<float>& buffer)
{
    if (! on && ! chainsActive)
        return;

    if (lfo.isSynced() && hostPlaying)
        lfo.setPhaseFromPpq (hostPpq);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), (int) chains.size());
    if (numChannels <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix    = mixSmooth.getNextValue();
        const float outG   = outSmooth.getNextValue();
        const float centre = centreSmooth.getNextValue();
        const float octs   = depthSmooth.getNextValue();
        const float fb     = feedbackSmooth.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float chOffset = (ch == 0 ? 0.0f : widthOffset);

            // Bipolar sweep in octaves around Freq, so the notches move by
            // musical intervals rather than by a fixed number of hertz.
            const float fc = centre * std::exp2f (octs * lfo.valueAt (chOffset));

            auto& chain = chains[(size_t) ch];
            chain.setFrequency (fc);

            auto* data    = buffer.getWritePointer (ch);
            const float x = data[i];
            const float y = chain.process (x + fb * feedbackState[(size_t) ch]);
            feedbackState[(size_t) ch] = y;

            data[i] = (x + mix * (y - x)) * outG;
        }

        lfo.advance();
    }

    if (! on && ! mixSmooth.isSmoothing() && ! outSmooth.isSmoothing()
        && mixSmooth.getCurrentValue() <= 0.0f)
    {
        for (auto& chain : chains)
            chain.reset();
        feedbackState.fill (0.0f);
        chainsActive = false;
    }
}
