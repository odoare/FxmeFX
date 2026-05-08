/*
  ==============================================================================

    ConvolReverb plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ConvolReverbComponent.h"

class FxmeConvolReverbAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FxmeConvolReverbAudioProcessorEditor (FxmeConvolReverbAudioProcessor&);
    ~FxmeConvolReverbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeConvolReverbAudioProcessor& audioProcessor;
    ConvolReverbComponent reverbComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeConvolReverbAudioProcessorEditor)
};
