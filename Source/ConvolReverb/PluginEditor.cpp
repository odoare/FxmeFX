/*
  ==============================================================================

    ConvolReverb plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "../Common/Theme.h"
#include "PluginEditor.h"

FxmeConvolReverbAudioProcessorEditor::FxmeConvolReverbAudioProcessorEditor (FxmeConvolReverbAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, fxmefx::theme::convolReverb),
      reverbComponent (p.getConvolReverb(), p.getApvts(), FxmeConvolReverbAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (reverbComponent);
    setResizable (true, true);
    setResizeLimits (500, 350 + fxmefx::kTopBarHeight, 1800, 1300);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeConvolReverbAudioProcessorEditor::~FxmeConvolReverbAudioProcessorEditor() = default;

void FxmeConvolReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), fxmefx::theme::convolReverb);
}

void FxmeConvolReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    reverbComponent.setBounds (area);
}
