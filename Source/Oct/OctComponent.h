/*
  ==============================================================================

    OctComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Oct.h"

class OctComponent : public juce::Component
{
public:
    OctComponent (Oct& octToControl,
                  juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& prefix);
    ~OctComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    Oct& oct;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label        titleLabel;
    juce::ToggleButton onButton;

    fxme::FxmeSlider drySlider, oct1Slider, oct2Slider;
    fxme::FxmeSlider detectSlider, toneSlider;
    juce::Label      dryLabel,  oct1Label,  oct2Label,
                     detectLabel, toneLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> onAtt;

    void setupRotary (fxme::FxmeSlider& slider, juce::Label& label,
                      const juce::String& text, double min, double max, double def,
                      const juce::String& suffix);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctComponent)
};
