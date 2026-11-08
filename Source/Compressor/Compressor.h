/*
  ==============================================================================

    Compressor.h

    The detector can be shaped and re-keyed, for side-chain work:

      - a high-pass and a low-pass on the detector only (off at their range's
        ends, 20 Hz and 20 kHz, where the detector is exactly what it was
        without them);
      - an external key: with the Ext SC switch on and a side-chain buffer
        given to process(), the detector listens to that buffer instead of
        the input. The switch decides, not the buffer's presence: a host may
        activate an aux bus nobody feeds, and a silent key would stop the
        compression. Without a buffer the detector stays on the input. The
        key is not affected by Pre Gain, which drives the input only.

    Only a host with an aux input asks for the switch (addParameters'
    withSidechain); embedding hosts and the Pure Data external do not get it.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VuMeter.h"
#include <FxmeTools/dsp/TptSvf.h>
#include <atomic>
#include <vector>

class Compressor
{
public:
    enum class ReleaseMode { Linear = 0, Opto = 1, Vintage = 2 };

    Compressor();

    void prepare (double sampleRate, int numChannels);
    /** `sidechain` (optional, read only) is the key the Ext SC switch selects;
        at least as many samples as `buffer`. */
    void process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain = nullptr);

    void setAttack (float attackMs);
    void setRelease (float releaseMs); // Corresponds to "decay time" in request
    void setThreshold (float thresholddB);
    void setRatio (float ratio);
    void setKnee (float kneedBNew);
    void setReleaseMode (int mode);
    void setPeakRmsMix (float mix);
    void setPostGain (float gaindB);
    void setSidechainFilters (float highPassHz, float lowPassHz);
    void setExternalSidechain (bool shouldUseIt) { externalSidechain = shouldUseIt; }
    void setOn (bool shouldBeOn);
    bool isOn() const;
    VuMeter& getGrMeter() { return grMeter; }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    /** `withSidechain`: also the Ext SC switch, for a host with an aux input. */
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix,
                               bool withSidechain = false);

    static constexpr float kScHighPassOffHz = 20.0f, kScLowPassOffHz = 20000.0f;

private:
    double currentSampleRate = 44100.0;

    float attackTimeMs = 10.0f;
    float releaseTimeMs = 100.0f;
    float thresholddB = 0.0f;
    float ratio = 1.0f;
    float kneedB = 0.0f;
    ReleaseMode releaseMode = ReleaseMode::Linear;
    float peakRmsMix = 0.0f;
    float postGaindB = 0.0f;
    float preGain = 1.0f;
    bool on = true;

    float envelope = 0.0f;
    VuMeter grMeter;

    // The detector's filters, per channel, and the external key.
    std::vector<fxme::TptSvf> scHighPass, scLowPass;
    float scHighPassHz = kScHighPassOffHz, scLowPassHz = kScLowPassOffHz;
    bool scHighPassOn = false, scLowPassOn = false;
    bool externalSidechain = false;

    // RMS detector state (fixed 10 ms averaging window)
    float rmsSquare = 0.0f;
    float rmsCoef = 0.0f;
    static constexpr float rmsTimeMs = 10.0f;

    // Program-dependent release state
    float lastGainReductiondB = 0.0f;
    float stress = 0.0f;
    float stressCoef = 0.0f;
    static constexpr float stressTimeMs = 200.0f;

    // Coefficients
    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;
    float makeUpGain = 1.0f;

    // Parameter Pointers
    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* preGainParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* threshParam = nullptr;
    std::atomic<float>* ratioParam = nullptr;
    std::atomic<float>* kneeParam = nullptr;
    std::atomic<float>* relModeParam = nullptr;
    std::atomic<float>* peakRmsParam = nullptr;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* scHighPassParam = nullptr;
    std::atomic<float>* scLowPassParam = nullptr;
    std::atomic<float>* extSidechainParam = nullptr;

    float lastOn = -1.0f, lastPreGain = -100.0f, lastAttack = -1.0f, lastRelease = -1.0f, lastThresh = 1.0f, lastRatio = -1.0f, lastGain = -1.0f;
    float lastKnee = -1.0f, lastRelMode = -1.0f, lastPeakRms = -1.0f;

    void updateCoefficients();
    void updateSidechainFilters();
};