/*
  ==============================================================================

    CabComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Cab.h"

/** Compact impulse-response thumbnail for one channel/slot. */
class CabIRPlot : public juce::Component
{
public:
    CabIRPlot (Cab& c, int channelToShow, juce::Colour colour)
        : cab (c), channel (channelToShow), tint (colour) {}

    void paint (juce::Graphics& g) override;
    void updateGraph();

private:
    Cab& cab;
    int channel;
    juce::Colour tint;
    juce::Path path;
};

class CabComponent : public juce::Component, public juce::Timer
{
public:
    CabComponent (Cab& cabToControl,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& prefix,
                      bool showTitle = true);
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

    fxme::FxmeSlider gainLSlider, gainRSlider;
    juce::Label      gainLLabel, gainRLabel;

    CabIRPlot irLPlot, irRPlot;
    std::atomic<bool> graphNeedsUpdate { true };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment>   onAtt;
    std::unique_ptr<ComboBoxAttachment> irLAtt, irRAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setupGainSlider (fxme::FxmeSlider& slider, juce::Label& label,
                          const juce::String& text);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabComponent)
};
