/*
  ==============================================================================

    Tape.h

    Tape saturation and colour: a tape machine's record and playback path
    around the tape's magnetic hysteresis, without the transport (no wow or
    flutter: those belong to a tape delay).

    Per channel:

      Drive, Saturation   the level into the record head. Saturation also
                          scales the level into the tape, but with the
                          opposite gain after it, so it changes how hard the
                          tape is hit at a constant output level; Drive does
                          not (unless Auto gain is on). The tape type's
                          headroom works the same way.
      pre-emphasis        fxme::tape::EmphasisFilter, the record curve for
                          the Speed and the EQ standard (NAB, IEC; AES at 30
                          ips): highs, and with NAB the deep lows, reach the
                          tape louder, so they saturate first.
      the tape            fxme::TapeHysteresis (Jiles-Atherton), 2x or 4x
                          oversampled (polyphase IIR, integer latency,
                          reported); Type and Bias set its loop. Then the
                          playback head's high-frequency loss (a 2nd-order
                          low-pass at fxme::tape::headLossCutoffHz), still
                          oversampled since its corner can be above the host's
                          Nyquist.
      hiss                (off at the bottom of its range) white noise added
                          on the tape, so the de-emphasis darkens it as it
                          does on a machine. Seeded: a bounce renders the
                          same.
      de-emphasis         the exact inverse of the pre-emphasis.
      head bump           fxme::tape::headBumpFor: the bump, the dip above it
                          and the roll-off under it, for the speed.
      bias colour         a high shelf at 8 kHz, -6 dB per unit of bias over
                          the calibrated point: over-biased tape is darker,
                          under-biased brighter.
      output              the tape's gain normalised (approxGain), Auto gain
                          (divides by Drive), Mix against the dry signal
                          (delayed by the same latency), Output.

    The On switch fades to the dry signal, which keeps its delay, so the
    latency does not change with it.

    For the GUI and FxmeNet: two waveform taps of the left channel at the
    host's rate, the field H into the tape and its magnetisation M (the
    hysteresis loop, plotted one against the other), and the input and
    output levels.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/Biquad.h>
#include <FxmeTools/dsp/TapeEq.h>
#include <FxmeTools/dsp/TapeHysteresis.h>
#include <FxmeTools/dsp/WaveformTap.h>
#include <FxmeTools/dsp/VuMeter.h>
#include <FxmeTools/util/Random.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

class Tape
{
public:
    static constexpr int   maxChannels = 2;
    static constexpr float kHissOffDb  = -90.0f;   // the bottom of the Hiss range: off

    Tape();
    ~Tape();

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** The On parameter when there is one: right in a mirror (FxmeNet), which
        never processes, and no race with the audio thread. */
    bool isOn() const { return onParam != nullptr ? onParam->load() > 0.5f : on; }

    /** The oversampling's delay, for the processor to report. */
    int getLatencySamples() const noexcept { return latencySamples.load(); }

    fxme::WaveformTap& getFieldTap() noexcept         { return fieldTap; }
    fxme::WaveformTap& getMagnetisationTap() noexcept { return magTap; }
    fxme::VuMeter& getInputMeter() noexcept           { return inMeter; }
    fxme::VuMeter& getOutputMeter() noexcept          { return outMeter; }

private:
    static constexpr int kChunk = 512;      // samples sent up at once
    static constexpr int kMaxLatency = 64;  // the dry delay's ring

    void setOversampling (int factor);
    void updateOversampledFilters();
    void updateEq();
    void updateTape();
    void updateGains (bool snap);
    void processChunk (float* const* data, int numChannels, int numSamples);

    double currentSampleRate = 44100.0;
    int    oversamplingFactor = 2;
    std::atomic<int> latencySamples { 0 };

    using OS = juce::dsp::Oversampling<float>;
    std::unique_ptr<OS> oversampler2x, oversampler4x;
    OS* os = nullptr;

    std::array<fxme::tape::EmphasisFilter, maxChannels> preEmphasis, deEmphasis;
    std::array<fxme::TapeHysteresis, maxChannels> tape;
    std::array<fxme::Biquad, maxChannels> headLoss, bump, dip, lowCut, biasShelf;
    std::array<fxme::Random, maxChannels> hissNoise;

    // The dry signal, delayed by the latency, for Mix and the On fade.
    std::array<std::array<float, kMaxLatency>, maxChannels> dryRing {};
    int dryPos = 0;

    std::array<std::vector<float>, maxChannels> scratch;   // one chunk, for the meters and taps
    std::vector<float> monoScratch, fieldScratch, magScratch;

    juce::SmoothedValue<float> inGainSmooth, normSmooth, mixSmooth, outSmooth;

    fxme::WaveformTap fieldTap, magTap;
    fxme::VuMeter inMeter, outMeter;

    // Resolved parameter values.
    bool  on = true, autoGain = true;
    float driveDb = 0.0f, saturation01 = 0.5f, bias01 = 0.5f, hissDb = kHissOffDb;
    float mix01 = 1.0f, outDb = 0.0f;
    fxme::TapeHysteresis::TapeType type = fxme::TapeHysteresis::TapeType::modern;
    fxme::tape::Speed speed = fxme::tape::Speed::ips15;
    fxme::tape::EqStandard eq = fxme::tape::EqStandard::nab;
    float hissGain = 0.0f, hissCompensation = 1.0f;
    bool  active = true;   // false once the switch-off fade has finished

    // APVTS pointers.
    std::atomic<float>* onParam    = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* satParam   = nullptr;
    std::atomic<float>* biasParam  = nullptr;
    std::atomic<float>* typeParam  = nullptr;
    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* eqParam    = nullptr;
    std::atomic<float>* hissParam  = nullptr;
    std::atomic<float>* osParam    = nullptr;
    std::atomic<float>* autoParam  = nullptr;
    std::atomic<float>* mixParam   = nullptr;
    std::atomic<float>* outParam   = nullptr;
};
