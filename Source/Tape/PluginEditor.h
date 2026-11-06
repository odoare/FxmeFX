/*
  ==============================================================================

    Tape plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TapeComponent.h"
#include "../Common/TopBar.h"

class FxmeTapeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 700;
    static constexpr int kPreferredHeight = 340 + fxmefx::kTopBarHeight;

    FxmeTapeAudioProcessorEditor (FxmeTapeAudioProcessor&);
    ~FxmeTapeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeTapeAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    TapeComponent tapeComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeTapeAudioProcessorEditor)
};
