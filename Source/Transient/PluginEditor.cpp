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
      topBar (JucePlugin_Name, "FX-Mechanics Transient Designer", juce::Colours::red),
      transientComponent (p.getTransient(), p.getApvts(), FxmeTransientAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (transientComponent);
    setResizable (true, true);
    setResizeLimits (400, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeTransientAudioProcessorEditor::~FxmeTransientAudioProcessorEditor() = default;

void FxmeTransientAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colours::red);
}

void FxmeTransientAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    transientComponent.setBounds (area);
}
