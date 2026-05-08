/*
  ==============================================================================

    StereoDelay.cpp

  ==============================================================================
*/

#include "StereoDelay.h"

StereoDelay::StereoDelay()
{
}

float StereoDelay::Biquad::process(float in)
{
    float out = in * b0 + z1;
    z1 = in * b1 + z2 - a1 * out;
    z2 = in * b2 - a2 * out;
    return out;
}

void StereoDelay::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxDelaySamples = (int)(sampleRate * 2.0); // 2 seconds max delay
    delayBuffer.setSize(2, maxDelaySamples, false, true, true);
    delayBuffer.clear();
    writePos = 0;
    filterL.reset();
    filterR.reset();
    updateDelayTimes();
}

void StereoDelay::process(juce::AudioBuffer<float>& buffer)
{
    checkParameters();

    if (!on) return;

    const int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);
    const auto* delayDataL = delayBuffer.getReadPointer(0);
    const auto* delayDataR = delayBuffer.getReadPointer(1);
    auto* delayWriteL = delayBuffer.getWritePointer(0);
    auto* delayWriteR = delayBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        // Read from delay lines
        int readPosL = (writePos - delaySamplesL + maxDelaySamples) % maxDelaySamples;
        int readPosR = (writePos - delaySamplesR + maxDelaySamples) % maxDelaySamples;

        float delayedL = delayDataL[readPosL];
        float delayedR = delayDataR[readPosR];

        // Calculate feedback with cross-feedback
        float feedbackL = delayedL * feedbackLGain + delayedR * crossFeedbackGain;
        float feedbackR = delayedR * feedbackRGain + delayedL * crossFeedbackGain;

        // Apply filter to feedback
        float filteredFeedbackL = filterL.process(feedbackL);
        float filteredFeedbackR = filterR.process(feedbackR);

        // Write input + feedback to delay buffer
        delayWriteL[writePos] = juce::jlimit(-1.0f, 1.0f, channelDataL[i] + filteredFeedbackL);
        delayWriteR[writePos] = juce::jlimit(-1.0f, 1.0f, channelDataR[i] + filteredFeedbackR);

        // Calculate output (dry + wet)
        channelDataL[i] = channelDataL[i] * dryGainLinear + delayedL * wetGainLinear;
        channelDataR[i] = channelDataR[i] * dryGainLinear + delayedR * wetGainLinear;

        writePos = (writePos + 1) % maxDelaySamples;
    }

}

void StereoDelay::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

void StereoDelay::addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_DelayL", prefix + " Del Delay L", 0.0f, 2.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_DelayR", prefix + " Del Delay R", 0.0f, 2.0f, 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_FdbkL", prefix + " Del Feedback L", -60.0f, 6.0f, -6.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_FdbkR", prefix + " Del Feedback R", -60.0f, 6.0f, -6.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_CrossFdbk", prefix + " Del Cross Feedback", -60.0f, 6.0f, -60.0f));
    // params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_FilterCutoff", prefix + " Del Filter Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f), 5000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_FilterCutoff", prefix + " Del Filter Cutoff", 20.0f, 20000.0f, 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_FilterQ", prefix + " Del Filter Q", 0.1f, 10.0f, 0.707f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_DryGain", prefix + " Del Dry Gain", -60.0f, 6.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_Del_WetGain", prefix + " Del Wet Gain", -60.0f, 6.0f, -6.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_Del_On", prefix + " Del On", true));
}

void StereoDelay::assignParameters(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    delayLParam = apvts.getRawParameterValue(prefix + "_Del_DelayL");
    delayRParam = apvts.getRawParameterValue(prefix + "_Del_DelayR");
    fdbkLParam = apvts.getRawParameterValue(prefix + "_Del_FdbkL");
    fdbkRParam = apvts.getRawParameterValue(prefix + "_Del_FdbkR");
    crossFdbkParam = apvts.getRawParameterValue(prefix + "_Del_CrossFdbk");
    cutoffParam = apvts.getRawParameterValue(prefix + "_Del_FilterCutoff");
    qParam = apvts.getRawParameterValue(prefix + "_Del_FilterQ");
    dryGainParam = apvts.getRawParameterValue(prefix + "_Del_DryGain");
    wetGainParam = apvts.getRawParameterValue(prefix + "_Del_WetGain");
    onParam = apvts.getRawParameterValue(prefix + "_Del_On");
}

