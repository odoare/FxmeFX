/*
  ==============================================================================

    Tube.cpp
    Created: 3 Feb 2026 6:36:27pm
    Author:  doare

  ==============================================================================
*/

#include "Tube.h"

namespace
{
    // Asymmetric soft saturator approximating a 12AX7 plate stage:
    //   - Softer compression on the negative side (extended dynamic range).
    //   - Harder corner on the positive side (grid-current conduction).
    // Slope at x = 0 is 1 on both branches, so it composes cleanly with drive.
    inline float triodeCurve (float x)
    {
        constexpr float kSoft = 1.2f; // negative side
        constexpr float kHard = 2.0f; // positive side (harder corner)
        if (x >= 0.0f)
            return (1.0f - std::exp (-kHard * x)) / kHard;
        return (std::exp (kSoft * x) - 1.0f) / kSoft;
    }

    // One-sided saturator: ~0 below cutoff, tanh-saturated above.
    inline float halfWaveSat (float u)
    {
        if (u <= 0.0f) return 0.0f;
        return std::tanh (u);
    }

    // Push-pull (Class AB) — two oppositely biased half-wave saturators summed.
    // overlap = 0  -> Class B (sharp crossover, odd harmonics dominate)
    // overlap > 0  -> Class AB (smoother crossover, gradual approach to Class A)
    inline float classAbCurve (float x, float overlap)
    {
        return halfWaveSat (x + overlap) - halfWaveSat (-x + overlap);
    }
}

Tube::Tube() {}

void Tube::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    railVoltage = 1.0f;

    const int maxChannels = 4;
    x1.assign (maxChannels, 0.0f);
    y1.assign (maxChannels, 0.0f);
    xLpState.assign (maxChannels, 0.0f);
    satLpState.assign (maxChannels, 0.0f);

    // Tone shelf split frequency (~1.5 kHz) — splits "warmth" from "edge".
    constexpr float toneSplitHz = 1500.0f;
    toneLpCoef = std::exp (-2.0f * juce::MathConstants<float>::pi * toneSplitHz / (float) sampleRate);

    // Power-supply sag time constants: fast attack, slow recovery.
    sagAttackCoef  = std::exp (-1.0f / (0.010f * (float) sampleRate)); // ~10 ms
    sagReleaseCoef = std::exp (-1.0f / (0.200f * (float) sampleRate)); // ~200 ms
}

void Tube::setDrive (float drivedB) { drive = juce::Decibels::decibelsToGain (drivedB); }
void Tube::setBias (float b)        { bias = b; }
void Tube::setTone (float t)
{
    tone = juce::jlimit (-1.0f, 1.0f, t);
    // Maps tone to a +/- 12 dB pre-emphasis gain (4x or 1/4x at extremes).
    toneGain = std::pow (4.0f, tone);
}
void Tube::setSag (float s)         { sag = juce::jlimit (0.0f, 1.0f, s); }
void Tube::setOutput (float gaindB) { outputGain = juce::Decibels::decibelsToGain (gaindB); }
void Tube::setOn (bool shouldBeOn)  { on = shouldBeOn; }
void Tube::setModel (TubeModel m)   { currentModel = m; }
bool Tube::isOn() const             { return on; }

void Tube::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam    = apvts.getRawParameterValue (prefix + "_Tube_On");
    driveParam = apvts.getRawParameterValue (prefix + "_Tube_Drive");
    biasParam  = apvts.getRawParameterValue (prefix + "_Tube_Bias");
    toneParam  = apvts.getRawParameterValue (prefix + "_Tube_Tone");
    sagParam   = apvts.getRawParameterValue (prefix + "_Tube_Sag");
    outParam   = apvts.getRawParameterValue (prefix + "_Tube_Out");
    modelParam = apvts.getRawParameterValue (prefix + "_Tube_Model");
}

void Tube::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool>  (juce::ParameterID { prefix + "_Tube_On",    1 }, prefix + " Tube On",    false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Tube_Drive", 1 }, prefix + " Tube Drive", 0.0f, 40.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Tube_Bias",  1 }, prefix + " Tube Bias",  0.0f, 0.5f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Tube_Tone",  1 }, prefix + " Tube Tone", -1.0f, 1.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Tube_Sag",   1 }, prefix + " Tube Sag",   0.0f, 1.0f,  0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Tube_Out",   1 }, prefix + " Tube Out", -20.0f, 20.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { prefix + "_Tube_Model", 1 }, prefix + " Tube Model",
                                                                    juce::StringArray { "Standard", "Dynamic", "Triode", "Class AB" }, 0));
}

