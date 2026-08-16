/*
  ==============================================================================

    Oct plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OctComponent.h"
#include "../Common/TopBar.h"

class FxmeOctAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 520;
    static constexpr int kPreferredHeight = 320 + fxmefx::kTopBarHeight;

    FxmeOctAudioProcessorEditor (FxmeOctAudioProcessor&);
    ~FxmeOctAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeOctAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    OctComponent octComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeOctAudioProcessorEditor)
};
