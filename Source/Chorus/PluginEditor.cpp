/*
  ==============================================================================

    Chorus plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto chorusTint = juce::Colour::fromRGB (60, 200, 190);
}

FxmeChorusAudioProcessorEditor::FxmeChorusAudioProcessorEditor (FxmeChorusAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, "FX-Mechanics Stereo Chorus", chorusTint),
      chorusComponent (p.getChorus(), p.getApvts(), FxmeChorusAudioProcessor::parameterPrefix, false)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (chorusComponent);
    setResizable (true, true);
    setResizeLimits (460, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeChorusAudioProcessorEditor::~FxmeChorusAudioProcessorEditor() = default;

void FxmeChorusAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), chorusTint);
}

void FxmeChorusAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    chorusComponent.setBounds (area);
}
