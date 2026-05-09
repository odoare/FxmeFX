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

namespace
{
    // RBJ-cookbook biquad LPF, normalised by a0.
    Oct::BiquadCoeffs makeLowpass (float fc, double sampleRate, float Q)
    {
        Oct::BiquadCoeffs c;
        const float fs    = (float) sampleRate;
        const float clamp = juce::jlimit (1.0f, 0.45f * fs, fc);
        const float w     = 2.0f * juce::MathConstants<float>::pi * clamp / fs;
        const float cs    = std::cos (w);
        const float sn    = std::sin (w);
        const float alpha = sn / (2.0f * juce::jmax (0.001f, Q));
        const float a0    = 1.0f + alpha;

        c.b0 = ((1.0f - cs) * 0.5f) / a0;
        c.b1 = (1.0f - cs) / a0;
        c.b2 = c.b0;
        c.a1 = (-2.0f * cs) / a0;
        c.a2 = (1.0f - alpha) / a0;
        return c;
    }

    // Transposed Direct Form II — robust under coefficient changes.
    inline float processBiquad (const Oct::BiquadCoeffs& c, float x, float& s1, float& s2)
    {
        const float y = c.b0 * x + s1;
        s1 = c.b1 * x - c.a1 * y + s2;
        s2 = c.b2 * x - c.a2 * y;
        return y;
    }
}

void Oct::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    channels.assign ((size_t) juce::jmax (1, numChannels), ChannelState{});
    updateCoefficients();
}

void Oct::updateCoefficients()
{
    // 4th-order Butterworth LPF: two cascaded biquads with the standard
    // Butterworth Q values for a maximally-flat magnitude response.
    detectBq1 = makeLowpass (detectFreqHz, currentSampleRate, 0.5411961f);
    detectBq2 = makeLowpass (detectFreqHz, currentSampleRate, 1.3065630f);

    toneLpCoef      = onePoleCoef (toneFreqHz,             currentSampleRate);
    envAttackCoef   = onePoleCoef (1000.0f / envAttackMs,  currentSampleRate);
    envReleaseCoef  = onePoleCoef (1000.0f / envReleaseMs, currentSampleRate);

    // Lock the comparator out for a fraction of the half-period at the LP
    // cutoff. The LP is the upper bound on the fundamental we expect to
    // detect, so a half-period at the cutoff is the *minimum* time between
    // legitimate same-direction zero-crossings of any signal that survives
    // the filter. Anything shorter than that came from harmonic ripple.
    refractorySamples = (int) ((float) currentSampleRate
                                 * refractoryFrac
                                 / (2.0f * juce::jmax (1.0f, detectFreqHz)));
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

    // Pre-detection LPF cutoff: a 4th-order Butterworth that strips
    // harmonics before the comparator. Set this just above the fundamental
    // of the played note — for a 5-string bass low B (≈30 Hz) try 80–120 Hz,
    // for guitar low E (≈82 Hz) try 200–300 Hz.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Oct_Detect", 1 }, prefix + " Oct Detect",
        juce::NormalisableRange<float> (60.0f, 1500.0f, 1.0f, 0.5f), 400.0f));

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

            // ── Input envelope follower (drives the output amplitude) ──
            const float absIn = std::abs (in);
            if (absIn > s.envelope)
                s.envelope += envAttackCoef  * (absIn - s.envelope);
            else
                s.envelope += envReleaseCoef * (absIn - s.envelope);

            // ── Pre-detection 4th-order Butterworth LPF ──
            // Two cascaded biquads: 24 dB/oct rolloff strips harmonics so
            // the comparator sees something close to a sine at the
            // fundamental. This is the single biggest difference vs. the
            // 1-pole version: a 1-pole at 800 Hz on a bass note barely
            // attenuates the 2nd/3rd harmonic and lets them re-trigger the
            // comparator inside one period.
            float lp = processBiquad (detectBq1, in, s.bq1s1, s.bq1s2);
            lp       = processBiquad (detectBq2, lp, s.bq2s1, s.bq2s2);

            // Envelope of the *detection* signal (NOT the raw input) — this
            // is the level the comparator actually sees and the right
            // reference for sizing hysteresis.
            const float absDetect = std::abs (lp);
            if (absDetect > s.detectEnvelope)
                s.detectEnvelope += envAttackCoef  * (absDetect - s.detectEnvelope);
            else
                s.detectEnvelope += envReleaseCoef * (absDetect - s.detectEnvelope);

            // ── Refractory countdown ──
            if (s.refractoryCounter > 0)
                --s.refractoryCounter;

            // ── Schmitt-trigger zero-crossing on the LP-filtered signal ──
            // Hysteresis sized to 20 % of the *detected* envelope — generous
            // enough to ignore residual harmonic ripple. Gated below the
            // input-envelope threshold so silence doesn't generate phantom
            // octaves.
            const float hyst = juce::jmax (1.0e-6f, s.detectEnvelope * hysteresisRatio);
            const bool wasHigh = s.schmittHigh;
            if (s.envelope >= gateThreshold)
            {
                if (s.schmittHigh && lp < -hyst)
                    s.schmittHigh = false;
                else if (! s.schmittHigh && lp >  hyst)
                    s.schmittHigh = true;
            }

            // Rising edge toggles ÷2 (OC-2 CD4013 cascade), but only outside
            // the refractory window. The refractory period rules out a second
            // toggle inside one fundamental cycle even when the LP-filtered
            // signal still has a small wobble.
            const bool risingEdge = (! wasHigh && s.schmittHigh);
            if (risingEdge && s.refractoryCounter <= 0)
            {
                const bool ff1Was = s.ff1;
                s.ff1 = ! s.ff1;
                s.refractoryCounter = refractorySamples;

                // ÷4 only ticks on rising edges of ÷2.
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
