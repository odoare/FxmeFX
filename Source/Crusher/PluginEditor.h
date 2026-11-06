/*
  ==============================================================================

    Crusher plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CrusherComponent.h"
#include "../Common/TopBar.h"

class FxmeCrusherAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 560;
    static constexpr int kPreferredHeight = 320 + fxmefx::kTopBarHeight;

    FxmeCrusherAudioProcessorEditor (FxmeCrusherAudioProcessor&);
    ~FxmeCrusherAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeCrusherAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    CrusherComponent crusherComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeCrusherAudioProcessorEditor)
};
