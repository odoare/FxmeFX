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
      topBar (JucePlugin_Name, "FX-Mechanics 5-band Equalizer", juce::Colours::cyan),
      equalizerComponent (p.getEqualizer(), p.getApvts(), FxmeEqualizerAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (equalizerComponent);
    setResizable (true, true);
    setResizeLimits (600, 360 + fxmefx::kTopBarHeight, 2000, 1400);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeEqualizerAudioProcessorEditor::~FxmeEqualizerAudioProcessorEditor() = default;

void FxmeEqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colours::cyan);
}

void FxmeEqualizerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    equalizerComponent.setBounds (area);
}
