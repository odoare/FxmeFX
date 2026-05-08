/*
  ==============================================================================

    Cabinet plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CabinetComponent.h"

class FxmeCabinetAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FxmeCabinetAudioProcessorEditor (FxmeCabinetAudioProcessor&);
    ~FxmeCabinetAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeCabinetAudioProcessor& audioProcessor;
    CabinetComponent cabinetComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeCabinetAudioProcessorEditor)
};
