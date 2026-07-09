/*
  ==============================================================================

    Cab plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeCabAudioProcessorEditor::FxmeCabAudioProcessorEditor (FxmeCabAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, "FX-Mechanics Cab IR loader", juce::Colours::orange),
      cabComponent (p.getCab(), p.getApvts(), FxmeCabAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (cabComponent);
    setResizable (true, true);
    setResizeLimits (480, 320 + fxmefx::kTopBarHeight, 1800, 1300);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeCabAudioProcessorEditor::~FxmeCabAudioProcessorEditor() = default;

void FxmeCabAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colours::orange);
}

void FxmeCabAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    cabComponent.setBounds (area);
}
