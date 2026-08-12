/*
  ==============================================================================

    Phaser plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PhaserComponent.h"
#include "../Common/TopBar.h"

class FxmePhaserAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 620;   // three combos in the row
    static constexpr int kPreferredHeight = 340 + fxmefx::kTopBarHeight;

    FxmePhaserAudioProcessorEditor (FxmePhaserAudioProcessor&);
    ~FxmePhaserAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmePhaserAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    PhaserComponent phaserComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmePhaserAudioProcessorEditor)
};
