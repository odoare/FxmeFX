/*
  ==============================================================================

    ChorusComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Chorus.h"

class ChorusComponent : public juce::Component,
                        private juce::Timer
{
public:
    ChorusComponent (Chorus& chorusToControl,
                     juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& prefix,
                     bool showTitle = true);
    ~ChorusComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
                      double min, double max, double interval, const juce::String& suffix);
    void setSliderColours (juce::Slider& s, juce::Colour c);
    void setupCombo (juce::ComboBox& box, juce::Label& label, const juce::String& text,
                     const juce::StringArray& items);

    Chorus& chorus;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label        titleLabel;
    juce::ToggleButton onButton, syncButton;

    juce::ComboBox shapeBox, divBox;
    juce::Label    shapeLabel, divLabel;

    fxme::FxmeSlider rateSlider, depthSlider, delaySlider, voicesSlider,
                     widthSlider, mixSlider, outSlider;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment>   onAtt, syncAtt;
    std::unique_ptr<ComboBoxAttachment> shapeAtt, divAtt;

    std::atomic<float>* syncParam = nullptr;
    bool lastSyncState = false;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusComponent)
};
