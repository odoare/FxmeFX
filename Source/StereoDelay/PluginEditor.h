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
    static constexpr int kPreferredWidth  = 600;
    static constexpr int kPreferredHeight = 300;

    FxmeStereoDelayAudioProcessorEditor (FxmeStereoDelayAudioProcessor&);
    ~FxmeStereoDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeStereoDelayAudioProcessor& audioProcessor;
    StereoDelayComponent stereoDelayComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeStereoDelayAudioProcessorEditor)
};
