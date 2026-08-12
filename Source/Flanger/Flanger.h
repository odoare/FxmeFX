/*
  ==============================================================================

    Flanger.h

    Classic stereo flanger: one very short modulated delay per channel, fed
    back into itself. Summing the swept delay with the dry signal produces a
    comb of moving notches; the feedback sharpens them into the familiar jet
    whoosh, and taking it negative flips the comb (notches become peaks) for
    the hollower, more metallic variant.

    Delay sets the shortest delay of the sweep (the classic "manual" control)
    and Depth how far above it the LFO travels, so the two knobs behave the
    way they do on a pedal: Delay places the comb, Depth sets how far it
    moves.

    The two channels share one fxme::ModLfo and read it at a phase offset set
    by Width, so the sweep crosses the stereo field instead of pumping both
    sides at once. The rate is free (Hz) or locked to the host tempo, in which
    case the phase is re-derived from the playhead's PPQ position at the top
    of every block.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/ModDelayLine.h>
#include <FxmeTools/dsp/ModLfo.h>
#include <array>
#include <vector>

class Flanger
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** Host tempo, for the tempo-synced rate. */
    void setBPM (double bpm);
    double getBPM() const { return currentBPM; }

    /** Host tempo plus the timeline position, so a synced sweep can be locked
        to the bar. `isPlaying` false leaves the LFO free-running. */
    void setTransport (double bpm, double ppqPosition, bool isPlaying);

    /** The rate the LFO actually runs at, in Hz (tempo-derived while synced). */
    float getEffectiveRateHz() const { return lfo.frequencyHz(); }

private:
    void updateDelayTargets();

    static constexpr float kMaxDelayMs = 10.0f;   // Delay knob ceiling
    static constexpr float kMaxSweepMs = 10.0f;   // widest sweep above it

    double currentSampleRate = 44100.0;
    double currentBPM        = 120.0;
    double hostPpq           = 0.0;
    bool   hostPlaying       = false;

    std::array<fxme::ModDelayLine, 2> lines;
    fxme::ModLfo lfo;

    juce::SmoothedValue<float> mixSmooth, outSmooth, baseSmooth, sweepSmooth, feedbackSmooth;

    bool  on          = true;
    bool  linesActive = true;    // false once the dry-only fade has finished
    float widthOffset = 0.5f;    // right-channel phase offset, in cycles

    // Parameter cache
    float delayMs = 1.0f, depth01 = 0.7f, mix01 = 0.5f;

    // APVTS pointers
    std::atomic<float>* onParam       = nullptr;
    std::atomic<float>* rateParam     = nullptr;
    std::atomic<float>* syncParam     = nullptr;
    std::atomic<float>* divParam      = nullptr;
    std::atomic<float>* shapeParam    = nullptr;
    std::atomic<float>* depthParam    = nullptr;
    std::atomic<float>* delayParam    = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* widthParam    = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* outParam      = nullptr;

    // Last seen values (change detection)
    float lastOn    = -1.0f, lastRate     = -1.0f, lastSync  = -1.0f;
    float lastDiv   = -1.0f, lastShape    = -1.0f, lastDepth = -1.0f;
    float lastDelay = -1.0f, lastFeedback = -1000.0f, lastWidth = -1.0f;
    float lastMix   = -1.0f, lastOut      = -1000.0f;
};
