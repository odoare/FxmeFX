/*
  ==============================================================================

    Chorus.h

    Classic stereo chorus: two to four modulated delay taps per channel, each
    reading the same fxme::ModDelayLine at a different point of one shared
    fxme::ModLfo cycle, summed and blended with the dry signal. No feedback —
    that is what separates a chorus from the flanger next door; the effect
    comes purely from the slow detune of several delayed copies.

    Stereo comes from two places: the right channel's taps sit half a cycle
    (at Width 100 %) ahead of the left channel's, and each extra voice is
    spread evenly around the cycle, so the copies drift apart instead of
    moving as one.

    The LFO rate is either free (Hz) or locked to the host tempo, in which
    case the phase is re-derived from the playhead's PPQ position at the top
    of every block, so the sweep lands in the same place on every pass over
    the same bar.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/ModDelayLine.h>
#include <FxmeTools/dsp/ModLfo.h>
#include <array>
#include <vector>

class Chorus
{
public:
    static constexpr int maxVoices = 4;

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

    /** The rate the LFO actually runs at, in Hz (tempo-derived while synced).
        Message-thread friendly read for the GUI. */
    float getEffectiveRateHz() const { return lfo.frequencyHz(); }

private:
    void updateDelayTargets();

    static constexpr float kMaxDelayMs = 40.0f;   // Delay knob ceiling
    static constexpr float kMaxSwingMs = 20.0f;   // widest modulation swing

    double currentSampleRate = 44100.0;
    double currentBPM        = 120.0;
    double hostPpq           = 0.0;
    bool   hostPlaying       = false;

    std::array<fxme::ModDelayLine, 2> lines;
    fxme::ModLfo lfo;

    juce::SmoothedValue<float> mixSmooth, outSmooth, centreSmooth, swingSmooth;

    bool  on          = true;
    bool  linesActive = true;    // false once the dry-only fade has finished
    int   voices      = 2;
    float widthOffset = 0.5f;    // right-channel phase offset, in cycles

    // Parameter cache
    float delayMs = 12.0f, depth01 = 0.4f, mix01 = 0.5f;

    // APVTS pointers
    std::atomic<float>* onParam     = nullptr;
    std::atomic<float>* rateParam   = nullptr;
    std::atomic<float>* syncParam   = nullptr;
    std::atomic<float>* divParam    = nullptr;
    std::atomic<float>* shapeParam  = nullptr;
    std::atomic<float>* depthParam  = nullptr;
    std::atomic<float>* delayParam  = nullptr;
    std::atomic<float>* voicesParam = nullptr;
    std::atomic<float>* widthParam  = nullptr;
    std::atomic<float>* mixParam    = nullptr;
    std::atomic<float>* outParam    = nullptr;

    // Last seen values (change detection)
    float lastOn     = -1.0f, lastRate  = -1.0f, lastSync  = -1.0f;
    float lastDiv    = -1.0f, lastShape = -1.0f, lastDepth = -1.0f;
    float lastDelay  = -1.0f, lastVoices = -1.0f, lastWidth = -1.0f;
    float lastMix    = -1.0f, lastOut   = -1000.0f;
};
