/*
  ==============================================================================

    CabComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Cab.h"

/** Compact two-channel impulse-response thumbnail. */
class CabIRPlot : public juce::Component
{
public:
    explicit CabIRPlot (Cab& c) : cab (c) {}

    void paint (juce::Graphics& g) override;
    void updateGraph();

private:
    Cab& cab;
    juce::Path leftPath, rightPath;
};

class CabComponent : public juce::Component, public juce::Timer
{
public:
    CabComponent (Cab& cabToControl,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& prefix);
    ~CabComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    Cab& cab;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label        titleLabel;
    juce::ToggleButton onButton;

    juce::Label    irLLabel, irRLabel;
    juce::ComboBox irLBox, irRBox;

    fxme::FxmeSlider gainSlider;
    juce::Label      gainLabel;

    CabIRPlot irPlot;
    std::atomic<bool> graphNeedsUpdate { true };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment>   onAtt;
    std::unique_ptr<ComboBoxAttachment> irLAtt, irRAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label,
                         const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabComponent)
};
