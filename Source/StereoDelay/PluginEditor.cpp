/*
  ==============================================================================

    StereoDelay plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeStereoDelayAudioProcessorEditor::FxmeStereoDelayAudioProcessorEditor (FxmeStereoDelayAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      topBar (JucePlugin_Name, "FX-Mechanics Stereo Delay", juce::Colours::green),
      stereoDelayComponent (p.getStereoDelay(), p.getApvts(), FxmeStereoDelayAudioProcessor::parameterPrefix)
{
    addAndMakeVisible (topBar);
    addAndMakeVisible (stereoDelayComponent);
    setResizable (true, true);
    setResizeLimits (500, 350 + fxmefx::kTopBarHeight, 1800, 1300);
    setSize (kPreferredWidth, kPreferredHeight);
}

FxmeStereoDelayAudioProcessorEditor::~FxmeStereoDelayAudioProcessorEditor() = default;

void FxmeStereoDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    fxmefx::paintTintedBackground (g, getLocalBounds().toFloat(), juce::Colours::green);
}

void FxmeStereoDelayAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (fxmefx::kTopBarHeight));
    stereoDelayComponent.setBounds (area);
}
