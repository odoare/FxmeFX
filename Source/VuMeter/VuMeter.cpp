/*
  ==============================================================================

    VuMeter.cpp
    Created: 2 Feb 2026 9:20:56am
    Author:  doare

  ==============================================================================
*/

#include "VuMeter.h"

void VuMeter::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    setWindowDuration (0.1); 
    resetMax();
}

void VuMeter::setWindowDuration (double duration)
{
    windowSize = juce::jmax (1, juce::roundToInt (duration * currentSampleRate));
    window.assign (windowSize, 0.0f);
    currentSample = 0;
    currentSum = 0.0;
    rms = 0.0f;
    peak = 0.0f;
}

void VuMeter::clear()
{
    rms = 0.0f;
    peak = 0.0f;
    currentSum = 0.0;
    std::fill (window.begin(), window.end(), 0.0f);
}

void VuMeter::process (const float* input, int numSamples)
{
    float localMax = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = input[i];
        float sq = sample * sample;

        // Running sum: subtract old, add new
        float oldSample = window[currentSample];
        currentSum -= (oldSample * oldSample);
        currentSum += sq;
        
        window[currentSample] = sample;
        
        currentSample = (currentSample + 1) % windowSize;

        float absSample = std::abs (sample);
        if (absSample > localMax)
            localMax = absSample;
    }
    
    // Prevent negative sum due to precision errors
    if (currentSum < 0.0) currentSum = 0.0;
    
    // Update atomics once per block
    rms = (float) std::sqrt (currentSum / windowSize);
    peak = localMax;
    
    float currentMax = maxLevel.load();
    if (localMax > currentMax)
        maxLevel.store (localMax);
}
