/*
  ==============================================================================

    Tape plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto tapeTint = TapeComponent::tint();
}

FxmeTapeAudioProcessorEditor::FxmeTapeAudioProcessorEditor (FxmeTapeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, tapeTint),
      tapeComponent (p.getTape(), p.getApvts(), FxmeTapeAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (tapeComponent);
    setResizable (true, true);
    setResizeLimits (600, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeTapeAudioProcessorEditor::~FxmeTapeAudioProcessorEditor() = default;

void FxmeTapeAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), tapeTint);
}

void FxmeTapeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    tapeComponent.setBounds (area);
}
