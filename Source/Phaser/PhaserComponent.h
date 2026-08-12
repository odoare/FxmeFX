/*
  ==============================================================================

    PhaserComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Phaser.h"

class PhaserComponent : public juce::Component,
                        private juce::Timer
{
public:
    PhaserComponent (Phaser& phaserToControl,
                     juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& prefix,
                     bool showTitle = true);
    ~PhaserComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
                      double min, double max, double interval, const juce::String& suffix);
    void setSliderColours (juce::Slider& s, juce::Colour c);
    void setupCombo (juce::ComboBox& box, juce::Label& label, const juce::String& text,
                     const juce::StringArray& items);

    Phaser& phaser;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label        titleLabel;
    juce::ToggleButton onButton, syncButton;

    juce::ComboBox shapeBox, stagesBox, divBox;
    juce::Label    shapeLabel, stagesLabel, divLabel;

    fxme::FxmeSlider rateSlider, depthSlider, freqSlider, feedbackSlider,
                     widthSlider, mixSlider, outSlider;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment>   onAtt, syncAtt;
    std::unique_ptr<ComboBoxAttachment> shapeAtt, stagesAtt, divAtt;

    std::atomic<float>* syncParam = nullptr;
    bool lastSyncState = false;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserComponent)
};
