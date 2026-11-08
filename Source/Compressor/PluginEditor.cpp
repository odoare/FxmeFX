/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "../Common/Theme.h"
#include "PluginEditor.h"

FxmeCompressorAudioProcessorEditor::FxmeCompressorAudioProcessorEditor (FxmeCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, fxmefx::theme::compressor),
      compressorComponent (p.getCompressor(), p.getApvts(), FxmeCompressorAudioProcessor::parameterPrefix, false, false, true)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (compressorComponent);
    setResizable (true, true);
    setResizeLimits (400, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeCompressorAudioProcessorEditor::~FxmeCompressorAudioProcessorEditor()
{
}

void FxmeCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), fxmefx::theme::compressor);
}

void FxmeCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    compressorComponent.setBounds (area);
}
