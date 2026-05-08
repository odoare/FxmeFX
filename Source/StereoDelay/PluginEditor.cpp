/*
  ==============================================================================

    StereoDelay plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeStereoDelayAudioProcessorEditor::FxmeStereoDelayAudioProcessorEditor (FxmeStereoDelayAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      stereoDelayComponent (p.getStereoDelay(), p.getApvts(), FxmeStereoDelayAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (stereoDelayComponent);
    setResizable (true, true);
    setResizeLimits (500, 350, 1800, 1300);
    setSize (600, 300);
}

FxmeStereoDelayAudioProcessorEditor::~FxmeStereoDelayAudioProcessorEditor() = default;

void FxmeStereoDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeStereoDelayAudioProcessorEditor::resized()
{
    stereoDelayComponent.setBounds (getLocalBounds());
}
