/*
  ==============================================================================

    CabinetComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Cabinet.h"

/** Compact two-channel impulse-response thumbnail. */
class CabinetIRPlot : public juce::Component
{
public:
    explicit CabinetIRPlot (Cabinet& c) : cabinet (c) {}

    void paint (juce::Graphics& g) override;
    void updateGraph();

private:
    Cabinet& cabinet;
    juce::Path leftPath, rightPath;
};

class CabinetComponent : public juce::Component, public juce::Timer
{
public:
    CabinetComponent (Cabinet& cabinetToControl,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& prefix);
    ~CabinetComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    Cabinet& cabinet;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label        titleLabel;
    juce::ToggleButton onButton;

    juce::Label    irLLabel, irRLabel;
    juce::ComboBox irLBox, irRBox;

    fxme::FxmeSlider gainSlider;
    juce::Label      gainLabel;

    CabinetIRPlot irPlot;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetComponent)
};
