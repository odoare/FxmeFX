/*
  ==============================================================================

    Flanger plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto flangerTint = juce::Colour::fromRGB (90, 150, 240);
}

FxmeFlangerAudioProcessorEditor::FxmeFlangerAudioProcessorEditor (FxmeFlangerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, "FX-Mechanics Stereo Flanger", flangerTint),
      flangerComponent (p.getFlanger(), p.getApvts(), FxmeFlangerAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (flangerComponent);
    setResizable (true, true);
    setResizeLimits (460, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeFlangerAudioProcessorEditor::~FxmeFlangerAudioProcessorEditor() = default;

void FxmeFlangerAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), flangerTint);
}

void FxmeFlangerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    flangerComponent.setBounds (area);
}
