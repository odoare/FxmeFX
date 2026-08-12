/*
  ==============================================================================

    Flanger plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FlangerComponent.h"
#include "../Common/TopBar.h"

class FxmeFlangerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 560;
    static constexpr int kPreferredHeight = 340 + fxmefx::kTopBarHeight;

    FxmeFlangerAudioProcessorEditor (FxmeFlangerAudioProcessor&);
    ~FxmeFlangerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeFlangerAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    FlangerComponent flangerComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeFlangerAudioProcessorEditor)
};
