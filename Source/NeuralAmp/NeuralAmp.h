/*
  ==============================================================================

    NeuralAmp.h

    Plays a neural amp or pedal model: a NAM file (Steven Atkinson's Neural
    Amp Modeler: WaveNet A1 and A2, LSTM) or a Keras model, through
    FxmeTools' FxmeNeural (fxme::neural::NeuralAmpEngine on NeuralAudio).
    No cabinet: FxmeCab goes after it.

    Per block:

      Input       a gain into the model (the drive), smoothed.
      Gate        (off at the bottom of its range) a noise gate before the
                  model, which a high-gain model would otherwise amplify:
                  the input's peak (instant rise, 10 ms fall) drives a
                  fxme::GateStateMachine (open at the threshold, closing 6 dB
                  lower after 30 ms), the gain moving in 1 ms when it
                  opens, 60 ms when it closes.
      the model   at the rate it was trained at (48 kHz), the resampler
                  around it when the host differs (latency reported: 0 at
                  48 kHz, 139 samples at 44.1 kHz, 275 at 96 kHz, the same
                  with or without a model). Channels: Mono (one model on the
                  sum of the two sides, the result on both: a guitar amp,
                  half the CPU) or Dual mono (one model per side). A mono
                  bus runs one model either way. Calibration: Level, the
                  interface's full scale in dBu, against the level the model
                  was captured at (when the file says, and Calibrate is on);
                  Normalize brings the model's loudness to -18 dB.
      Tone stack  Bass (low shelf, 150 Hz), Mid (peak, 425 Hz), Treble
                  (high shelf, 1.8 kHz), as in NAM's own plugin, after the
                  model.
      output      Mix against the dry signal (delayed by the same latency),
                  Output.

    On fades to the dry signal, which keeps its delay, so the latency does
    not change with it. Once faded the models stop running; turned back on,
    they run unheard for the latency + 10 ms before the fade starts (their
    history fills again), so nothing stale is heard.

    The model is kept in the state (fxme::EmbeddedData, slot
    "<prefix>_NAmp_Model": the file's bytes, deflated), so a session and a
    preset carry it, with no file on disk. Loading happens on a thread of
    its own (never on the audio thread); a model change crossfades (see
    NeuralAmpEngine). Module presets carry the model in their <Extra> when
    one is loaded; a preset without one keeps the current model (factory
    presets are settings for any model).

    FxmeNet: a mirror (setMirrorCheck) never builds a network; it reads the
    model's name and metadata from the state.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/Biquad.h>
#include <FxmeTools/dsp/GateStateMachine.h>
#include <FxmeTools/dsp/VuMeter.h>
#include <FxmeTools/neural/NeuralAmpEngine.h>
#include <array>
#include <atomic>
#include <functional>
#include <vector>

class NeuralAmp
{
public:
    static constexpr int   maxChannels = 2;
    static constexpr float kGateOffDb  = -90.0f;   // the bottom of the Gate range: off
    enum class Channels { dualMono = 0, mono = 1 };

    /** What the GUI shows about the model. */
    struct ModelStatus
    {
        bool hasModel = false;        // one is in the state
        bool loading = false;         // being built
        bool ready = false;           // playing (never in a mirror)
        juce::String fileName;        // as loaded, e.g. "Plexi.nam"
        juce::String error;           // why it does not play, when it does not
        fxme::neural::ModelInfo info; // its metadata
        int storedBytes = 0;          // what it adds to a saved state
        bool remote = false;          // FxmeNet mirror: loading / ready / error are the server's
    };

    NeuralAmp();
    ~NeuralAmp();

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    /** Also remembers `apvts` and `prefix`: the model lives in its state. */
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** The On parameter when there is one: right in a mirror (FxmeNet). */
    bool isOn() const { return onParam != nullptr ? onParam->load() > 0.5f : on; }

    /** The resampler's delay, for the processor to report. */
    int getLatencySamples() const noexcept { return latencySamples.load(); }

    //==========================================================================
    // The model. Message thread unless said otherwise.

    /** Reads `file`, checks it is a model, keeps it in the state and loads
        it. False (and `error` set) when it is not a model. */
    bool loadModelFile (const juce::File& file, juce::String& error);
    /** Removes the model from the state: dry (with the tone stack). */
    void clearModel();
    /** After the state was replaced (a session, a preset): loads the model
        it holds, if it is not the one playing, or unloads. Any thread but the
        audio one. */
    void syncModelFromState();
    /** FxmeNet: returns true in a mirror, which then never loads networks. */
    void setMirrorCheck (std::function<bool()> isMirror) { mirrorCheck = std::move (isMirror); }

    /** Module presets: the model into a preset's <Extra>, and back. */
    void writePresetExtra (juce::XmlElement& extra) const;
    void readPresetExtra (const juce::XmlElement* extra);

    ModelStatus getModelStatus() const;
    /** Bumped whenever getModelStatus() changes. */
    int getStatusGeneration() const noexcept { return statusGeneration.load(); }

    /** <user data>/FX-Mechanics/Models/NAM: where the browser looks. */
    static juce::File getModelsFolder();
    /** The extensions the effect reads (".nam", ".json"). */
    static bool isModelFile (const juce::File& f);

    //==========================================================================
    // For the GUI and FxmeNet.
    fxme::VuMeter& getInputMeter() noexcept   { return inMeter; }
    fxme::VuMeter& getOutputMeter() noexcept  { return outMeter; }
    /** -1 gate off, else fxme::GateStateMachine::State: 0 closed, 1 open, 2 hold. */
    std::atomic<float>& getGateState() noexcept { return gateState; }
    /** The model's state where it plays, as a value FxmeNet carries to the
        mirror: 0 none, 1 loading, 2 playing, 3 failed. Written by the
        instance that loads (never by a mirror, whose copy telemetry fills),
        read by getModelStatus() in a mirror. */
    std::atomic<float>& getModelStateValue() noexcept { return modelState; }

