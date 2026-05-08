/*
  ==============================================================================

    Equalizer plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EqualizerComponent.h"

class FxmeEqualizerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FxmeEqualizerAudioProcessorEditor (FxmeEqualizerAudioProcessor&);
    ~FxmeEqualizerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeEqualizerAudioProcessor& audioProcessor;
    EqualizerComponent equalizerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeEqualizerAudioProcessorEditor)
};
