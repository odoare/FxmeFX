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
                      bool showTitle = true,
                      bool compact = false);
    ~CabComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    /** Called after a file was embedded as an external IR (the plugin tells
        FxmeNet that its state changed). */
    std::function<void()> onExternalLoaded;

private:
    Cab& cab;
    juce::AudioProcessorValueTreeState& apvts;
    const juce::String paramPrefix;
    const bool compact;     // a wide, short slot: no title, flat knobs, one row of controls

    juce::Label        titleLabel;
    fxme::FxmeButton onButton;

    juce::Label    irLLabel, irRLabel;
    juce::ComboBox irLBox, irRBox;
    juce::TextButton irLLoad { "Load..." }, irRLoad { "Load..." };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String shownExtL, shownExtR;   // the external items' text, as last set

    fxme::FxmeSlider gainLSlider, gainRSlider;
    juce::Label      gainLLabel, gainRLabel;

    CabIRPlot irLPlot, irRPlot;
    std::atomic<bool> graphNeedsUpdate { true };
    int lastIrGeneration = -1;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> irLAtt, irRAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    juce::ComboBox& boxFor (int channel) { return channel == 0 ? irLBox : irRBox; }
    void refreshExternalItemText();
    void openExternalIRChooser (int channel);

    void setupGainSlider (fxme::FxmeSlider& slider, juce::Label& label,
                          const juce::String& text);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabComponent)
};
