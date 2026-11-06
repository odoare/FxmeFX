/*
  ==============================================================================

    Gate plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GateComponent.h"
#include "../Common/TopBar.h"

class FxmeGateAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 720;
    static constexpr int kPreferredHeight = 360 + fxmefx::kTopBarHeight;

    FxmeGateAudioProcessorEditor (FxmeGateAudioProcessor&);
    ~FxmeGateAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeGateAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    GateComponent gateComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeGateAudioProcessorEditor)
};
