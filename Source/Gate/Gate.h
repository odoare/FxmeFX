/*
  ==============================================================================

    Gate.h

    Noise gate and downward expander.

    The detector is the input through a sidechain high-pass and low-pass (on
    the detector only), linked across the channels (the larger of the two),
    and followed by a peak envelope (instant rise, 10 ms fall) so that the
    zero crossings of a low note do not read as silence. Its level drives a
    fxme::GateStateMachine: open at the threshold, closing a hysteresis
    lower, after a hold.

    While the gate is open the gain is 0 dB. When it is closed, the gain is
    what fxme::GainComputer (expand mode) asks for at that level: down to the
    range at once in Gate mode (an infinite ratio), along a ratio in Expander
    mode, never deeper than the range in either. The gain then moves in dB,
    with the attack when it rises (opening) and the release when it falls
    (closing).

    Look-ahead delays the audio, not the detector, so the gate starts opening
    before a transient arrives and the attack does not eat it; the delay is
    reported to the host as latency (getLatencySamples()). Listen replaces
    the output with the sidechain-filtered signal, to set the filters by ear.

    The On switch fades (20 ms) between the gated and the untouched signal,
    which keeps going through the look-ahead delay, so the latency does not
    change with it.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/DynamicGainComputer.h>
#include <FxmeTools/dsp/GateStateMachine.h>
#include <FxmeTools/dsp/TptSvf.h>
#include <array>
#include <atomic>
#include <vector>

class Gate
{
public:
    enum class Mode { gate = 0, expander = 1 };

    static constexpr float kMaxLookaheadMs = 5.0f;
    static constexpr float kMinLevelDb     = -80.0f;   // threshold and range floor

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** The On parameter when there is one: right in a mirror (FxmeNet), which
        never processes, and no race with the audio thread. */
    bool isOn() const { return onParam != nullptr ? onParam->load() > 0.5f : on; }

    /** The look-ahead in samples, for the processor to report. */
    int getLatencySamples() const noexcept { return latencySamples.load(); }

    // For the GUI (and FxmeNet's telemetry): the detector's highest level over
    // the last block (dBFS), the gain at its end (dB, <= 0), and the state
    // (GateStateMachine::State as a float: 0 closed, 1 open, 2 hold).
    std::atomic<float>& getDisplayLevel() noexcept { return displayLevel; }
    std::atomic<float>& getDisplayGain() noexcept  { return displayGain; }
    std::atomic<float>& getDisplayState() noexcept { return displayState; }

private:
    void updateFilters();
    void updateTimes();
    void updateComputer();

    double currentSampleRate = 44100.0;
    int    numChannelsPrepared = 2;

    // Sidechain filters, per channel.
    std::array<fxme::TptSvf, 2> scHighPass, scLowPass;

    fxme::GateStateMachine stateMachine;
    fxme::GainComputer computer;

    float peakEnv = 0.0f, peakFall = 0.0f;   // the detector's envelope
    float gainDb = kMinLevelDb;              // current gain, after the ballistics
    float attackCoef = 0.0f, releaseCoef = 0.0f;

    // Look-ahead: one circular buffer per channel.
    std::array<std::vector<float>, 2> delay;
    int delaySize = 1, writePos = 0, lookaheadSamples = 0;
    std::atomic<int> latencySamples { 0 };

    juce::SmoothedValue<float> onSmooth, outSmooth;

    // Resolved parameter values.
    bool  on = true, listen = false;
    Mode  mode = Mode::gate;
    float thresholdDb = -40.0f, hysteresisDb = 4.0f, rangeDb = 80.0f, ratio = 2.0f;
    float attackMs = 0.5f, holdMs = 20.0f, releaseMs = 150.0f;
    float hpHz = 20.0f, lpHz = 20000.0f, lookaheadMs = 0.0f, outDb = 0.0f;

    std::atomic<float> displayLevel { kMinLevelDb }, displayGain { 0.0f }, displayState { 0.0f };

    // APVTS pointers.
    std::atomic<float>* onParam        = nullptr;
    std::atomic<float>* modeParam      = nullptr;
    std::atomic<float>* threshParam    = nullptr;
    std::atomic<float>* hystParam      = nullptr;
    std::atomic<float>* rangeParam     = nullptr;
    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* holdParam      = nullptr;
    std::atomic<float>* releaseParam   = nullptr;
    std::atomic<float>* hpParam        = nullptr;
    std::atomic<float>* lpParam        = nullptr;
    std::atomic<float>* listenParam    = nullptr;
    std::atomic<float>* lookaheadParam = nullptr;
    std::atomic<float>* outParam       = nullptr;
};
