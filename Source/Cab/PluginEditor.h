/*
  ==============================================================================

    Cab plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CabComponent.h"
#include "../Common/TopBar.h"

class FxmeCabAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 600;
    static constexpr int kPreferredHeight = 400 + fxmefx::kTopBarHeight;

    FxmeCabAudioProcessorEditor (FxmeCabAudioProcessor&);
    ~FxmeCabAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeCabAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    CabComponent cabComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeCabAudioProcessorEditor)
};
