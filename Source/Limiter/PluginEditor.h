/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LimiterComponent.h"
#include "../Common/TopBar.h"

class FxmeLimiterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 600;
    static constexpr int kPreferredHeight = 300 + fxmefx::kTopBarHeight;

    FxmeLimiterAudioProcessorEditor (FxmeLimiterAudioProcessor&);
    ~FxmeLimiterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeLimiterAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    LimiterComponent limiterComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeLimiterAudioProcessorEditor)
};
