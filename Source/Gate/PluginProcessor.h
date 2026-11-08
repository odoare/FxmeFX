/*
  ==============================================================================

    Gate plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#ifndef FXME_PD_BUILD
 #include "../Common/EffectPresets.h"
 #include <FxmeTools/net/NetBridge.h>   // FxmeNet
#endif
#include "Gate.h"

class FxmeGateAudioProcessor  : public juce::AudioProcessor
{
public:
    static constexpr const char* parameterPrefix = "Main";

    FxmeGateAudioProcessor();
    ~FxmeGateAudioProcessor() override;

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
    // FxmeNet: the server and the client find the bridge through the VST3 component.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &net; }
   #endif

   #ifndef FXME_PD_BUILD
    /** The effect's presets (module presets, shared with every plugin
        embedding it; see Common/EffectPresets.h). */
    fxme::ModulePresetTarget& getPresets() noexcept { return presets.target; }
   #endif
    Gate& getGate() { return gate; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

   #ifndef FXME_PD_BUILD
    fxmefx::EffectPresets presets { apvts, "Gate", juce::String (parameterPrefix) + "_Gate_",
                                   [] (const juce::String& key) { return key != "ExtSc"; } };   // routing, not sound
   #endif
    Gate gate;

   #ifndef FXME_PD_BUILD
    fxme::net::NetBridge net;   // FxmeNet: declared after what it registers
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeGateAudioProcessor)
};
