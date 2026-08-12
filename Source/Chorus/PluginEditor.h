/*
  ==============================================================================

    Chorus plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ChorusComponent.h"
#include "../Common/TopBar.h"

class FxmeChorusAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 560;
    static constexpr int kPreferredHeight = 340 + fxmefx::kTopBarHeight;

    FxmeChorusAudioProcessorEditor (FxmeChorusAudioProcessor&);
    ~FxmeChorusAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeChorusAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    ChorusComponent chorusComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeChorusAudioProcessorEditor)
};
