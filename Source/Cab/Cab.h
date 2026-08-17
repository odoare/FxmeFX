/*
  ==============================================================================

    Cab.h

    Stereo cabinet/IR convolver: two independent mono IR slots (one per
    output channel) backed by the WDL convolution engine.

    Unlike ConvolReverb, this effect intentionally has no length / shape /
    offset shaping — a cabinet IR is meant to be played back faithfully.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <BinaryData.h>
#include <convoengine.h>
#include <array>
#include <atomic>
#include <vector>

class Cab
{
public:
    static constexpr int NumSlots = 2;            // 0 = left, 1 = right

    Cab();
    ~Cab();

    void prepare (double sampleRate, int samplesPerBlock);
    void process (juce::AudioBuffer<float>& buffer);

    /** Provide the list of IRs that the host UI can pick from.
        @param names      display names (typically .wav filenames)
        @param resources  matching BinaryData symbol names */
    void setImpulseList (const juce::StringArray& names, const juce::StringArray& resources);

    /** Load slot @p channel (0 = L, 1 = R) with the IR at @p index in the
        list passed to setImpulseList(). */
    void selectImpulse (int channel, int index);

    void setOn (bool shouldBeOn);
    bool isOn() const { return on; }

    const juce::StringArray& getImpulseNames() const { return irNames; }
    int getCurrentImpulseIndex (int channel) const
        { return channel >= 0 && channel < NumSlots ? currentIndex[(size_t) channel] : -1; }

    /** Snapshot of the loaded IR for the given slot — useful for drawing. */
    juce::AudioBuffer<float> getIR (int channel) const;

    /** Bumped every time a new pair of IRs is installed. The GUI watches it to
        know when getIR() has something new: loading is asynchronous now, so a
        combo-box change arrives before the audio it selected does. */
    int getIrGeneration() const noexcept { return irGeneration.load(); }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);

    /** Applies the cheap parameters (on/off and the two gains) — no allocation,
        no locking, safe from process(). The IR selectors are handled by the
        loader thread instead; see serviceParameters(). */
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix,
                               int numIRs = 0);

    /** Rewrite a saved APVTS state made with the single-gain version of this
        effect: the legacy prefix + "_Cab_Gain" value is copied into the new
        per-channel prefix + "_Cab_GainL" / "_Cab_GainR" parameters. Call it on
        the XML in setStateInformation() before replaceState(). */
    static void migrateLegacyState (juce::XmlElement& state, const juce::String& prefix);

private:
    // WDL engine — single engine driven by a 2-channel impulse buffer where
    // ch 0 = left IR and ch 1 = right IR. The engine then convolves each
    // input channel with its matching impulse channel.
    //
    // Building one of these decodes a WAV, resamples it and runs the engine's
    // FFT partitioning, all of which allocates — so it is built on the loader
    // thread and handed to the audio thread as a finished object.
    struct Stage
    {
        WDL_ImpulseBuffer impulse;
        WDL_ConvolutionEngine_Div engine;
    };

    /** What process() convolves with. stageLock is held just long enough to
        swap the pointer; the audio thread try-locks it and skips a single block
        if a swap is in flight, rather than waiting for an IR load. */
    std::unique_ptr<Stage> stage;
    mutable juce::CriticalSection stageLock;
    std::atomic<int> irGeneration { 0 };

    /** Serialises loading — the IR list, the two mono IRs and building a new
        Stage. Taken by the loader thread and by the GUI, never by audio. */
    mutable juce::CriticalSection lock;

    /** Polls the IR selectors off the audio thread. A plain thread rather than
        an AsyncUpdater because this must work with no editor open and with no
        message loop at all (the Pd external never pumps one). */
    class ParameterPoller : public juce::Thread
    {
    public:
        explicit ParameterPoller (Cab& o)
            : juce::Thread ("FxmeCab IR loader"), owner (o) {}

        void run() override
        {
            while (! threadShouldExit())
            {
                owner.serviceParameters();
                wait (20);
            }
        }

    private:
        Cab& owner;
    };

    ParameterPoller poller { *this };
    void serviceParameters();

    juce::AudioBuffer<WDL_FFT_REAL> wdlInputBuffer;
    std::vector<WDL_FFT_REAL*>      wdlInputPtrs;

    double currentSampleRate = 44100.0;

    juce::StringArray irNames;
    juce::StringArray irResources;
    std::array<juce::AudioBuffer<float>, NumSlots> monoIR;
    std::array<int, NumSlots> currentIndex { -1, -1 };

    std::array<float, NumSlots> gaindB     { 0.0f, 0.0f };
    std::array<float, NumSlots> gainLinear { 1.0f, 1.0f };
    bool on = true;

    std::atomic<float>* onParam  = nullptr;
    std::atomic<float>* irLParam = nullptr;
    std::atomic<float>* irRParam = nullptr;
    std::array<std::atomic<float>*, NumSlots> gainParam { nullptr, nullptr };

    float lastOn  = -1.0f;
    int   lastIRL = -1;
    int   lastIRR = -1;
    std::array<float, NumSlots> lastGain { -1000.0f, -1000.0f };

    void loadResource (int channel, const juce::String& resourceName);
    void loadIRFromReader (int channel, juce::AudioFormatReader& reader);
    void rebuildEngineImpulse();   // builds a new Stage and swaps it in
    void publishStage (std::unique_ptr<Stage> next);
    void updateGain();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cab)
};
