/*
  ==============================================================================

    NeuralAmp plugin editor (FxmeNeuralAmp).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NeuralAmpComponent.h"
#include "../Common/TopBar.h"

class FxmeNeuralAmpAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    static constexpr int kPreferredWidth  = 580;
    static constexpr int kPreferredHeight = 420 + fxmefx::kTopBarHeight;

    FxmeNeuralAmpAudioProcessorEditor (FxmeNeuralAmpAudioProcessor&);
    ~FxmeNeuralAmpAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeNeuralAmpAudioProcessor& audioProcessor;
    fxmefx::TopBar topBar;
    NeuralAmpComponent ampComponent;

    // FxmeSlider's right-click value entry puts a TextEditor on screen, so the
    // editor owns one focus fixer (see FxmeTools/components/TextEntryFocusFixer.h).
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeNeuralAmpAudioProcessorEditor)
};
