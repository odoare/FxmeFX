/*
  ==============================================================================

    ConvolReverbComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ConvolReverb.h"

class ImpulseResponsePlot : public juce::Component
{
public:
    ImpulseResponsePlot(ConvolReverb& r);
    void paint(juce::Graphics& g) override;
    void updateGraph();

private:
    ConvolReverb& reverb;
    juce::Path irPlotPathL, irPlotPathR, irMaxPathL, irMaxPathR;
};

class ConvolReverbComponent : public juce::Component, public juce::Timer
{
public:
    ConvolReverbComponent (ConvolReverb& reverbToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~ConvolReverbComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    ConvolReverb& reverb;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label titleLabel;
    juce::ToggleButton onButton;

    juce::ComboBox irBox;
    juce::Label irLabel;
    fxme::FxmeSlider lengthSlider;
    juce::Label lengthLabel;
    fxme::FxmeSlider startOffsetSlider;
    juce::Label startOffsetLabel;
    juce::ComboBox shapeBox;
    juce::Label shapeLabel;
    fxme::FxmeSlider dryGainSlider, wetGainSlider;
    juce::Label dryGainLabel, wetGainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> irAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt;

    ImpulseResponsePlot irPlot;
    std::atomic<bool> graphNeedsUpdate { false };

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    juce::String prefix;
    int externalSlotId = 0;          // ComboBox id of the External entry
    bool userIRPick = false;         // Set on combo mouseDown to distinguish user vs state-driven changes
    std::unique_ptr<juce::FileChooser> chooser;

    struct IRBoxClickWatcher : juce::MouseListener
    {
        ConvolReverbComponent& owner;
        IRBoxClickWatcher (ConvolReverbComponent& o) : owner (o) {}
        void mouseDown (const juce::MouseEvent&) override { owner.userIRPick = true; }
    };
    IRBoxClickWatcher irBoxClickWatcher { *this };

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setupBarSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    void openExternalIRChooser();
    void refreshExternalItemText();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolReverbComponent)
};