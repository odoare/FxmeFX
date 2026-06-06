/*
  ==============================================================================

    Cab plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CabComponent.h"

class FxmeCabAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 600;
    static constexpr int kPreferredHeight = 400;

    FxmeCabAudioProcessorEditor (FxmeCabAudioProcessor&);
    ~FxmeCabAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeCabAudioProcessor& audioProcessor;
    CabComponent cabComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeCabAudioProcessorEditor)
};
