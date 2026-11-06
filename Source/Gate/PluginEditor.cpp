/*
  ==============================================================================

    Gate plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto gateTint = GateComponent::tint();
}

FxmeGateAudioProcessorEditor::FxmeGateAudioProcessorEditor (FxmeGateAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, gateTint),
      gateComponent (p.getGate(), p.getApvts(), FxmeGateAudioProcessor::parameterPrefix, false)
{
    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (gateComponent);
    setResizable (true, true);
    setResizeLimits (560, 320 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeGateAudioProcessorEditor::~FxmeGateAudioProcessorEditor() = default;

void FxmeGateAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), gateTint);
}

void FxmeGateAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    gateComponent.setBounds (area);
}
