/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#ifndef FXME_PD_BUILD
 #include "../Common/EffectPresets.h"
#endif
#include "Compressor.h"

class FxmeCompressorAudioProcessor  : public juce::AudioProcessor
{
public:
    static constexpr const char* parameterPrefix = "Main";

    FxmeCompressorAudioProcessor();
    ~FxmeCompressorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return apvts; }

   #ifndef FXME_PD_BUILD
    /** The effect's presets (module presets, shared with every plugin
        embedding it; see Common/EffectPresets.h). */
    fxme::ModulePresetTarget& getPresets() noexcept { return presets.target; }
   #endif
    Compressor& getCompressor() { return compressor; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

   #ifndef FXME_PD_BUILD
    fxmefx::EffectPresets presets { apvts, "Compressor", juce::String (parameterPrefix) + "_Comp_" };
   #endif
    Compressor compressor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeCompressorAudioProcessor)
};
