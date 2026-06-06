/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeCompressorAudioProcessorEditor::FxmeCompressorAudioProcessorEditor (FxmeCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      compressorComponent (p.getCompressor(), p.getApvts(), FxmeCompressorAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (compressorComponent);
    setResizable (true, true);
    setResizeLimits (400, 300, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeCompressorAudioProcessorEditor::~FxmeCompressorAudioProcessorEditor()
{
}

void FxmeCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeCompressorAudioProcessorEditor::resized()
{
    compressorComponent.setBounds (getLocalBounds());
}
