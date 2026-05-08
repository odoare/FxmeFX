/*
  ==============================================================================

    CompressorComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Compressor.h"
#include "VuMeterComponent.h"

class CompressorComponent : public juce::Component,
                            public juce::Timer
{
public:
    CompressorComponent (Compressor& compressorToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~CompressorComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    Compressor& compressor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    VuMeterComponent grMeter;
    juce::Label preGainLabel, attackLabel, releaseLabel, threshLabel, ratioLabel, gainLabel;
    juce::Label kneeLabel, peakRmsLabel;
    fxme::FxmeSlider preGainSlider, attackSlider, releaseSlider, threshSlider, ratioSlider, gainSlider;
    fxme::FxmeSlider kneeSlider, peakRmsSlider;
    juce::ComboBox relModeBox;

    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment>   onAtt;
    std::unique_ptr<ComboBoxAttachment> relModeAtt;

    void setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorComponent)
};