private:
    static constexpr int kChunk = 512;

    struct Request
    {
        juce::MemoryBlock bytes;
        juce::String fileName;
        int generation = 0;
    };

    class Loader : public juce::Thread
    {
    public:
        explicit Loader (NeuralAmp& o) : juce::Thread ("FxmeNeuralAmp loader"), owner (o) {}
        void run() override;
    private:
        NeuralAmp& owner;
    };

    juce::String slotId() const { return prefix + "_NAmp_Model"; }
    bool isMirror() const { return mirrorCheck && mirrorCheck(); }
    void request (juce::MemoryBlock bytes, const juce::String& fileName);
    void serviceLoader();
    bool loadInto (const Request& r, juce::String& error);   // under loaderLock
    void setStatus (std::function<void (ModelStatus&)> change);

    void processChunk (float* const* data, int numChannels, int numSamples);
    void updateToneStack();

    // Models: both engines carry the model, so Channels switches at once.
    std::array<fxme::neural::NeuralAmpEngine, maxChannels> engines;
    Loader loader { *this };
    juce::CriticalSection loaderLock;     // prepare against the loader's builds
    juce::CriticalSection requestLock;    // the request below
    Request wanted;                       // the latest asked for (empty bytes: none)
    int builtGeneration = -1;             // what the engines hold (under loaderLock)
    int nextGeneration = 0;
    mutable juce::CriticalSection statusLock;
    ModelStatus status;
    std::atomic<int> statusGeneration { 0 };
    std::function<bool()> mirrorCheck;

    juce::AudioProcessorValueTreeState* apvts = nullptr;
    juce::String prefix { "Main" };

    double currentSampleRate = 0.0;
    int    maxBlock = kChunk;
    std::atomic<int> latencySamples { 0 };

    // Signal path.
    std::array<std::vector<float>, maxChannels> wet, dryDelay;
    int dryPos = 0, dryLength = 0;
    std::vector<float> monoScratch;
    std::array<std::array<fxme::Biquad, 3>, maxChannels> tone;   // bass, mid, treble
    fxme::GateStateMachine gate;
    float peakEnv = 0.0f, peakFall = 0.0f, gateGain = 1.0f, gateOpenCoef = 0.0f, gateCloseCoef = 0.0f;
    juce::SmoothedValue<float> inSmooth, mixSmooth, outSmooth, modeSmooth;
    fxme::VuMeter inMeter, outMeter;
    std::atomic<float> gateState { -1.0f };
    std::atomic<float> modelState { 0.0f };

    // Resolved parameter values.
    bool  on = true, active = true, calibrate = true, normalise = true;
    float inDb = 0.0f, gateDb = kGateOffDb, bassDb = 0.0f, midDb = 0.0f, trebleDb = 0.0f;
    float mix01 = 1.0f, outDb = 0.0f, levelDbu = 12.0f;
    Channels channels = Channels::mono, pendingChannels = Channels::mono;
    int preRoll = 0;       // samples left before the On fade-in may start
    int modePreRoll = 0;   // the same after a Channels switch, for the dip

    // APVTS pointers.
    std::atomic<float>* onParam     = nullptr;
    std::atomic<float>* inParam     = nullptr;
    std::atomic<float>* gateParam   = nullptr;
    std::atomic<float>* bassParam   = nullptr;
    std::atomic<float>* midParam    = nullptr;
    std::atomic<float>* trebleParam = nullptr;
    std::atomic<float>* mixParam    = nullptr;
    std::atomic<float>* outParam    = nullptr;
    std::atomic<float>* chanParam   = nullptr;
    std::atomic<float>* levelParam  = nullptr;
    std::atomic<float>* calibParam  = nullptr;
    std::atomic<float>* normParam   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralAmp)
};
