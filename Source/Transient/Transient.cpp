/*
  ==============================================================================

    Transient.cpp

  ==============================================================================
*/

#include "Transient.h"

namespace
{
    // Time-constant set (ms) selected by the Character control. Indices match
    // Transient::Character: Soft / Standard / Hard.
    struct TimeConstants
    {
        float fastAttackMs;    // attack-pair: fast envelope tracks onsets
        float slowAttackMs;    // attack-pair: slow envelope lags onsets
        float commonReleaseMs; // attack-pair: shared release
        float commonAttackMs;  // release-pair: shared (fast) attack
        float fastReleaseMs;   // release-pair: fast envelope drops quickly
        float slowReleaseMs;   // release-pair: slow envelope drops slowly
    };

    constexpr TimeConstants kCharacters[] =
    {
        // Soft     — long, gentle. Vocals, room mics, sustained material.
        { 1.5f, 40.0f, 400.0f, 3.0f, 120.0f, 500.0f },
        // Standard — balanced general-purpose drum work. Original values.
        { 0.5f, 25.0f, 250.0f, 1.0f,  60.0f, 350.0f },
        // Hard     — fast, snappy. Close-mic kick / snare / claps.
        { 0.2f, 15.0f, 150.0f, 0.5f,  30.0f, 200.0f },
    };

    // Maximum gain modification (dB) at full Attack/Sustain. The differential
    // signal is naturally bounded by the input dynamic range; we clamp to a
    // sensible range so the control feels musical.
    constexpr float kMaxModDb = 18.0f;

    // dB floor used when the envelopes are near silence — keeps gainToDecibels
    // from producing huge negative values that would inflate the difference.
    constexpr float kDbFloor = -100.0f;

    inline float msToCoef (float ms, float sampleRate)
    {
        return std::exp (-1.0f / (ms * 0.001f * sampleRate));
    }
}

Transient::Transient()
{
    updateCoefficients();
}

void Transient::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    fastAttackEnv = slowAttackEnv = fastReleaseEnv = slowReleaseEnv = 0.0f;
    gainMeter.prepare (sampleRate);
    updateCoefficients();
    juce::ignoreUnused (numChannels);
}

void Transient::updateCoefficients()
{
    const auto sr = (float) currentSampleRate;
    const auto& tc = kCharacters[juce::jlimit (0, (int) NumCharacters - 1, character)];
    fastAttackCoef    = msToCoef (tc.fastAttackMs,    sr);
    slowAttackCoef    = msToCoef (tc.slowAttackMs,    sr);
    commonReleaseCoef = msToCoef (tc.commonReleaseMs, sr);
    commonAttackCoef  = msToCoef (tc.commonAttackMs,  sr);
    fastReleaseCoef   = msToCoef (tc.fastReleaseMs,   sr);
    slowReleaseCoef   = msToCoef (tc.slowReleaseMs,   sr);
    makeUpGain        = juce::Decibels::decibelsToGain (postGaindB);
}

void Transient::setAttackAmount (float a)  { attackAmount  = juce::jlimit (-1.0f, 1.0f, a); }
void Transient::setSustainAmount (float s) { sustainAmount = juce::jlimit (-1.0f, 1.0f, s); }
void Transient::setPostGain (float g)      { postGaindB = g; updateCoefficients(); }
void Transient::setCharacter (int idx)     { character = juce::jlimit (0, (int) NumCharacters - 1, idx); updateCoefficients(); }
void Transient::setOn (bool b)             { on = b; }
bool Transient::isOn() const               { return on; }

void Transient::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam        = apvts.getRawParameterValue (prefix + "_Trans_On");
    preGainParam   = apvts.getRawParameterValue (prefix + "_Trans_PreGain");
    attackParam    = apvts.getRawParameterValue (prefix + "_Trans_Attack");
    sustainParam   = apvts.getRawParameterValue (prefix + "_Trans_Sustain");
    gainParam      = apvts.getRawParameterValue (prefix + "_Trans_Gain");
    characterParam = apvts.getRawParameterValue (prefix + "_Trans_Character");
}

