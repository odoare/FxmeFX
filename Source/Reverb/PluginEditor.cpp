/*
  ==============================================================================

    Reverb plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto reverbTint = ReverbComponent::tint();
}

FxmeReverbAudioProcessorEditor::FxmeReverbAudioProcessorEditor (FxmeReverbAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, reverbTint),
      reverbComponent (p.getReverb(), p.getApvts(), FxmeReverbAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (reverbComponent);
    setResizable (true, true);
    setResizeLimits (460, 300 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeReverbAudioProcessorEditor::~FxmeReverbAudioProcessorEditor() = default;

void FxmeReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), reverbTint);
}

void FxmeReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    reverbComponent.setBounds (area);
}
