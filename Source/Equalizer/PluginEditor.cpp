/*
  ==============================================================================

    Equalizer plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "../Common/Theme.h"
#include "PluginEditor.h"

FxmeEqualizerAudioProcessorEditor::FxmeEqualizerAudioProcessorEditor (FxmeEqualizerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, fxmefx::theme::equalizer),
      equalizerComponent (p.getEqualizer(), p.getApvts(), FxmeEqualizerAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (equalizerComponent);
    setResizable (true, true);
    setResizeLimits (600, 360 + fxmefx::kTopBarHeight, 2000, 1400);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeEqualizerAudioProcessorEditor::~FxmeEqualizerAudioProcessorEditor() = default;

void FxmeEqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), fxmefx::theme::equalizer);
}

void FxmeEqualizerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    equalizerComponent.setBounds (area);
}
