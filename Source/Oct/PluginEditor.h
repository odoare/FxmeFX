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
    static constexpr int kPreferredWidth  = 520;
    static constexpr int kPreferredHeight = 320;

    FxmeOctAudioProcessorEditor (FxmeOctAudioProcessor&);
    ~FxmeOctAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeOctAudioProcessor& audioProcessor;
    OctComponent octComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeOctAudioProcessorEditor)
};
