/*
  ==============================================================================

    NeuralAmp plugin editor (FxmeNeuralAmp).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto ampTint = NeuralAmpComponent::tint();
}

FxmeNeuralAmpAudioProcessorEditor::FxmeNeuralAmpAudioProcessorEditor (FxmeNeuralAmpAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, ampTint),
      ampComponent (p.getNeuralAmp(), p.getApvts(), FxmeNeuralAmpAudioProcessor::parameterPrefix, false)
{
    // Through the processor, which tells FxmeNet the state changed.
    ampComponent.onLoadFile = [&p] (const juce::File& f, juce::String& error) { return p.loadModelFile (f, error); };
    ampComponent.onClear = [&p] { p.clearModel(); };

    topBar.setPresetBank (p.getPresets());
    addAndMakeVisible (topBar);
    addAndMakeVisible (ampComponent);
    setResizable (true, true);
    setResizeLimits (540, 400 + fxmefx::kTopBarHeight, 1600, 1200);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeNeuralAmpAudioProcessorEditor::~FxmeNeuralAmpAudioProcessorEditor() = default;

void FxmeNeuralAmpAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxme::paintTintedBackground (g, getLocalBounds().toFloat(), ampTint);
}

void FxmeNeuralAmpAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    ampComponent.setBounds (area);
}
