/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeLimiterAudioProcessorEditor::FxmeLimiterAudioProcessorEditor (FxmeLimiterAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      limiterComponent (p.getLimiter(), p.getApvts(), FxmeLimiterAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (limiterComponent);
    setResizable (true, true);
    setResizeLimits (400, 300, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeLimiterAudioProcessorEditor::~FxmeLimiterAudioProcessorEditor()
{
}

void FxmeLimiterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeLimiterAudioProcessorEditor::resized()
{
    limiterComponent.setBounds (getLocalBounds());
}
