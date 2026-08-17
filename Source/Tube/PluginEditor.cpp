/*
  ==============================================================================

    Tube plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeTubeAudioProcessorEditor::FxmeTubeAudioProcessorEditor (FxmeTubeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, "FX-Mechanics Tube saturation", juce::Colours::orange),
      tubeComponent (p.getTube(), p.getApvts(), FxmeTubeAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (tubeComponent);
    setResizable (true, true);
    setResizeLimits (400, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeTubeAudioProcessorEditor::~FxmeTubeAudioProcessorEditor() = default;

void FxmeTubeAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colours::orange);
}

void FxmeTubeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    tubeComponent.setBounds (area);
}
