/*
  ==============================================================================

    Equalizer plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeEqualizerAudioProcessorEditor::FxmeEqualizerAudioProcessorEditor (FxmeEqualizerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      equalizerComponent (p.getEqualizer(), p.getApvts(), FxmeEqualizerAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (equalizerComponent);
    setResizable (true, true);
    setResizeLimits (600, 360, 2000, 1400);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeEqualizerAudioProcessorEditor::~FxmeEqualizerAudioProcessorEditor() = default;

void FxmeEqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void FxmeEqualizerAudioProcessorEditor::resized()
{
    equalizerComponent.setBounds (getLocalBounds());
}
