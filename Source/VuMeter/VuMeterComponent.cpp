/*
  ==============================================================================

    VuMeterComponent.cpp
    Created: 2 Feb 2026 9:21:06am
    Author:  doare

  ==============================================================================
*/

#include "VuMeterComponent.h"

VuMeterComponent::VuMeterComponent()
{
    startTimerHz (30);
}

VuMeterComponent::~VuMeterComponent()
{
    stopTimer();
}

void VuMeterComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    float displayValue = juce::jlimit (minValue, maxValue, value);
    float proportion = juce::jmap (displayValue, minValue, maxValue, 0.0f, 1.0f);

    g.setColour (meterColor);

    if (horizontal)
    {
        int width = juce::roundToInt (proportion * getWidth());
        g.fillRect (0, 0, width, getHeight());

        // Zero Level (vertical mark).
        if (zeroLevel > minValue && zeroLevel < maxValue)
        {
            float zeroProp = juce::jmap (zeroLevel, minValue, maxValue, 0.0f, 1.0f);
            int zeroX = juce::roundToInt (zeroProp * getWidth());
            g.setColour (juce::Colours::white);
            g.drawLine ((float) zeroX, 0.0f, (float) zeroX, (float) getHeight(), 1.0f);
        }
    }
    else
    {
        int height = juce::roundToInt (proportion * getHeight());
        g.fillRect (0, getHeight() - height, getWidth(), height);

        // Zero Level (horizontal mark).
        if (zeroLevel > minValue && zeroLevel < maxValue)
        {
            float zeroProp = juce::jmap (zeroLevel, minValue, maxValue, 0.0f, 1.0f);
            int zeroY = juce::roundToInt ((1.0f - zeroProp) * getHeight());
            g.setColour (juce::Colours::white);
            g.drawLine (0, (float) zeroY, (float) getWidth(), (float) zeroY, 1.0f);
        }
    }
}

void VuMeterComponent::resized()
{
}

void VuMeterComponent::setValue (float newValue)
{
    value = newValue;
}

void VuMeterComponent::setMeterColor (juce::Colour newColor)
{
    meterColor = newColor;
}

void VuMeterComponent::setRange (float newMin, float newMax)
{
    minValue = newMin;
    maxValue = newMax;
}

void VuMeterComponent::setZeroLevel (float newZeroLevel)
{
    zeroLevel = newZeroLevel;
}

void VuMeterComponent::setHorizontal (bool shouldBeHorizontal)
{
    horizontal = shouldBeHorizontal;
}

void VuMeterComponent::timerCallback()
{
    repaint();
}
