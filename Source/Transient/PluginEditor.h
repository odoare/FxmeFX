/*
  ==============================================================================

    Transient plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TransientComponent.h"
#include "../Common/TopBar.h"

class FxmeTransientAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 480;
    static constexpr int kPreferredHeight = 300 + fxmefx::kTopBarHeight;

    FxmeTransientAudioProcessorEditor (FxmeTransientAudioProcessor&);
    ~FxmeTransientAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeTransientAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    TransientComponent transientComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeTransientAudioProcessorEditor)
};
