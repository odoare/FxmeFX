/*
  ==============================================================================

    StereoDelayComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StereoDelay.h"
// #include "ConvolReverbComponent.h" // For FxmeLookAndFeel

class StereoDelayComponent : public juce::Component, public juce::Timer
{
public:
    StereoDelayComponent(StereoDelay& delayToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, bool showTitle = true);
    ~StereoDelayComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    StereoDelay& delay;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label titleLabel;
    fxme::FxmeButton onButton;

    juce::Label bpmLabel;
    fxme::FxmeSlider delayLSlider, delayRSlider;
    juce::Label delayLLabel, delayRLabel;

    // How each side reads its delay value, and what that currently comes to in
    // milliseconds — without the read-out, "0.25" means nothing on its own.
    juce::ComboBox modeLBox, modeRBox;
    juce::Label    modeLLabel, modeRLabel;
    juce::Label    resolvedLabel;
    juce::String   resolvedText;

    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> modeLAtt, modeRAtt;

    void setupCombo (juce::ComboBox& box, juce::Label& label, const juce::String& text);
    juce::String resolvedTimeText() const;

    fxme::FxmeSlider fdbkLSlider, fdbkRSlider;
    juce::Label fdbkLLabel, fdbkRLabel;

    fxme::FxmeSlider crossFdbkSlider;
    juce::Label crossFdbkLabel;

    fxme::FxmeSlider cutoffSlider, qSlider;
    juce::Label cutoffLabel, qLabel;

    fxme::FxmeSlider dryGainSlider, wetGainSlider;
    juce::Label dryGainLabel, wetGainLabel;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setupBarSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setSliderColours(juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoDelayComponent)
};