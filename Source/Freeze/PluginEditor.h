/*
  ==============================================================================

    Freeze plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FreezeComponent.h"
#include "../Common/TopBar.h"

class FxmeFreezeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 520;
    static constexpr int kPreferredHeight = 360 + fxmefx::kTopBarHeight;

    FxmeFreezeAudioProcessorEditor (FxmeFreezeAudioProcessor&);
    ~FxmeFreezeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeFreezeAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    FreezeComponent freezeComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeFreezeAudioProcessorEditor)
};
