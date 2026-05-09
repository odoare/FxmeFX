/*
  ==============================================================================

    Oct.cpp

  ==============================================================================
*/

#include "Oct.h"

Oct::Oct()
{
    updateCoefficients();
}

float Oct::onePoleCoef (float cutoffHz, double sampleRate)
{
    const float fs = (float) sampleRate;
    const float fc = juce::jlimit (1.0f, 0.45f * fs, cutoffHz);
    return 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * fc / fs);
}

void Oct::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    channels.assign ((size_t) juce::jmax (1, numChannels), ChannelState{});
    updateCoefficients();
}

void Oct::updateCoefficients()
{
    detectLpCoef    = onePoleCoef (detectFreqHz, currentSampleRate);
    toneLpCoef      = onePoleCoef (toneFreqHz,   currentSampleRate);
    envAttackCoef   = onePoleCoef (1000.0f / envAttackMs,  currentSampleRate);
    envReleaseCoef  = onePoleCoef (1000.0f / envReleaseMs, currentSampleRate);
}

void Oct::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

void Oct::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam         = apvts.getRawParameterValue (prefix + "_Oct_On");
    dryGainParam    = apvts.getRawParameterValue (prefix + "_Oct_Dry");
    oct1GainParam   = apvts.getRawParameterValue (prefix + "_Oct_Oct1");
    oct2GainParam   = apvts.getRawParameterValue (prefix + "_Oct_Oct2");
    detectFreqParam = apvts.getRawParameterValue (prefix + "_Oct_Detect");
    toneFreqParam   = apvts.getRawParameterValue (prefix + "_Oct_Tone");
}

void Oct::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                         const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Oct_On", 1 }, prefix + " Oct On", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Oct_Dry",  1 }, prefix + " Oct Dry",   -60.0f, 12.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Oct_Oct1", 1 }, prefix + " Oct Oct1",  -60.0f, 12.0f, -6.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Oct_Oct2", 1 }, prefix + " Oct Oct2",  -60.0f, 12.0f, -60.0f));

    // Pre-detection LPF cutoff: keeps the zero-crossing detector locked on the
    // fundamental even when the input has lots of harmonics.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Oct_Detect", 1 }, prefix + " Oct Detect",
        juce::NormalisableRange<float> (200.0f, 3000.0f, 1.0f, 0.5f), 800.0f));

    // Output tone LPF — softens the synthesised square waves.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Oct_Tone", 1 }, prefix + " Oct Tone",
        juce::NormalisableRange<float> (300.0f, 10000.0f, 1.0f, 0.5f), 2000.0f));
}

void Oct::checkParameters()
{
    if (onParam && *onParam != lastOn)
    {
        setOn (*onParam > 0.5f);
        lastOn = *onParam;
    }

    if (dryGainParam && *dryGainParam != lastDryGain)
    {
        dryGainLinear = juce::Decibels::decibelsToGain (dryGainParam->load(), -60.0f);
        lastDryGain = *dryGainParam;
    }
    if (oct1GainParam && *oct1GainParam != lastOct1Gain)
    {
        oct1GainLinear = juce::Decibels::decibelsToGain (oct1GainParam->load(), -60.0f);
        lastOct1Gain = *oct1GainParam;
    }
    if (oct2GainParam && *oct2GainParam != lastOct2Gain)
    {
        oct2GainLinear = juce::Decibels::decibelsToGain (oct2GainParam->load(), -60.0f);
        lastOct2Gain = *oct2GainParam;
    }

    bool coefsChanged = false;
    if (detectFreqParam && *detectFreqParam != lastDetectFreq)
    {
        detectFreqHz = *detectFreqParam;
        lastDetectFreq = detectFreqHz;
        coefsChanged = true;
    }
    if (toneFreqParam && *toneFreqParam != lastToneFreq)
    {
        toneFreqHz = *toneFreqParam;
        lastToneFreq = toneFreqHz;
        coefsChanged = true;
    }
    if (coefsChanged)
        updateCoefficients();
}

void Oct::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    if ((int) channels.size() < numChannels)
        channels.resize ((size_t) numChannels);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& s = channels[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = data[i];

            // ── Envelope follower (peak, fast attack / slow release) ──
            const float absIn = std::abs (in);
            if (absIn > s.envelope)
                s.envelope += envAttackCoef * (absIn - s.envelope);
            else
                s.envelope += envReleaseCoef * (absIn - s.envelope);

            // ── Pre-detection LPF — keeps zero-crossing locked on the fundamental ──
            s.detectLpState += detectLpCoef * (in - s.detectLpState);

            // ── Schmitt-trigger zero-crossing with envelope-tracked hysteresis ──
            // Below the gate threshold the comparator stops toggling, which
            // prevents noise from generating phantom octaves during silence.
            const float hyst = juce::jmax (1.0e-6f, s.envelope * hysteresisRatio);
            const bool wasHigh = s.schmittHigh;
            if (s.envelope >= gateThreshold)
            {
                if (s.schmittHigh && s.detectLpState < -hyst)
                    s.schmittHigh = false;
                else if (! s.schmittHigh && s.detectLpState >  hyst)
                    s.schmittHigh = true;
            }

            // Rising edge of the comparator toggles ÷2; rising edge of ÷2
            // toggles ÷4 — exactly the OC-2 CD4013 cascade.
            if (! wasHigh && s.schmittHigh)
            {
                const bool ff1Was = s.ff1;
                s.ff1 = ! s.ff1;
                if (! ff1Was && s.ff1)
                    s.ff2 = ! s.ff2;
            }

            // ── Synthesise squares × envelope, soften through tone LPF ──
            const float oct1Raw = (s.ff1 ? 1.0f : -1.0f) * s.envelope;
            const float oct2Raw = (s.ff2 ? 1.0f : -1.0f) * s.envelope;

            s.oct1ToneLpState += toneLpCoef * (oct1Raw - s.oct1ToneLpState);
            s.oct2ToneLpState += toneLpCoef * (oct2Raw - s.oct2ToneLpState);

            // ── Mix dry + Oct1 + Oct2 ──
            data[i] = in * dryGainLinear
                    + s.oct1ToneLpState * oct1GainLinear
                    + s.oct2ToneLpState * oct2GainLinear;
        }
    }
}
