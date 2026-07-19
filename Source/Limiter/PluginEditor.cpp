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
      topBar (JucePlugin_Name, "FX-Mechanics look-ahead limiter / maximizer", juce::Colours::orange),
      limiterComponent (p.getLimiter(), p.getApvts(), FxmeLimiterAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (limiterComponent);
    setResizable (true, true);
    setResizeLimits (400, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeLimiterAudioProcessorEditor::~FxmeLimiterAudioProcessorEditor()
{
}

void FxmeLimiterAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colours::orange);
}

void FxmeLimiterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    limiterComponent.setBounds (area);
}
