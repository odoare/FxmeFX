/*
  ==============================================================================

    Crusher.h

    Bit and sample-rate reduction, after an old sampler's signal path:

      drive        input gain into the converter, which clips at full scale;
      anti-alias   (Clean) a 48 dB/octave low-pass just under the target
                   rate's Nyquist, so the decimation does not fold the highs
                   back down. Off (Dirty), the aliasing is the sound;
      quantizer    fxme::BitCrusher: 1 to 24 bits, fractional, mid-tread,
                   mid-rise or mu-law, with optional TPDF dither (Noise),
                   which turns the quantization distortion into hiss;
      decimation   fxme::Downsampler: a sample and hold at the target rate,
                   whose hold lengths can jitter (deterministic, both channels
                   in lockstep) to blur the fixed aliasing pattern;
      smoothing    (Smooth) a 24 dB/octave low-pass at the same cutoff, the
                   reconstruction filter that rounds the steps off;
      DC blocker   a 10 Hz high-pass (mid-rise and dither leave an offset);
      mix, output.

    At the top of the Rate range the decimation (and the filters with it) is
    off. The Clean and Smooth switches crossfade, so they do not click; the
    On switch fades to the untouched signal.

    Two waveform taps (input and output, left channel) feed the GUI's view of
    the steps and FxmeNet's telemetry. They only record while enabled (the
    editor is open).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/BitCrusher.h>
#include <FxmeTools/dsp/Downsampler.h>
#include <FxmeTools/dsp/TptSvf.h>
#include <FxmeTools/dsp/WaveformTap.h>
#include <FxmeTools/util/Random.h>
#include <array>
#include <atomic>
#include <vector>

class Crusher
{
public:
    static constexpr float kMinRateHz = 200.0f;
    static constexpr float kMaxRateHz = 48000.0f;   // the top of the range: decimation off

    Crusher();

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** The On parameter when there is one: right in a mirror (FxmeNet), which
        never processes, and no race with the audio thread. */
    bool isOn() const { return onParam != nullptr ? onParam->load() > 0.5f : on; }

    fxme::WaveformTap& getInputTap() noexcept  { return inputTap; }
    fxme::WaveformTap& getOutputTap() noexcept { return outputTap; }

private:
    void updateRate();
    void flushTaps();

    double currentSampleRate = 44100.0;

    std::array<fxme::Downsampler, 2> holds;
    fxme::BitCrusher quantizer;
    std::array<fxme::Random, 2> ditherNoise;
    fxme::SvfPassCascade antiAlias, smoothing;
    bool decimating = false;

    // DC blocker (one-pole high-pass), per channel.
    float dcCoef = 0.999f;
    std::array<float, 2> dcX1 {}, dcY1 {};

    juce::SmoothedValue<float> mixSmooth, outSmooth, driveSmooth, cleanSmooth, smoothSmooth;

    fxme::WaveformTap inputTap, outputTap;
    std::vector<float> tapInScratch, tapOutScratch;
    int tapFill = 0;

    // Resolved parameter values.
    bool  on = true, clean = false, smooth = false;
    float bits = 8.0f, rateHz = 11025.0f, jitter01 = 0.0f, dither01 = 0.0f;
    float driveDb = 0.0f, mix01 = 1.0f, outDb = 0.0f;
    int   mode = 0;
    bool  active = true;   // false once the switch-off fade has finished

    // APVTS pointers.
    std::atomic<float>* onParam     = nullptr;
    std::atomic<float>* driveParam  = nullptr;
    std::atomic<float>* bitsParam   = nullptr;
    std::atomic<float>* rateParam   = nullptr;
    std::atomic<float>* modeParam   = nullptr;
    std::atomic<float>* jitterParam = nullptr;
    std::atomic<float>* noiseParam  = nullptr;
    std::atomic<float>* cleanParam  = nullptr;
    std::atomic<float>* smoothParam = nullptr;
    std::atomic<float>* mixParam    = nullptr;
    std::atomic<float>* outParam    = nullptr;
};
