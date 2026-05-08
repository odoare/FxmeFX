/*
  ==============================================================================

    StereoDelayComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StereoDelay.h"
// #include "ConvolReverbComponent.h" // For FxmeLookAndFeel

class StereoDelayComponent : public juce::Component, public juce::Timer
{
public:
    StereoDelayComponent(StereoDelay& delayToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~StereoDelayComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    StereoDelay& delay;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label titleLabel;
    juce::ToggleButton onButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;

    juce::Label bpmLabel;
    fxme::FxmeSlider delayLSlider, delayRSlider;
    juce::Label delayLLabel, delayRLabel;

    fxme::FxmeSlider fdbkLSlider, fdbkRSlider;
    juce::Label fdbkLLabel, fdbkRLabel;

    fxme::FxmeSlider crossFdbkSlider;
    juce::Label crossFdbkLabel;

    fxme::FxmeSlider cutoffSlider, qSlider;
    juce::Label cutoffLabel, qLabel;

    fxme::FxmeSlider dryGainSlider, wetGainSlider;
    juce::Label dryGainLabel, wetGainLabel;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setupBarSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setSliderColours(juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoDelayComponent)
};