void Tube::checkParameters()
{
    if (onParam    && *onParam    != lastOn)    { setOn (*onParam > 0.5f);                              lastOn = *onParam; }
    if (driveParam && *driveParam != lastDrive) { setDrive (*driveParam);                               lastDrive = *driveParam; }
    if (biasParam  && *biasParam  != lastBias)  { setBias (*biasParam);                                 lastBias = *biasParam; }
    if (toneParam  && *toneParam  != lastTone)  { setTone (*toneParam);                                 lastTone = *toneParam; }
    if (sagParam   && *sagParam   != lastSag)   { setSag (*sagParam);                                   lastSag = *sagParam; }
    if (outParam   && *outParam   != lastOut)   { setOutput (*outParam);                                lastOut = *outParam; }
    if (modelParam && *modelParam != lastModel) { setModel (static_cast<TubeModel> ((int) *modelParam)); lastModel = *modelParam; }
}

void Tube::process (juce::AudioBuffer<float>& buffer)
{
    if (! on) return;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    if (numChannels > (int) x1.size()) return;

    float* const* dataAll = buffer.getArrayOfWritePointers();

    const float oneMinusToneCoef    = 1.0f - toneLpCoef;
    const float oneMinusSagAttack   = 1.0f - sagAttackCoef;
    const float oneMinusSagRelease  = 1.0f - sagReleaseCoef;
    const float invToneGain         = 1.0f / toneGain;

    // Sag is applied to the dynamics-aware models. Standard is the static reference.
    const bool sagActive = (sag > 0.0f) && (currentModel != TubeModel::Standard);

    for (int i = 0; i < numSamples; ++i)
    {
        // Track instantaneous demand across all channels (max abs).
        float maxAbs = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float a = std::abs (dataAll[ch][i]);
            if (a > maxAbs) maxAbs = a;
        }

        // RC rail model: rail droops with current draw, recovers slowly.
        if (sagActive)
        {
            const float driveAbs = drive * maxAbs;
            // Saturating demand in [0, 1] so very high drive doesn't blow up the target.
            const float demand   = driveAbs / (1.0f + driveAbs);
            const float target   = 1.0f - sag * demand;

            const float coef = (target < railVoltage) ? sagAttackCoef : sagReleaseCoef;
            const float oneMinus = (target < railVoltage) ? oneMinusSagAttack : oneMinusSagRelease;
            railVoltage = coef * railVoltage + oneMinus * target;
            railVoltage = juce::jlimit (0.1f, 1.0f, railVoltage);
        }
        else
        {
            railVoltage = 1.0f;
        }

        // Per-physics: input is normalised to rail-relative units, curve clips
        // to ±1 in that frame, then scaled back. Gives both "harder clipping"
        // and "less swing" as the rail collapses.
        const float effRail = railVoltage;
        const float invRail = 1.0f / effRail;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float in = dataAll[ch][i];

            // ---- Pre-emphasis (tone) ----
            float xLp = xLpState[ch];
            xLp = toneLpCoef * xLp + oneMinusToneCoef * in;
            xLpState[ch] = xLp;
            const float xHf  = in - xLp;
            const float pre  = xLp + toneGain * xHf;

            // ---- Drive ----
            const float driven = pre * drive;

            // ---- Saturation (curve depends on model) ----
            float saturated;
            switch (currentModel)
            {
                case TubeModel::Standard:
                    saturated = std::tanh (driven + bias);
                    break;
                case TubeModel::Dynamic:
                    // Same curve as Standard; sag handled by the rail model.
                    saturated = effRail * std::tanh ((driven + bias) * invRail);
                    break;
                case TubeModel::Triode:
                    saturated = effRail * triodeCurve ((driven + bias) * invRail);
                    break;
                case TubeModel::ClassAB:
                    // For Class AB, bias sets the operating-point overlap.
                    saturated = effRail * classAbCurve (driven * invRail, bias);
                    break;
                default:
                    saturated = std::tanh (driven + bias);
                    break;
            }

            // ---- De-emphasis (tone post-filter, inverse of pre) ----
            float satLp = satLpState[ch];
            satLp = toneLpCoef * satLp + oneMinusToneCoef * saturated;
            satLpState[ch] = satLp;
            const float satHf = saturated - satLp;
            const float post  = satLp + invToneGain * satHf;

            // ---- DC blocker (removes offset from asymmetry / bias) ----
            const float x_1 = x1[ch];
            const float y_1 = y1[ch];
            const float out = post - x_1 + 0.995f * y_1;
            x1[ch] = post;
            y1[ch] = out;

            dataAll[ch][i] = out * outputGain;
        }
    }
}
