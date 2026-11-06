/*
  ==============================================================================

    DeEsser.h

    De-esser: turns sibilance down when a high band of the signal gets loud.

    Detector. The input (or its mid, in Mid mode) through a band-pass (centre
    Freq, width Q, 0 dB at the centre) or a high-pass (from Freq) on the
    sidechain, its level measured as RMS over 2 ms, the two channels linked
    (their mean power). In Relative mode the level is the band's against the
    whole signal's (same RMS), so the threshold reads "how far the band rises
    against the rest" and works at any input level.

    Gain. fxme::GainComputer in compress mode (a fixed 6:1 with a 6 dB soft
    knee: a de-esser is a steep compressor on one band), never deeper than
    Range, then fxme::GainBallistics. Speed sets both times at once: from
    5 ms attack and 150 ms release (0 %) to 0.2 ms and 20 ms (100 %).

    Two ways to apply it:

      wideband    the whole signal is turned down while the band triggers
                  (classic, safe on any material);
      split band  only the band is: a fxme::RegaliaMitraEq bell at the
                  detector's frequency and Q (a high shelf from Freq with the
                  high-pass detector), whose gain is the computed gain. The
                  structure takes a new gain every sample without clicks.

    Stereo: Linked acts on both channels alike; Mid detects on the mid
    (L + R) / 2 and only turns the mid down, so a centred vocal is de-essed
    and the sides (reverb, cymbals spread wide) are left alone.

    Listen outputs the detector's signal (the band), to find the sibilance.
    The On switch fades (20 ms) to the untouched signal.

    For the GUI and FxmeNet: the detector's highest level over the last
    block, the gain at its end, and two spectrum taps (input and output, the
    channels summed).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/DynamicGainComputer.h>
#include <FxmeTools/dsp/RegaliaMitraEq.h>
#include <FxmeTools/dsp/SpectrumTap.h>
#include <FxmeTools/dsp/TptSvf.h>
#include <array>
#include <atomic>

class DeEsser
{
public:
    enum class Mode   { wideband = 0, splitBand = 1 };
    enum class Filter { bandPass = 0, highPass = 1 };
    enum class Stereo { linked = 0, mid = 1 };

    static constexpr float kMinFreqHz   = 2000.0f;
    static constexpr float kMaxFreqHz   = 12000.0f;
    static constexpr float kMinLevelDb  = -60.0f;   // threshold floor, meter floor
    static constexpr float kMaxRangeDb  = 24.0f;    // deepest reduction

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** The On parameter when there is one: right in a mirror (FxmeNet), which
        never processes, and no race with the audio thread. */
    bool isOn() const { return onParam != nullptr ? onParam->load() > 0.5f : on; }

    double getSampleRate() const noexcept { return currentSampleRate; }

    std::atomic<float>& getDisplayLevel() noexcept { return displayLevel; }
    std::atomic<float>& getDisplayGain() noexcept  { return displayGain; }
    fxme::SpectrumTap& getInputTap() noexcept      { return inputTap; }
    fxme::SpectrumTap& getOutputTap() noexcept     { return outputTap; }

private:
    void updateFilters (bool snap);
    void updateComputer();
    void updateTimes();
    static void pushSum (fxme::SpectrumTap& tap, const juce::AudioBuffer<float>& buffer, int numChannels) noexcept;

    double currentSampleRate = 44100.0;

    std::array<fxme::TptSvf, 2> detectorFilter;   // per channel (channel 0 only in Mid mode)
    fxme::LevelDetector bandDetector, fullDetector;
    fxme::GainComputer computer;
    fxme::GainBallistics ballistics;
    fxme::RegaliaMitraEq bandEq;

    juce::SmoothedValue<float> onSmooth;

    fxme::SpectrumTap inputTap, outputTap;
    std::atomic<float> displayLevel { kMinLevelDb }, displayGain { 0.0f };

    // Resolved parameter values.
    bool   on = true, relative = false, listen = false;
    Mode   mode = Mode::wideband;
    Filter filter = Filter::bandPass;
    Stereo stereo = Stereo::linked;
    float  freqHz = 6000.0f, q = 1.5f, thresholdDb = -30.0f, rangeDb = 12.0f, speed01 = 0.5f;

    // APVTS pointers.
    std::atomic<float>* onParam       = nullptr;
    std::atomic<float>* modeParam     = nullptr;
    std::atomic<float>* filterParam   = nullptr;
    std::atomic<float>* freqParam     = nullptr;
    std::atomic<float>* qParam        = nullptr;
    std::atomic<float>* threshParam   = nullptr;
    std::atomic<float>* rangeParam    = nullptr;
    std::atomic<float>* speedParam    = nullptr;
    std::atomic<float>* relativeParam = nullptr;
    std::atomic<float>* stereoParam   = nullptr;
    std::atomic<float>* listenParam   = nullptr;
};
