/*
  ==============================================================================

    Limiter.h

    Look-ahead brickwall limiter / maximizer. A short look-ahead delay lets the
    gain envelope start ducking before a peak arrives, so transients are caught
    transparently; a final hard clamp at the ceiling guarantees the output can
    never exceed it. "Drive" pushes the signal into the ceiling (the maximizer
    behaviour); "Ceiling" is the absolute output limit; "Release" sets how fast
    gain recovers after a peak.

    True Peak mode limits the inter-sample peaks too, those a DAC's
    reconstruction makes between samples, as ITU-R BS.1770 measures them: the
    peak is read on a 4x oversampled copy of the signal (a 49-tap polyphase
    interpolator, 12 or 13 taps per phase), and the gain is built so that it
    is fully down for every sample around a peak when that peak comes out:
    the target held over the look-ahead, an instant attack, the release, then
    a moving average over the look-ahead. Its delay is 6 samples longer than
    sample mode's (the interpolator's). Like any 4x estimate it may read up
    to about half a dB low on content near Nyquist, which is why masters aim
    at -1 dBTP rather than 0.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VuMeter.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

class Limiter
{
public:
    Limiter();

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void setOn (bool shouldBeOn) { on = shouldBeOn; }
    /** The On parameter when there is one: right in a mirror (FxmeNet), which
        never processes, and no race with the audio thread. */
    bool isOn() const { return onParam != nullptr ? onParam->load() > 0.5f : on; }
    VuMeter& getGrMeter() { return grMeter; }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);

private:
    void updateCoefficients();
    void processTruePeak (juce::AudioBuffer<float>& buffer);
    void resetTruePeak();

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

    std::atomic<float>* truePeakParam = nullptr;

    float lastOn = -1.0f, lastDrive = -1000.0f, lastCeiling = -1000.0f, lastRelease = -1.0f;
    float lastTruePeak = -1.0f;

    //==========================================================================
    // True Peak mode (all of it sized in prepare(); nothing allocates in process()).
    bool truePeak = false;

    // The 4x interpolator: phase k's tap j is the prototype's h[4j + k], whose
    // centre is h[24], so phase k gives the signal at n - 6 + k / 4.
    static constexpr int tpPhases = 4, tpTaps = 13, tpDelay = 6;
    std::array<std::array<float, tpTaps>, tpPhases> tpCoeffs {};
    struct TpHistory { std::array<float, 2 * tpTaps> x {}; int pos = 0; };
    std::vector<TpHistory> tpHistory;              // per channel

    std::vector<std::vector<float>> tpDelayLine;   // [channel][lookahead + tpDelay]
    int tpWritePos = 0;

    // The peak held over the last lookahead + 2 samples: a monotonic queue.
    int tpHold = 1;
    std::vector<float> holdValue;
    std::vector<std::uint32_t> holdIndex;
    int holdFront = 0, holdCount = 0;
    std::uint32_t sampleCounter = 0;

    float tpRelease = 1.0f;                        // the target, released
    std::vector<float> boxRing;                    // the moving average over the look-ahead
    double boxSum = 0.0;
    int boxPos = 0;
};
