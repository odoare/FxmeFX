/*
  ==============================================================================

    TransientComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Transient.h"
#include "VuMeterComponent.h"

class TransientComponent : public juce::Component,
                           public juce::Timer
{
public:
    TransientComponent (Transient& transientToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~TransientComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    Transient& transientFx;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    juce::ComboBox characterBox;
    VuMeterComponent gainMeter;
    juce::Label preGainLabel, attackLabel, sustainLabel, gainLabel;
    fxme::FxmeSlider preGainSlider, attackSlider, sustainSlider, gainSlider;

    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ButtonAttachment>   onAtt;
    std::unique_ptr<ComboBoxAttachment> characterAtt;

    void setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientComponent)
};
