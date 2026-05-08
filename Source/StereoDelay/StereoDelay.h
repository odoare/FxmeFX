/*
  ==============================================================================

    StereoDelay.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>

class StereoDelay
{
public:
    StereoDelay();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);

    void assignParameters(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);
    void setOn (bool shouldBeOn);
    bool isOn() const { return on; }
    void setBPM(double bpm);
    double getBPM() const { return currentBPM; }

private:
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
        void reset() { z1 = 0.0f; z2 = 0.0f; }
        float process(float in);
    };

    void updateDelayTimes();
    void updateGains();
    void updateFilter();
    void calcLowPass(Biquad& bq, float f, float q);

    double currentSampleRate = 44100.0;
    int maxDelaySamples = 0;

    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;

    Biquad filterL, filterR;

    // Parameter cache
    float delayTimeLMs = 0.5f, delayTimeRMs = 0.5f; // Now in beats
    float feedbackL = -6.0f, feedbackR = -6.0f;
    float crossFeedback = -60.0f;
    float filterCutoff = 5000.0f, filterQ = 1.0f;
    float dryGain = 0.0f;
    float wetGain = -6.0f;
    bool on = true;

    // Smoothed gains
    double currentBPM = 120.0;
    float feedbackLGain = 0.0f, feedbackRGain = 0.0f;
    float crossFeedbackGain = 0.0f;
    float dryGainLinear = 1.0f, wetGainLinear = 0.5f;

    // Delay times in samples
    int delaySamplesL = 0, delaySamplesR = 0;

    // Parameter Pointers
    std::atomic<float>* delayLParam = nullptr;
    std::atomic<float>* delayRParam = nullptr;
    std::atomic<float>* fdbkLParam = nullptr;
    std::atomic<float>* fdbkRParam = nullptr;
    std::atomic<float>* crossFdbkParam = nullptr;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* qParam = nullptr;
    std::atomic<float>* dryGainParam = nullptr;
    std::atomic<float>* wetGainParam = nullptr;
    std::atomic<float>* onParam = nullptr;

    // Last values for change detection
    float lastDelayL = -1.0f, lastDelayR = -1.0f;
    float lastFdbkL = -100.0f, lastFdbkR = -100.0f;
    float lastCrossFdbk = -100.0f;
    float lastCutoff = -1.0f, lastQ = -1.0f;
    float lastDryGain = -100.0f;
    float lastWetGain = -100.0f;
    float lastOn = -1.0f;
};