/*
  ==============================================================================

    Phaser plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "../Common/Theme.h"
#include "PluginEditor.h"

namespace
{
    const auto phaserTint = fxmefx::theme::phaser;
}

FxmePhaserAudioProcessorEditor::FxmePhaserAudioProcessorEditor (FxmePhaserAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, phaserTint),
      phaserComponent (p.getPhaser(), p.getApvts(), FxmePhaserAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (phaserComponent);
    setResizable (true, true);
    setResizeLimits (520, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmePhaserAudioProcessorEditor::~FxmePhaserAudioProcessorEditor() = default;

void FxmePhaserAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), phaserTint);
}

void FxmePhaserAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    phaserComponent.setBounds (area);
}
