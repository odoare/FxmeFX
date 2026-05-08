/*
  ==============================================================================

    VuMeter.h
    Created: 2 Feb 2026 9:20:55am
    Author:  doare

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>

class VuMeter
{
public:
    VuMeter() = default;

    void prepare (double sampleRate);
    void process (const float* input, int numSamples);

    // Getters return dB values
    float getRMS() const { return juce::Decibels::gainToDecibels (rms.load()); }
    float getPeak() const { return juce::Decibels::gainToDecibels (peak.load()); }
    float getMax() const { return juce::Decibels::gainToDecibels (maxLevel.load()); }
    void resetMax() { maxLevel = 0.0f; }
    void clear();

    void setWindowDuration (double duration);

private:
    double currentSampleRate = 44100.0;
    int windowSize = 1;
    int currentSample = 0;
    
    // Store linear values
    std::atomic<float> rms { 0.0f };
    std::atomic<float> peak { 0.0f };
    std::atomic<float> maxLevel { 0.0f };
    
    std::vector<float> window;
    double currentSum = 0.0;
};
