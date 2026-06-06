/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CompressorComponent.h"

class FxmeCompressorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 600;
    static constexpr int kPreferredHeight = 300;

    FxmeCompressorAudioProcessorEditor (FxmeCompressorAudioProcessor&);
    ~FxmeCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeCompressorAudioProcessor& audioProcessor;
    CompressorComponent compressorComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeCompressorAudioProcessorEditor)
};
