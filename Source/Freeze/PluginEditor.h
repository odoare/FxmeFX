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

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeFreezeAudioProcessorEditor)
};
