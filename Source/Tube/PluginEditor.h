/*
  ==============================================================================

    Tube plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TubeComponent.h"

class FxmeTubeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 640;
    static constexpr int kPreferredHeight = 300;

    FxmeTubeAudioProcessorEditor (FxmeTubeAudioProcessor&);
    ~FxmeTubeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeTubeAudioProcessor& audioProcessor;
    TubeComponent tubeComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeTubeAudioProcessorEditor)
};
