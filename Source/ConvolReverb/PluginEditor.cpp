/*
  ==============================================================================

    ConvolReverb plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeConvolReverbAudioProcessorEditor::FxmeConvolReverbAudioProcessorEditor (FxmeConvolReverbAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      reverbComponent (p.getConvolReverb(), p.getApvts(), FxmeConvolReverbAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (reverbComponent);
    setResizable (true, true);
    setResizeLimits (500, 350, 1800, 1300);
    setSize (600, 400);
}

FxmeConvolReverbAudioProcessorEditor::~FxmeConvolReverbAudioProcessorEditor() = default;

void FxmeConvolReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeConvolReverbAudioProcessorEditor::resized()
{
    reverbComponent.setBounds (getLocalBounds());
}
