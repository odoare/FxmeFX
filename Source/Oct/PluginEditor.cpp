/*
  ==============================================================================

    Oct plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeOctAudioProcessorEditor::FxmeOctAudioProcessorEditor (FxmeOctAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      octComponent (p.getOct(), p.getApvts(), FxmeOctAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (octComponent);
    setResizable (true, true);
    setResizeLimits (400, 280, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeOctAudioProcessorEditor::~FxmeOctAudioProcessorEditor() = default;

void FxmeOctAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeOctAudioProcessorEditor::resized()
{
    octComponent.setBounds (getLocalBounds());
}
