/*
  ==============================================================================

    StereoDelay plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StereoDelayComponent.h"

class FxmeStereoDelayAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FxmeStereoDelayAudioProcessorEditor (FxmeStereoDelayAudioProcessor&);
    ~FxmeStereoDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeStereoDelayAudioProcessor& audioProcessor;
    StereoDelayComponent stereoDelayComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeStereoDelayAudioProcessorEditor)
};
