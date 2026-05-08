/*
  ==============================================================================

    Transient.h

    SPL-style transient designer using the Differential Envelope Technique:
    two attack-detection envelopes (fast vs. slow attack, common release) yield
    a positive difference at signal onsets; two release-detection envelopes
    (common attack, fast vs. slow release) yield a positive difference during
    the sustain tail. The user's Attack/Sustain controls scale these
    differences and apply them as gain.

    Reference: SPL Differential Envelope Technique, originally patented as
    DE 4326811 / US 5,727,074 (Tilgner/Neumann, mid-1990s) — patents long
    expired. Also covered in Reiss & McPherson, "Audio Effects: Theory,
    Implementation and Application" (CRC, 2014).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VuMeter.h"
#include <atomic>

class Transient
{
public:
    Transient();

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    enum Character { Soft = 0, Standard = 1, Hard = 2, NumCharacters = 3 };

    void setAttackAmount (float amountMinusOneToOne);
    void setSustainAmount (float amountMinusOneToOne);
    void setPostGain (float gaindB);
    void setCharacter (int characterIndex);
    void setOn (bool shouldBeOn);
    bool isOn() const;

    VuMeter& getGainMeter() { return gainMeter; }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);

private:
    double currentSampleRate = 44100.0;

    float attackAmount  = 0.0f;
    float sustainAmount = 0.0f;
    float postGaindB    = 0.0f;
    float preGain       = 1.0f;
    int   character     = Standard;
    bool  on            = false;

    float fastAttackEnv  = 0.0f;
    float slowAttackEnv  = 0.0f;
    float fastReleaseEnv = 0.0f;
    float slowReleaseEnv = 0.0f;

    float fastAttackCoef    = 0.0f;
    float slowAttackCoef    = 0.0f;
    float commonReleaseCoef = 0.0f;
    float commonAttackCoef  = 0.0f;
    float fastReleaseCoef   = 0.0f;
    float slowReleaseCoef   = 0.0f;

    float makeUpGain = 1.0f;

    VuMeter gainMeter;

    std::atomic<float>* onParam        = nullptr;
    std::atomic<float>* preGainParam   = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* sustainParam   = nullptr;
    std::atomic<float>* gainParam      = nullptr;
    std::atomic<float>* characterParam = nullptr;

    float lastOn = -1.0f, lastPreGain = -100.0f, lastAttack = -1000.0f, lastSustain = -1000.0f, lastGain = -1000.0f, lastCharacter = -1.0f;

    void updateCoefficients();
};
