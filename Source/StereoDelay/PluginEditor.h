/*
  ==============================================================================

    StereoDelay plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StereoDelayComponent.h"
#include "../Common/TopBar.h"

class FxmeStereoDelayAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 600;
    static constexpr int kPreferredHeight = 300 + fxmefx::kTopBarHeight;

    FxmeStereoDelayAudioProcessorEditor (FxmeStereoDelayAudioProcessor&);
    ~FxmeStereoDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeStereoDelayAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    StereoDelayComponent stereoDelayComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeStereoDelayAudioProcessorEditor)
};
