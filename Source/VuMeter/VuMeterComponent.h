/*
  ==============================================================================

    VuMeterComponent.h
    Created: 2 Feb 2026 9:21:06am
    Author:  doare

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VuMeter.h"

/**
 * @class VuMeterComponent
 * @brief A component that displays a VU meter.
 */
class VuMeterComponent : public juce::Component,
                          public juce::Timer
{
public:
    /** Constructor. */
    VuMeterComponent();
    /** Destructor. */
    ~VuMeterComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /**
     * @brief Sets the current value to display.
     * @param newValue The value in dB.
     */
    void setValue (float newValue);

    /**
     * @brief Sets the color of the meter bar.
     * @param newColor The new color.
     */
    void setMeterColor (juce::Colour newColor);

    /**
     * @brief Sets the display range of the meter.
     * @param newMin Minimum value in dB.
     * @param newMax Maximum value in dB.
     */
    void setRange (float newMin, float newMax);

    /**
     * @brief Sets the zero level mark.
     * @param newZeroLevel The zero level in dB.
     */
    void setZeroLevel (float newZeroLevel);

private:
    float value = -100.0f;
    juce::Colour meterColor = juce::Colours::green;
    float minValue = -60.0f;
    float maxValue = 0.0f;
    float zeroLevel = -100.0f; // -Inf
    
    void timerCallback() override;
};
