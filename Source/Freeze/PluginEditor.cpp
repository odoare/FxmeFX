/*
  ==============================================================================

    Freeze plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "../Common/Theme.h"
#include "PluginEditor.h"

FxmeFreezeAudioProcessorEditor::FxmeFreezeAudioProcessorEditor (FxmeFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, fxmefx::theme::freeze),
      freezeComponent (p.getFreeze(), p.getApvts(), FxmeFreezeAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (freezeComponent);
    setResizable (true, true);
    setResizeLimits (400, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeFreezeAudioProcessorEditor::~FxmeFreezeAudioProcessorEditor() = default;

void FxmeFreezeAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), fxmefx::theme::freeze);
}

void FxmeFreezeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    freezeComponent.setBounds (area);
}