void Transient::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    juce::StringArray characterOptions;
    characterOptions.add ("Soft");
    characterOptions.add ("Standard");
    characterOptions.add ("Hard");

    params.push_back (std::make_unique<juce::AudioParameterBool>   (juce::ParameterID { prefix + "_Trans_On",        1 }, prefix + " Trans On",        false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { prefix + "_Trans_PreGain",   1 }, prefix + " Trans Pre Gain",  -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { prefix + "_Trans_Attack",    1 }, prefix + " Trans Attack",   -100.0f, 100.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { prefix + "_Trans_Sustain",   1 }, prefix + " Trans Sustain",  -100.0f, 100.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { prefix + "_Trans_Gain",      1 }, prefix + " Trans Gain",      -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { prefix + "_Trans_Character", 1 }, prefix + " Trans Character", characterOptions, (int) Standard));
}

void Transient::checkParameters()
{
    if (onParam        && *onParam        != lastOn)        { setOn (*onParam > 0.5f); lastOn = *onParam; }
    if (preGainParam   && *preGainParam   != lastPreGain)   { preGain = juce::Decibels::decibelsToGain (preGainParam->load()); lastPreGain = *preGainParam; }
    if (attackParam    && *attackParam    != lastAttack)    { attackAmount  = *attackParam  * 0.01f; lastAttack  = *attackParam; }
    if (sustainParam   && *sustainParam   != lastSustain)   { sustainAmount = *sustainParam * 0.01f; lastSustain = *sustainParam; }
    if (gainParam      && *gainParam      != lastGain)      { postGaindB = *gainParam; makeUpGain = juce::Decibels::decibelsToGain (postGaindB); lastGain = *gainParam; }
    if (characterParam && *characterParam != lastCharacter) { setCharacter ((int) *characterParam); lastCharacter = *characterParam; }
}

void Transient::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    buffer.applyGain (preGain);

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    auto* channelData = buffer.getArrayOfWritePointers();

    for (int i = 0; i < numSamples; ++i)
    {
        // Linked sidechain: peak across channels.
        float inputAbs = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float s = std::abs (channelData[ch][i]);
            if (s > inputAbs) inputAbs = s;
        }

        // Attack-detection pair: differ in attack speed, share release.
        if (inputAbs > fastAttackEnv) fastAttackEnv = fastAttackCoef    * fastAttackEnv + (1.0f - fastAttackCoef)    * inputAbs;
        else                          fastAttackEnv = commonReleaseCoef * fastAttackEnv + (1.0f - commonReleaseCoef) * inputAbs;

        if (inputAbs > slowAttackEnv) slowAttackEnv = slowAttackCoef    * slowAttackEnv + (1.0f - slowAttackCoef)    * inputAbs;
        else                          slowAttackEnv = commonReleaseCoef * slowAttackEnv + (1.0f - commonReleaseCoef) * inputAbs;

        // Release-detection pair: share attack, differ in release.
        if (inputAbs > fastReleaseEnv) fastReleaseEnv = commonAttackCoef * fastReleaseEnv + (1.0f - commonAttackCoef) * inputAbs;
        else                           fastReleaseEnv = fastReleaseCoef  * fastReleaseEnv + (1.0f - fastReleaseCoef)  * inputAbs;

        if (inputAbs > slowReleaseEnv) slowReleaseEnv = commonAttackCoef * slowReleaseEnv + (1.0f - commonAttackCoef) * inputAbs;
        else                           slowReleaseEnv = slowReleaseCoef  * slowReleaseEnv + (1.0f - slowReleaseCoef)  * inputAbs;

        const float fastAttackDb  = juce::Decibels::gainToDecibels (fastAttackEnv,  kDbFloor);
        const float slowAttackDb  = juce::Decibels::gainToDecibels (slowAttackEnv,  kDbFloor);
        const float fastReleaseDb = juce::Decibels::gainToDecibels (fastReleaseEnv, kDbFloor);
        const float slowReleaseDb = juce::Decibels::gainToDecibels (slowReleaseEnv, kDbFloor);

        // attackDiff is positive at onsets; sustainDiff is positive in the tail.
        const float attackDiff  = juce::jmax (0.0f, fastAttackDb  - slowAttackDb);
        const float sustainDiff = juce::jmax (0.0f, slowReleaseDb - fastReleaseDb);

        float modDb = attackAmount  * juce::jmin (attackDiff,  kMaxModDb)
                    + sustainAmount * juce::jmin (sustainDiff, kMaxModDb);
        modDb = juce::jlimit (-kMaxModDb, kMaxModDb, modDb);

        const float gain = juce::Decibels::decibelsToGain (modDb) * makeUpGain;

        // Feed the gain meter (linear) so the UI can show modification depth.
        gainMeter.process (&gain, 1);

        for (int ch = 0; ch < numChannels; ++ch)
            channelData[ch][i] *= gain;
    }
}
