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
      topBar (JucePlugin_Name, "FX-Mechanics Octaver", juce::Colour::fromRGB (140, 100, 220)),
      octComponent (p.getOct(), p.getApvts(), FxmeOctAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (octComponent);
    setResizable (true, true);
    setResizeLimits (400, 280 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeOctAudioProcessorEditor::~FxmeOctAudioProcessorEditor() = default;

void FxmeOctAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colour::fromRGB (140, 100, 220));
}

void FxmeOctAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    octComponent.setBounds (area);
}
