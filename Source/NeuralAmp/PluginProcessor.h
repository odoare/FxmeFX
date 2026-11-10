/*
  ==============================================================================

    NeuralAmp plugin processor (FxmeNeuralAmp). No Pure Data external:
    loading a model needs a message the Pd host does not have.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Common/EffectPresets.h"
#include <FxmeTools/net/NetBridge.h>   // FxmeNet
#include "NeuralAmp.h"

class FxmeNeuralAmpAudioProcessor  : public juce::AudioProcessor
{
public:
    static constexpr const char* parameterPrefix = "Main";

    FxmeNeuralAmpAudioProcessor();
    ~FxmeNeuralAmpAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
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

    // FxmeNet: the server and the client find the bridge through the VST3 component.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &net; }

    /** The effect's presets (module presets, shared with every plugin
        embedding it; see Common/EffectPresets.h). They carry the model. */
    fxme::ModulePresetTarget& getPresets() noexcept { return presets.target; }
    NeuralAmp& getNeuralAmp() { return amp; }

    /** Message thread: a model file into the state, then loaded. False (and
        `error` set) when it is not a model. */
    bool loadModelFile (const juce::File& file, juce::String& error);
    void clearModel();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    // Level and Calibrate describe the user's interface, not a sound: out of
    // the presets, so loading one never changes them.
    fxmefx::EffectPresets presets { apvts, "NeuralAmp", juce::String (parameterPrefix) + "_NAmp_",
                                    [] (const juce::String& key) { return key != "Level" && key != "Calib"; } };
    NeuralAmp amp;
    fxme::net::NetBridge net;   // FxmeNet: declared after what it registers

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeNeuralAmpAudioProcessor)
};