void StereoDelay::checkParameters()
{
    if (delayLParam && *delayLParam != lastDelayL)
    {
        delayTimeLMs = *delayLParam;
        lastDelayL = delayTimeLMs;
        updateDelayTimes();
    }
    if (delayRParam && *delayRParam != lastDelayR)
    {
        delayTimeRMs = *delayRParam;
        lastDelayR = delayTimeRMs;
        updateDelayTimes();
    }
    if (fdbkLParam && *fdbkLParam != lastFdbkL)
    {
        feedbackL = *fdbkLParam;
        lastFdbkL = feedbackL;
        updateGains();
    }
    if (fdbkRParam && *fdbkRParam != lastFdbkR)
    {
        feedbackR = *fdbkRParam;
        lastFdbkR = feedbackR;
        updateGains();
    }
    if (crossFdbkParam && *crossFdbkParam != lastCrossFdbk)
    {
        crossFeedback = *crossFdbkParam;
        lastCrossFdbk = crossFeedback;
        updateGains();
    }
    if (cutoffParam && *cutoffParam != lastCutoff)
    {
        filterCutoff = *cutoffParam;
        lastCutoff = filterCutoff;
        updateFilter();
    }
    if (qParam && *qParam != lastQ)
    {
        filterQ = *qParam;
        lastQ = filterQ;
        updateFilter();
    }
    if (dryGainParam && *dryGainParam != lastDryGain)
    {
        dryGain = *dryGainParam;
        lastDryGain = dryGain;
        updateGains();
    }
    if (wetGainParam && *wetGainParam != lastWetGain)
    {
        wetGain = *wetGainParam;
        lastWetGain = wetGain;
        updateGains();
    }
    if (onParam && *onParam != lastOn)
    {
        setOn (*onParam > 0.5f);
        lastOn = *onParam;
    }
}

void StereoDelay::setBPM(double bpm)
{
    if (currentBPM != bpm)
    {
        currentBPM = bpm;
        updateDelayTimes();
    }
}

void StereoDelay::updateDelayTimes()
{
    if (currentBPM <= 0.0) currentBPM = 120.0;
    double secondsPerBeat = 60.0 / currentBPM;

    delaySamplesL = (int)(delayTimeLMs * secondsPerBeat * currentSampleRate);
    if (delaySamplesL < 1) delaySamplesL = 1;
    if (delaySamplesL >= maxDelaySamples) delaySamplesL = maxDelaySamples - 1;

    delaySamplesR = (int)(delayTimeRMs * secondsPerBeat * currentSampleRate);
    if (delaySamplesR < 1) delaySamplesR = 1;
    if (delaySamplesR >= maxDelaySamples) delaySamplesR = maxDelaySamples - 1;
}

void StereoDelay::updateGains()
{
    feedbackLGain = juce::Decibels::decibelsToGain(feedbackL);
    feedbackRGain = juce::Decibels::decibelsToGain(feedbackR);
    crossFeedbackGain = juce::Decibels::decibelsToGain(crossFeedback);
    dryGainLinear = juce::Decibels::decibelsToGain(dryGain);
    wetGainLinear = juce::Decibels::decibelsToGain(wetGain);
}

void StereoDelay::updateFilter()
{
    calcLowPass(filterL, filterCutoff, filterQ);
    calcLowPass(filterR, filterCutoff, filterQ);
}

void StereoDelay::calcLowPass(Biquad& bq, float f, float q)
{
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float)currentSampleRate;
    float cos_w0 = std::cos(w0);
    float alpha = std::sin(w0) / (2.0f * q);

    float a0 = 1.0f + alpha;
    bq.b0 = (1.0f - cos_w0) / 2.0f / a0;
    bq.b1 = (1.0f - cos_w0) / a0;
    bq.b2 = (1.0f - cos_w0) / 2.0f / a0;
    bq.a1 = -2.0f * cos_w0 / a0;
    bq.a2 = (1.0f - alpha) / a0;
}