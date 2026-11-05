/*
  ==============================================================================

    Tube.h
    Created: 3 Feb 2026 6:36:27pm
    Author:  doare

    Reference:
    Zölzer, U. (Ed.). (2011). DAFX: Digital Audio Effects (2nd ed.). Wiley.
    Chapter 5: Non-linear Processing (Section 5.3: Valve Emulation).

    Oversampling (Off, 2x, 4x): the whole stage (tone, drive, curve, sag, DC
    blocker) runs at 2 or 4 times the rate, between JUCE's polyphase IIR
    half-band filters, so the harmonics a hard drive makes above Nyquist are
    filtered out instead of folding back as inharmonic aliases. Off by
    default (the stage as it was). It delays the signal by a few samples:
    getLatencySamples() says how many, for the processor to report.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

class Tube
{
public:
    enum class TubeModel
    {
        Standard = 0,
        Dynamic  = 1,
        Triode   = 2,
        ClassAB  = 3
    };

    Tube();

    void prepare (double sampleRate);
    void process (juce::AudioBuffer<float>& buffer);

    void setDrive (float drivedB);
    void setBias (float bias);
    void setTone (float tone);
    void setSag (float sag);
    void setOutput (float gaindB);
    void setOn (bool shouldBeOn);
    void setModel (TubeModel model);
    bool isOn() const;

    /** The oversampling's delay at the current setting, in samples at the
        host's rate (0 when off). Any thread. */
    int getLatencySamples() const noexcept { return latencySamples.load(); }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);

private:
    void processStage (float* const* data, int numChannels, int numSamples);
    void setOversampling (int factor);
    void updateRateCoefficients();   // for the rate the stage runs at

    double currentSampleRate = 44100.0;

    // Oversampling: both built in prepare(), the one in use chosen by factor.
    static constexpr int maxChannels = 4;
    static constexpr int oversamplingChunk = 512;   // the most samples sent up at once
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler2x, oversampler4x;
    int oversamplingFactor = 1;
    std::atomic<int> latencySamples { 0 };
    float dcCoef = 0.995f;          // the DC blocker's pole, for the stage's rate
    bool on = true;
    TubeModel currentModel = TubeModel::Standard;

    float drive = 1.0f;
    float bias = 0.0f;
    float tone = 0.0f;       // -1..+1, frequency-dependent saturation
    float toneGain = 1.0f;   // = 4^tone, pre-emphasis amount
    float sag = 0.0f;        // 0..1, power-supply sag intensity
    float outputGain = 1.0f;

    // Power-supply sag state (RC rail model)
    float railVoltage = 1.0f;

    // Filter coefficients
    float toneLpCoef = 0.0f;   // one-pole LP (~1.5 kHz) for tone shelf
    float sagAttackCoef = 0.0f;
    float sagReleaseCoef = 0.0f;

    // Per-channel state
    std::vector<float> x1, y1;             // DC blocker
    std::vector<float> xLpState, satLpState; // tone pre/post-LP

    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* biasParam = nullptr;
    std::atomic<float>* toneParam = nullptr;
    std::atomic<float>* sagParam = nullptr;
    std::atomic<float>* outParam = nullptr;
    std::atomic<float>* modelParam = nullptr;
    std::atomic<float>* oversamplingParam = nullptr;

    float lastOn = -1.0f, lastDrive = -100.0f, lastBias = -1.0f, lastOut = -100.0f, lastModel = -1.0f;
    float lastTone = -100.0f, lastSag = -1.0f, lastOversampling = -1.0f;
};