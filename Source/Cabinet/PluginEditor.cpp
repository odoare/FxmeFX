/*
  ==============================================================================

    Cabinet plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeCabinetAudioProcessorEditor::FxmeCabinetAudioProcessorEditor (FxmeCabinetAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      cabinetComponent (p.getCabinet(), p.getApvts(), FxmeCabinetAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (cabinetComponent);
    setResizable (true, true);
    setResizeLimits (480, 320, 1800, 1300);
    setSize (640, 420);
}

FxmeCabinetAudioProcessorEditor::~FxmeCabinetAudioProcessorEditor() = default;

void FxmeCabinetAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeCabinetAudioProcessorEditor::resized()
{
    cabinetComponent.setBounds (getLocalBounds());
}
