/*
  ==============================================================================

    Phaser.h

    Classic stereo phaser: a cascade of first-order allpass sections per
    channel (fxme::AllpassChain), their common break frequency swept by one
    shared fxme::ModLfo, the result summed back with the dry signal. Every
    pair of sections puts one notch in that sum, so Stages sets how many
    notches travel up and down the spectrum — four for the familiar pedal
    voice, ten or twelve for the thick, resonant sweep.

    Unlike the chorus and the flanger nothing is delayed here: the effect is
    pure phase rotation, which is why a phaser stays tight on transients
    where a flanger smears them.

    Freq is the centre of the sweep and Depth how far either side of it the
    LFO travels, in octaves. Feedback recirculates the chain's output, which
    sharpens the notches into resonant peaks; negative values invert it for
    the hollower variant.

    The rate is free (Hz) or locked to the host tempo, in which case the
    phase is re-derived from the playhead's PPQ position at the top of every
    block.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/AllpassChain.h>
#include <FxmeTools/dsp/ModLfo.h>
#include <array>
#include <vector>

class Phaser
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** The Stages choice list, and the stage count each entry stands for. */
    static juce::StringArray stageChoices() { return { "2", "4", "6", "8", "10", "12" }; }
    static int stagesForIndex (int index)   { return 2 * (juce::jlimit (0, 5, index) + 1); }

    /** Host tempo, for the tempo-synced rate. */
    void setBPM (double bpm);
    double getBPM() const { return currentBPM; }

    /** Host tempo plus the timeline position, so a synced sweep can be locked
        to the bar. `isPlaying` false leaves the LFO free-running. */
    void setTransport (double bpm, double ppqPosition, bool isPlaying);

    /** The rate the LFO actually runs at, in Hz (tempo-derived while synced). */
    float getEffectiveRateHz() const { return lfo.frequencyHz(); }

private:
    // Widest excursion either side of Freq, at Depth 100 %.
    static constexpr float kSweepOctaves = 2.5f;

    double currentSampleRate = 44100.0;
    double currentBPM        = 120.0;
    double hostPpq           = 0.0;
    bool   hostPlaying       = false;

    std::array<fxme::AllpassChain, 2> chains;
    std::array<float, 2>              feedbackState { 0.0f, 0.0f };
    fxme::ModLfo lfo;

    juce::SmoothedValue<float> mixSmooth, outSmooth, centreSmooth, depthSmooth, feedbackSmooth;

    bool  on           = true;
    bool  chainsActive = true;   // false once the dry-only fade has finished
    float widthOffset  = 0.5f;   // right-channel phase offset, in cycles

    // Parameter cache (defaults mirror addParameters(), so the first block
    // after prepare() already sweeps around the right frequency)
    float mix01    = 0.5f;
    float freqHz   = 500.0f;
    float depthOct = 0.7f * kSweepOctaves;

    // APVTS pointers
    std::atomic<float>* onParam       = nullptr;
    std::atomic<float>* rateParam     = nullptr;
    std::atomic<float>* syncParam     = nullptr;
    std::atomic<float>* divParam      = nullptr;
    std::atomic<float>* shapeParam    = nullptr;
    std::atomic<float>* stagesParam   = nullptr;
    std::atomic<float>* depthParam    = nullptr;
    std::atomic<float>* freqParam     = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* widthParam    = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* outParam      = nullptr;

    // Last seen values (change detection)
    float lastOn     = -1.0f, lastRate     = -1.0f,    lastSync  = -1.0f;
    float lastDiv    = -1.0f, lastShape    = -1.0f,    lastStages = -1.0f;
    float lastDepth  = -1.0f, lastFreq     = -1.0f,    lastFeedback = -1000.0f;
    float lastWidth  = -1.0f, lastMix      = -1.0f,    lastOut   = -1000.0f;
};
