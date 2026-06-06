/*
  ==============================================================================

    Transient plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TransientComponent.h"

class FxmeTransientAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 480;
    static constexpr int kPreferredHeight = 300;

    FxmeTransientAudioProcessorEditor (FxmeTransientAudioProcessor&);
    ~FxmeTransientAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeTransientAudioProcessor& audioProcessor;
    TransientComponent transientComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeTransientAudioProcessorEditor)
};
