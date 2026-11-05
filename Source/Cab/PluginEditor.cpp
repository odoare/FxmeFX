/*
  ==============================================================================

    Cab plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "../Common/Theme.h"
#include "PluginEditor.h"

FxmeCabAudioProcessorEditor::FxmeCabAudioProcessorEditor (FxmeCabAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, fxmefx::theme::cab),
      cabComponent (p.getCab(), p.getApvts(), FxmeCabAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (cabComponent);
    setResizable (true, true);
    setResizeLimits (480, 320 + fxmefx::kTopBarHeight, 1800, 1300);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeCabAudioProcessorEditor::~FxmeCabAudioProcessorEditor() = default;

void FxmeCabAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), fxmefx::theme::cab);
}

void FxmeCabAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    cabComponent.setBounds (area);
}
