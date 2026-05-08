/*
  ==============================================================================

    Transient plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeTransientAudioProcessorEditor::FxmeTransientAudioProcessorEditor (FxmeTransientAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      transientComponent (p.getTransient(), p.getApvts(), FxmeTransientAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (transientComponent);
    setResizable (true, true);
    setResizeLimits (400, 300, 1600, 1200);
    setSize (640, 480);
}

FxmeTransientAudioProcessorEditor::~FxmeTransientAudioProcessorEditor() = default;

void FxmeTransientAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeTransientAudioProcessorEditor::resized()
{
    transientComponent.setBounds (getLocalBounds());
}
