/*
  ==============================================================================

    Oct plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OctComponent.h"

class FxmeOctAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FxmeOctAudioProcessorEditor (FxmeOctAudioProcessor&);
    ~FxmeOctAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeOctAudioProcessor& audioProcessor;
    OctComponent octComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeOctAudioProcessorEditor)
};
