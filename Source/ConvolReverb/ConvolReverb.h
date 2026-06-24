/*
  ==============================================================================

    ConvolReverb.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <BinaryData.h>
#include <convoengine.h>
#include <atomic>
#include <vector>

class ConvolReverb
{
public:
    ConvolReverb();
    ~ConvolReverb();

    void prepare (double sampleRate, int samplesPerBlock);
    void process (juce::AudioBuffer<float>& buffer);

    void setImpulseList (const juce::StringArray& names, const juce::StringArray& resourceNames);
    void selectImpulse (int index);
    void setLengthRatio (float ratio); // 0.0 to 1.0
    void setShapeType (int type); // 0: Exp, 1: Lin, 2: Log
    void setStartOffset (float offsetMs); // -100ms to 100ms
    void setOn (bool shouldBeOn);
    bool isOn() const { return on; }

    const juce::StringArray& getImpulseNames() const { return irNames; }
    juce::AudioBuffer<float> getModifiedIR() const;
    int getCurrentImpulseIndex() const { return currentIndex; }
    float getCurrentLengthRatio() const { return currentLengthRatio; }
    int getCurrentShapeType() const { return currentShapeType; }

    // External IR file support — the slot lives at index getExternalIndex(),
    // one past the last built-in. The path is persisted in the APVTS state
    // tree (not as a parameter, since hosts can't automate strings).
    void setExternalIRPath (const juce::String& path);
    juce::String getExternalIRPath() const;
    int getExternalIndex() const noexcept { return (int) irResources.size(); }
    static juce::Identifier externalPathPropertyId (const juce::String& prefix)
        { return juce::Identifier (prefix + "_Rev_ExtPath"); }

    // APVTS integration
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix, int numIRs = 0);

    // Poll APVTS parameters and reload the IR / update the modified buffer when
    // anything has changed.  Called from process() on the audio thread, but also
    // safe to call from the GUI timer when the component needs an up-to-date IR
    // snapshot for drawing (the GUI-side instance has its own lock).
    void checkParameters();

private:
    // WDL Engine
    WDL_ImpulseBuffer impulseBuffer;
    WDL_ConvolutionEngine_Div engine;
    mutable juce::CriticalSection lock;
    juce::AudioBuffer<WDL_FFT_REAL> wdlInputBuffer;
    std::vector<WDL_FFT_REAL*> wdlInputPtrs;

    double currentSampleRate = 44100.0;

    juce::StringArray irNames;
    juce::StringArray irResources;
    juce::AudioBuffer<float> originalIR; // Stores the raw loaded IR
    juce::AudioBuffer<float> modifiedIR; // Stores the IR after length/shape modifications

    int currentIndex = -1;
    float currentLengthRatio = 1.0f;
    int currentShapeType = 0;
    float currentStartOffsetMs = 0.0f;
    float dryGain = 0.0f;
    float wetGain = 0.0f;
    float dryGainLinear = 0.0f;
    float wetGainLinear = 1.0f;
    bool on = true;

    // Parameter pointers
    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* irParam = nullptr;
    std::atomic<float>* lengthParam = nullptr;
    std::atomic<float>* shapeParam = nullptr;
    std::atomic<float>* startOffsetParam = nullptr;
    std::atomic<float>* dryGainParam = nullptr;
    std::atomic<float>* wetGainParam = nullptr;

    // Last values for change detection
    int lastIR = -1;
    float lastLengthRatio = -1.0f;
    int lastShapeType = -1;
    float lastStartOffset = 0.0f;
    float lastDryGain = -100.0f;
    float lastWetGain = -100.0f;
    float lastOn = -1.0f;

    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;
    juce::Identifier extPathId;
    juce::String externalPath;

    void loadResource (const juce::String& resourceName);
    void loadExternalIR(); // Loads the external IR file from externalPath
    void loadIRFromReader (juce::AudioFormatReader& reader);
    void updateModifiedIR(); // Applies length/shape to originalIR and loads into wdlReverb
    void loadImpulseToEngine (const juce::AudioBuffer<float>& buffer);
    void updateGains();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolReverb)
};