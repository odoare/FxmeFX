/*
  ==============================================================================

    De-esser plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto deEsserTint = DeEsserComponent::tint();
}

FxmeDeEsserAudioProcessorEditor::FxmeDeEsserAudioProcessorEditor (FxmeDeEsserAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, deEsserTint),
      deEsserComponent (p.getDeEsser(), p.getApvts(), FxmeDeEsserAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (deEsserComponent);
    setResizable (true, true);
    setResizeLimits (600, 320 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeDeEsserAudioProcessorEditor::~FxmeDeEsserAudioProcessorEditor() = default;

void FxmeDeEsserAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), deEsserTint);
}

void FxmeDeEsserAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    deEsserComponent.setBounds (area);
}
