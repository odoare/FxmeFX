/*
  ==============================================================================

    CompressorComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Compressor.h"
#include <FxmeTools/components/GainReductionMeter.h>

class CompressorComponent : public juce::Component,
                            public juce::Timer
{
public:
    CompressorComponent (Compressor& compressorToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, bool showTitle = true, bool compact = false,
                         bool hasSidechain = false);   // the host has an aux input: show Ext SC
    ~CompressorComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    Compressor& compressor;
    juce::AudioProcessorValueTreeState& apvts;
    const bool compact;     // a wide, short slot: no title, flat knobs, one row of controls

    fxme::FxmeButton onButton;
    std::unique_ptr<fxme::FxmeButton> extButton;   // only with a side-chain input (the plugin's own editor)
    juce::Label titleLabel;
    fxme::GainReductionMeter grMeter;
    juce::Label attackLabel, releaseLabel, threshLabel, ratioLabel;
    juce::Label kneeLabel, peakRmsLabel, scHighPassLabel, scLowPassLabel;
    fxme::FxmeSlider preGainSlider, attackSlider, releaseSlider, threshSlider, ratioSlider, gainSlider;
    fxme::FxmeSlider kneeSlider, peakRmsSlider, scHighPassSlider, scLowPassSlider;
    juce::ComboBox relModeBox;

    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> relModeAtt;

    void setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorComponent)
};