/*
  ==============================================================================

    Crusher plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto crusherTint = CrusherComponent::tint();
}

FxmeCrusherAudioProcessorEditor::FxmeCrusherAudioProcessorEditor (FxmeCrusherAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, crusherTint),
      crusherComponent (p.getCrusher(), p.getApvts(), FxmeCrusherAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (crusherComponent);
    setResizable (true, true);
    setResizeLimits (480, 280 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeCrusherAudioProcessorEditor::~FxmeCrusherAudioProcessorEditor() = default;

void FxmeCrusherAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), crusherTint);
}

void FxmeCrusherAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    crusherComponent.setBounds (area);
}
