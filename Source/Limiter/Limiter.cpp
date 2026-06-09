/*
  ==============================================================================

    Limiter.cpp

  ==============================================================================
*/

#include "Limiter.h"

Limiter::Limiter()
{
    updateCoefficients();
}

void Limiter::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    lookaheadSamples  = juce::jmax (1, (int) std::round (lookaheadMs * 0.001 * sampleRate));

    // Size for at least stereo so a mono->stereo (or vice versa) block can't index
    // out of range; process() only touches the channels the buffer actually has.
    const int chans = juce::jmax (2, numChannels);
    delayLine.assign ((size_t) chans, std::vector<float> ((size_t) lookaheadSamples, 0.0f));
    writePos = 0;
    gainEnv  = 1.0f;

    grMeter.prepare (sampleRate);
    updateCoefficients();
}

void Limiter::updateCoefficients()
{
    // Attack reaches close to target within the look-ahead window so the ducking is
    // largely complete by the time the delayed peak is output; the final clamp mops
    // up any residual overshoot.
    const float attackMs = juce::jmax (0.05f, lookaheadMs * 0.5f);
    attackCoef  = std::exp (-1.0f / (attackMs * 0.001f * (float) currentSampleRate));
    releaseCoef = std::exp (-1.0f / (releaseTimeMs * 0.001f * (float) currentSampleRate));
}

void Limiter::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam      = apvts.getRawParameterValue (prefix + "_Lim_On");
    driveParam   = apvts.getRawParameterValue (prefix + "_Lim_Drive");
    ceilingParam = apvts.getRawParameterValue (prefix + "_Lim_Ceiling");
    releaseParam = apvts.getRawParameterValue (prefix + "_Lim_Release");
}

void Limiter::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Lim_On", 1 }, prefix + " Lim On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Lim_Drive", 1 }, prefix + " Lim Drive", 0.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Lim_Ceiling", 1 }, prefix + " Lim Ceiling",
        juce::NormalisableRange<float> (-24.0f, 0.0f, 0.1f), -0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Lim_Release", 1 }, prefix + " Lim Release",
        juce::NormalisableRange<float> (1.0f, 500.0f, 1.0f, 0.4f), 50.0f));
}

void Limiter::checkParameters()
{
    if (onParam && *onParam != lastOn) { on = *onParam > 0.5f; lastOn = *onParam; }
    if (driveParam && *driveParam != lastDrive)
    {
        driveGain = juce::Decibels::decibelsToGain (driveParam->load());
        lastDrive = *driveParam;
    }
    if (ceilingParam && *ceilingParam != lastCeiling)
    {
        ceilingLin = juce::Decibels::decibelsToGain (ceilingParam->load());
        lastCeiling = *ceilingParam;
    }
    if (releaseParam && *releaseParam != lastRelease)
    {
        releaseTimeMs = *releaseParam;
        lastRelease = *releaseParam;
        updateCoefficients();
    }
}

void Limiter::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    const int numCh      = juce::jmin (buffer.getNumChannels(), (int) delayLine.size());
    const int numSamples = buffer.getNumSamples();
    auto* ch = buffer.getArrayOfWritePointers();

    const float oneMinusAtt = 1.0f - attackCoef;
    const float oneMinusRel = 1.0f - releaseCoef;

    for (int i = 0; i < numSamples; ++i)
    {
        // Apply input drive and find the linked peak across channels.
        float peak = 0.0f;
        for (int c = 0; c < numCh; ++c)
        {
            const float s = ch[c][i] * driveGain;
            ch[c][i] = s;
            peak = juce::jmax (peak, std::abs (s));
        }

        // Gain that would bring this peak down to the ceiling.
        const float target = (peak > ceilingLin && peak > 0.0f) ? ceilingLin / peak : 1.0f;

        // Branching smoother: fast attack while the gain must fall (a louder peak is
        // entering the look-ahead window), slower release while it recovers. Because
        // the output is delayed by the look-ahead, the envelope has that long to reach
        // the target before the peak emerges.
        if (target < gainEnv) gainEnv = attackCoef  * gainEnv + oneMinusAtt * target;
        else                  gainEnv = releaseCoef * gainEnv + oneMinusRel * target;

        float grLinear = gainEnv;
        grMeter.process (&grLinear, 1);

        // Output the delayed sample with the anticipating gain, store the current
        // (drive-applied) sample, and hard-clamp at the ceiling as the absolute guard.
        for (int c = 0; c < numCh; ++c)
        {
            const float delayed = delayLine[(size_t) c][(size_t) writePos];
            delayLine[(size_t) c][(size_t) writePos] = ch[c][i];
            ch[c][i] = juce::jlimit (-ceilingLin, ceilingLin, delayed * gainEnv);
        }

        if (++writePos >= lookaheadSamples)
            writePos = 0;
    }
}
