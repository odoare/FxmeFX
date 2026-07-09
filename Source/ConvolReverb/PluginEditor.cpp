/*
  ==============================================================================

    ConvolReverb plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeConvolReverbAudioProcessorEditor::FxmeConvolReverbAudioProcessorEditor (FxmeConvolReverbAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, "FX-Mechanics Convolution Reverb", juce::Colours::yellowgreen),
      reverbComponent (p.getConvolReverb(), p.getApvts(), FxmeConvolReverbAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (reverbComponent);
    setResizable (true, true);
    setResizeLimits (500, 350 + fxmefx::kTopBarHeight, 1800, 1300);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeConvolReverbAudioProcessorEditor::~FxmeConvolReverbAudioProcessorEditor() = default;

void FxmeConvolReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colours::yellowgreen);
}

void FxmeConvolReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    reverbComponent.setBounds (area);
}
