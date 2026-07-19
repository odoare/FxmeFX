/*
  ==============================================================================

    Freeze plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeFreezeAudioProcessorEditor::FxmeFreezeAudioProcessorEditor (FxmeFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, "FX-Mechanics Spectral Freeze", juce::Colour::fromRGB (140, 100, 220)),
      freezeComponent (p.getFreeze(), p.getApvts(), FxmeFreezeAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (freezeComponent);
    setResizable (true, true);
    setResizeLimits (400, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeFreezeAudioProcessorEditor::~FxmeFreezeAudioProcessorEditor() = default;

void FxmeFreezeAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colour::fromRGB (140, 100, 220));
}

void FxmeFreezeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    freezeComponent.setBounds (area);
}
