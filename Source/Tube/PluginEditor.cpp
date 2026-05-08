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
      tubeComponent (p.getTube(), p.getApvts(), FxmeTubeAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (tubeComponent);
    setResizable (true, true);
    setResizeLimits (400, 300, 1600, 1200);
    setSize (640, 480);
}

FxmeTubeAudioProcessorEditor::~FxmeTubeAudioProcessorEditor() = default;

void FxmeTubeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeTubeAudioProcessorEditor::resized()
{
    tubeComponent.setBounds (getLocalBounds());
}
