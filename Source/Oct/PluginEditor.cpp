/*
  ==============================================================================

    Oct plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "../Common/Theme.h"
#include "PluginEditor.h"

FxmeOctAudioProcessorEditor::FxmeOctAudioProcessorEditor (FxmeOctAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, fxmefx::theme::oct),
      octComponent (p.getOct(), p.getApvts(), FxmeOctAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (octComponent);
    setResizable (true, true);
    setResizeLimits (400, 280 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeOctAudioProcessorEditor::~FxmeOctAudioProcessorEditor() = default;

void FxmeOctAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), fxmefx::theme::oct);
}

void FxmeOctAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    octComponent.setBounds (area);
}
