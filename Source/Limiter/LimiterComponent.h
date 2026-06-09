/*
  ==============================================================================

    LimiterComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Limiter.h"
#include "VuMeterComponent.h"

class LimiterComponent : public juce::Component,
                         public juce::Timer
{
public:
    LimiterComponent (Limiter& limiterToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~LimiterComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    Limiter& limiter;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    VuMeterComponent grMeter;
    fxme::FxmeSlider driveSlider, ceilingSlider, releaseSlider;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> onAtt;

    void setupSlider (fxme::FxmeSlider& slider, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LimiterComponent)
};
