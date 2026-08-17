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

class ConvolReverb : private juce::ValueTree::Listener
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

    /** Bumped every time a new impulse is installed. The GUI watches it to know
        when getModifiedIR() has something new: loading is asynchronous now, so a
        control change arrives before the audio it selected does. */
    int getIrGeneration() const noexcept { return irGeneration.load(); }
    int getCurrentImpulseIndex() const { return currentIndex; }
    float getCurrentLengthRatio() const { return currentLengthRatio; }
    int getCurrentShapeType() const { return currentShapeType; }

    // External IR file support — the slot lives at index getExternalIndex(),
    // one past the last built-in. The IR audio itself is embedded in the APVTS
    // state as FLAC+Base64 (fxme::EmbeddedAudio), so presets and host sessions
    // are self-contained; the file path is also kept (not as a parameter,
    // since hosts can't automate strings) for display and as a legacy
    // fallback for states saved before embedding existed.
    bool setExternalIRFile (const juce::File& file);   // embeds + selects the file; false if unreadable
    void setExternalIRPath (const juce::String& path); // legacy path only; "" clears the embedded IR too
    juce::String getExternalIRPath() const;
    bool hasExternalIR() const;               // embedded data present, or legacy path resolves
    juce::String getExternalIRName() const;   // display name (no extension), or empty
    int getExternalIndex() const noexcept { return (int) irResources.size(); }
    static juce::Identifier externalPathPropertyId (const juce::String& prefix)
        { return juce::Identifier (prefix + "_Rev_ExtPath"); }
    static juce::String externalIRSlotId (const juce::String& prefix)
        { return prefix + "_Rev_ExtIR"; }

    // APVTS integration
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix, int numIRs = 0);

    // Applies the cheap parameters (on/off and the two gains) — no allocation,
    // no locking, safe from process(). Everything that needs an IR reload is
    // handled by the loader thread instead; see serviceParameters().
    void checkParameters();

private:
    // A fully prepared convolution stage: an impulse and an engine that has
    // already partitioned it. Building one decodes audio, resamples and runs
    // the engine's FFT setup, all of which allocates — so it is built on the
    // loader thread and handed to the audio thread as a finished object.
    struct Stage
    {
        WDL_ImpulseBuffer impulse;
        WDL_ConvolutionEngine_Div engine;
    };

    /** What process() convolves with. Guarded by stageLock, which is held just
        long enough to swap the pointer: the audio thread try-locks it and skips
        a single block in the rare event that a swap is in flight, rather than
        waiting for one (it used to wait for a whole IR load). */
    std::unique_ptr<Stage> stage;
    mutable juce::CriticalSection stageLock;
    std::atomic<int> irGeneration { 0 };

    /** Serialises everything to do with loading — originalIR, modifiedIR, the
        external path, and building a new Stage. Taken by the loader thread and
        by the GUI; never by the audio thread. */
    mutable juce::CriticalSection lock;

    juce::AudioBuffer<WDL_FFT_REAL> wdlInputBuffer;
    std::vector<WDL_FFT_REAL*> wdlInputPtrs;

    /** Polls the IR-selection parameters off the audio thread. A plain thread
        rather than an AsyncUpdater because this has to work with no editor open
        and with no message loop at all — the Pd external initialises JUCE but
        never pumps one, so async callbacks would never arrive there. */
    class ParameterPoller : public juce::Thread
    {
    public:
        explicit ParameterPoller (ConvolReverb& o)
            : juce::Thread ("FxmeConvolReverb IR loader"), owner (o) {}

        void run() override
        {
            while (! threadShouldExit())
            {
                owner.serviceParameters();
                wait (20);   // a user or automation move lands within a frame
            }
        }

    private:
        ConvolReverb& owner;
    };

    ParameterPoller poller { *this };
    void serviceParameters();

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
    juce::String extSlotId;   // fxme::EmbeddedAudio slot of this instance's external IR

    // Set when the state tree is replaced (preset/session load) or the
    // external IR data changes, so checkParameters() reloads the external IR
    // even though the IR-selection parameter value itself didn't move.
    std::atomic<bool> externalStateChanged { false };
    void valueTreeRedirected (juce::ValueTree&) override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;

    void loadResource (const juce::String& resourceName);
    void loadExternalIR(); // Loads the external IR file from externalPath
    void loadIRFromReader (juce::AudioFormatReader& reader);
    void updateModifiedIR(); // Applies length/shape to originalIR and installs it
    void installStage (const juce::AudioBuffer<float>& buffer);
    void updateGains();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolReverb)
};