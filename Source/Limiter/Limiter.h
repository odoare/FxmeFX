/*
  ==============================================================================

    Limiter.h

    Look-ahead brickwall limiter / maximizer. A short look-ahead delay lets the
    gain envelope start ducking before a peak arrives, so transients are caught
    transparently; a final hard clamp at the ceiling guarantees the output can
    never exceed it. "Drive" pushes the signal into the ceiling (the maximizer
    behaviour); "Ceiling" is the absolute output limit; "Release" sets how fast
    gain recovers after a peak.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VuMeter.h"
#include <atomic>
#include <vector>

class Limiter
{
public:
    Limiter();

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void setOn (bool shouldBeOn) { on = shouldBeOn; }
    bool isOn() const { return on; }
    VuMeter& getGrMeter() { return grMeter; }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);

private:
    void updateCoefficients();

    double currentSampleRate = 44100.0;

    // Fixed look-ahead: enough to anticipate transients, small enough that the
    // (host-unreported) latency is negligible on an internal master bus.
    static constexpr float lookaheadMs = 2.0f;
    int    lookaheadSamples = 1;
    std::vector<std::vector<float>> delayLine;   // [channel][lookaheadSamples]
    int    writePos = 0;

    // Resolved parameter values.
    bool  on = false;
    float driveGain = 1.0f;       // linear input gain (maximizer push)
    float ceilingLin = 1.0f;      // linear output ceiling (brickwall)
    float releaseTimeMs = 50.0f;

    // Gain envelope + coefficients.
    float gainEnv = 1.0f;         // current gain, <= 1
    float attackCoef = 0.0f;      // matched to the look-ahead window
    float releaseCoef = 0.0f;

    VuMeter grMeter;              // gain-reduction meter (linear gain in, <= 1)

    // Parameter pointers + change tracking.
    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* ceilingParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;

    float lastOn = -1.0f, lastDrive = -1000.0f, lastCeiling = -1000.0f, lastRelease = -1.0f;
};
