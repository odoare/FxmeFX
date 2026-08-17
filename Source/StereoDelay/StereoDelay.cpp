/*
  ==============================================================================

    StereoDelay.cpp

  ==============================================================================
*/

#include "StereoDelay.h"

StereoDelay::StereoDelay()
{
}

void StereoDelay::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxDelaySamples = (int)(sampleRate * (double) maxDelaySeconds);
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

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numChannels < 1)
        return;

    // isBusesLayoutSupported accepts mono, so a mono instance has to work here:
    // there is no right channel to read or write, and cross feedback has
    // nothing to cross to. (getWritePointer(1) on a mono buffer returns the
    // null terminator of the channel array, so the old unconditional read of
    // channel 1 crashed rather than degrading.)
    const bool stereo = numChannels >= 2;

    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = stereo ? buffer.getWritePointer(1) : nullptr;
    const auto* delayDataL = delayBuffer.getReadPointer(0);
    const auto* delayDataR = delayBuffer.getReadPointer(1);
    auto* delayWriteL = delayBuffer.getWritePointer(0);
    auto* delayWriteR = delayBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        // Read from delay lines
        int readPosL = (writePos - delaySamplesL + maxDelaySamples) % maxDelaySamples;
        float delayedL = delayDataL[readPosL];

        if (stereo)
        {
            int readPosR = (writePos - delaySamplesR + maxDelaySamples) % maxDelaySamples;
            float delayedR = delayDataR[readPosR];

            // Calculate feedback with cross-feedback
            float feedbackL = delayedL * feedbackLGain + delayedR * crossFeedbackGain;
            float feedbackR = delayedR * feedbackRGain + delayedL * crossFeedbackGain;

            // Apply filter to feedback
            float filteredFeedbackL = filterL.processSample(feedbackL);
            float filteredFeedbackR = filterR.processSample(feedbackR);

            // Write input + feedback to delay buffer
            delayWriteL[writePos] = juce::jlimit(-1.0f, 1.0f, channelDataL[i] + filteredFeedbackL);
            delayWriteR[writePos] = juce::jlimit(-1.0f, 1.0f, channelDataR[i] + filteredFeedbackR);

            // Calculate output (dry + wet)
            channelDataL[i] = channelDataL[i] * dryGainLinear + delayedL * wetGainLinear;
            channelDataR[i] = channelDataR[i] * dryGainLinear + delayedR * wetGainLinear;
        }
        else
        {
            float filteredFeedbackL = filterL.processSample(delayedL * feedbackLGain);

            delayWriteL[writePos] = juce::jlimit(-1.0f, 1.0f, channelDataL[i] + filteredFeedbackL);
            channelDataL[i] = channelDataL[i] * dryGainLinear + delayedL * wetGainLinear;
        }

        writePos = (writePos + 1) % maxDelaySamples;
    }

}

void StereoDelay::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

void StereoDelay::addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_Del_On", prefix + " Del On", true));
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

    // Appended at the tail on purpose: the Pd external exposes parameters as
    // inlets in this order, so inserting anything earlier would renumber the
    // inlets of every existing patch. versionHint 2: these arrived after the
    // original parameter set, which read the delay times as beats.
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { prefix + "_Del_ModeL", 2 }, prefix + " Del Mode L",
        delayModeChoices(), (int) fxme::DelayTimeMode::seconds));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { prefix + "_Del_ModeR", 2 }, prefix + " Del Mode R",
        delayModeChoices(), (int) fxme::DelayTimeMode::seconds));
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
    modeLParam = apvts.getRawParameterValue(prefix + "_Del_ModeL");
    modeRParam = apvts.getRawParameterValue(prefix + "_Del_ModeR");
}

void StereoDelay::checkParameters()
{
    if (delayLParam && *delayLParam != lastDelayL)
    {
        delayValueL = *delayLParam;
        lastDelayL = delayValueL;
        updateDelayTimes();
    }
    if (delayRParam && *delayRParam != lastDelayR)
    {
        delayValueR = *delayRParam;
        lastDelayR = delayValueR;
        updateDelayTimes();
    }
    if (modeLParam && *modeLParam != lastModeL)
    {
        lastModeL = *modeLParam;
        modeL = (fxme::DelayTimeMode) juce::jlimit (0, 2, (int) lastModeL);
        updateDelayTimes();
    }
    if (modeRParam && *modeRParam != lastModeR)
    {
        lastModeR = *modeRParam;
        modeR = (fxme::DelayTimeMode) juce::jlimit (0, 2, (int) lastModeR);
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

fxme::DelayTimeResolver StereoDelay::makeResolver() const
{
    fxme::DelayTimeResolver resolver;
    resolver.bpm        = currentBPM > 0.0 ? currentBPM : 120.0;
    resolver.noteHz     = lastNoteHz.load();
    resolver.maxSeconds = maxDelaySeconds;
    return resolver;
}

float StereoDelay::resolvedSeconds (bool rightSide) const
{
    const auto resolver = makeResolver();
    return rightSide ? resolver.toSeconds (delayValueR, modeR)
                     : resolver.toSeconds (delayValueL, modeL);
}

void StereoDelay::updateDelayTimes()
{
    const auto resolver = makeResolver();

    const auto toSamples = [this, &resolver] (float value, fxme::DelayTimeMode mode)
    {
        const int samples = (int) (resolver.toSeconds (value, mode) * currentSampleRate);
        return juce::jlimit (1, juce::jmax (1, maxDelaySamples - 1), samples);
    };

    delaySamplesL = toSamples (delayValueL, modeL);
    delaySamplesR = toSamples (delayValueR, modeR);
}

void StereoDelay::setLastNoteHz (float hz)
{
    if (hz <= 0.0f || hz == lastNoteHz.load())
        return;

    lastNoteHz.store (hz);

    // Only the note mode reads the pitch, so nothing else has to be recomputed.
    if (modeL == fxme::DelayTimeMode::notePeriod || modeR == fxme::DelayTimeMode::notePeriod)
        updateDelayTimes();
}

void StereoDelay::migrateLegacyState (juce::XmlElement& state, const juce::String& prefix)
{
    const auto modeLId = prefix + "_Del_ModeL";
    const auto modeRId = prefix + "_Del_ModeR";

    // A state that already carries the modes has been through this once.
    for (auto* param : state.getChildWithTagNameIterator ("PARAM"))
    {
        const auto id = param->getStringAttribute ("id");
        if (id == modeLId || id == modeRId)
            return;
    }

    // Version 1 read both time values as beats. The sync mode reads them as a
    // proportion of a whole note, so a quarter of the old number is the same
    // delay: 0.5 beats and 0.125 whole notes are both an eighth note.
    for (auto* param : state.getChildWithTagNameIterator ("PARAM"))
    {
        const auto id = param->getStringAttribute ("id");
        if (id == prefix + "_Del_DelayL" || id == prefix + "_Del_DelayR")
            param->setAttribute ("value", param->getDoubleAttribute ("value", 0.0) * 0.25);
    }

    for (const auto& id : { modeLId, modeRId })
    {
        auto* param = state.createNewChildElement ("PARAM");
        param->setAttribute ("id", id);
        param->setAttribute ("value", (int) fxme::DelayTimeMode::tempoSync);
    }
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
    const auto coeffs = fxme::BiquadCoeffs::lowpass(currentSampleRate, filterCutoff, filterQ);
    filterL.c = coeffs;
    filterR.c = coeffs;
}