/*
  ==============================================================================

    Oct plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OctComponent.h"
#include "../Common/TopBar.h"

class FxmeOctAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 520;
    static constexpr int kPreferredHeight = 320 + fxmefx::kTopBarHeight;

    FxmeOctAudioProcessorEditor (FxmeOctAudioProcessor&);
    ~FxmeOctAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeOctAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    OctComponent octComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeOctAudioProcessorEditor)
};
