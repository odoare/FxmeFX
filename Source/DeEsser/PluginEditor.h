/*
  ==============================================================================

    De-esser plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DeEsserComponent.h"
#include "../Common/TopBar.h"

class FxmeDeEsserAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 760;
    static constexpr int kPreferredHeight = 380 + fxmefx::kTopBarHeight;

    FxmeDeEsserAudioProcessorEditor (FxmeDeEsserAudioProcessor&);
    ~FxmeDeEsserAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeDeEsserAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    DeEsserComponent deEsserComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeDeEsserAudioProcessorEditor)
};
