/*
  ==============================================================================

    Equalizer plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EqualizerComponent.h"
#include "../Common/TopBar.h"

class FxmeEqualizerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 600;
    static constexpr int kPreferredHeight = 700 + fxmefx::kTopBarHeight;

    FxmeEqualizerAudioProcessorEditor (FxmeEqualizerAudioProcessor&);
    ~FxmeEqualizerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeEqualizerAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    EqualizerComponent equalizerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeEqualizerAudioProcessorEditor)
};
