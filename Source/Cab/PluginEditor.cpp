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
      cabComponent (p.getCab(), p.getApvts(), FxmeCabAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (cabComponent);
    setResizable (true, true);
    setResizeLimits (480, 320, 1800, 1300);
    setSize (640, 420);
}

FxmeCabAudioProcessorEditor::~FxmeCabAudioProcessorEditor() = default;

void FxmeCabAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeCabAudioProcessorEditor::resized()
{
    cabComponent.setBounds (getLocalBounds());
}
