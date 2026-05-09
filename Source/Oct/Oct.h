/*
  ==============================================================================

    Oct.h

    Boss OC-2 / OC-3-style monophonic octaver. Not a pitch-shifter — instead
    the input is fed through a Schmitt-trigger zero-crossing detector and a
    pair of toggle flip-flops to produce ÷2 (octave-down) and ÷4 (two
    octaves-down) square waves. Each square is multiplied by an envelope
    follower of the input so the synthesised octaves track dynamics, then
    softened with a low-pass tone control. Latency is zero and tracking is
    extremely stable on bass-range material.

    Reference:
      Boss OC-2 service manual schematic (CD4013 flip-flops driven by an
      LM393 hysteresis comparator).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class Oct
{
public:
    Oct();

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void setOn (bool shouldBeOn);
    bool isOn() const { return on; }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

private:
    struct ChannelState
    {
        // Pre-detection low-pass (1-pole) — strips high harmonics so the
        // zero-crossing detector locks on the fundamental.
        float detectLpState = 0.0f;

        // Schmitt-trigger comparator state and the two toggle flip-flops.
        bool  schmittHigh = false;   // current comparator output
        bool  ff1 = false;           // ÷2 output (octave-down square)
        bool  ff2 = false;           // ÷4 output (two-octaves-down square)

        // Envelope follower (peak with fast attack / slow release).
        float envelope = 0.0f;

        // Output tone low-pass states per octave (1-pole each).
        float oct1ToneLpState = 0.0f;
        float oct2ToneLpState = 0.0f;
    };

    double currentSampleRate = 44100.0;
    bool   on = true;

    // Live coefficients
    float detectLpCoef     = 0.0f;
    float toneLpCoef       = 0.0f;
    float envAttackCoef    = 0.0f;
    float envReleaseCoef   = 0.0f;

    // Parameter-cache values
    float detectFreqHz   = 800.0f;
    float toneFreqHz     = 2000.0f;
    float dryGainLinear  = 1.0f;
    float oct1GainLinear = juce::Decibels::decibelsToGain (-6.0f);
    float oct2GainLinear = 0.0f;
    float gateThreshold  = juce::Decibels::decibelsToGain (-50.0f);

    static constexpr float envAttackMs  = 2.0f;    // fast — track transients
    static constexpr float envReleaseMs = 120.0f;  // slow — avoid flutter
    static constexpr float hysteresisRatio = 0.05f; // schmitt hysteresis as a fraction of the envelope

    std::vector<ChannelState> channels;

    // APVTS pointers
    std::atomic<float>* onParam        = nullptr;
    std::atomic<float>* dryGainParam   = nullptr;
    std::atomic<float>* oct1GainParam  = nullptr;
    std::atomic<float>* oct2GainParam  = nullptr;
    std::atomic<float>* detectFreqParam = nullptr;
    std::atomic<float>* toneFreqParam  = nullptr;

    // Last seen values (change-detection)
    float lastOn         = -1.0f;
    float lastDryGain    = -1000.0f;
    float lastOct1Gain   = -1000.0f;
    float lastOct2Gain   = -1000.0f;
    float lastDetectFreq = -1.0f;
    float lastToneFreq   = -1.0f;

    void updateCoefficients();
    static float onePoleCoef (float cutoffHz, double sampleRate);
};
