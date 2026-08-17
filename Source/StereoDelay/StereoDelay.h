/*
  ==============================================================================

    StereoDelay.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/Biquad.h>              // fxme::Biquad / fxme::BiquadCoeffs
#include <FxmeTools/dsp/DelayTimeResolver.h>   // fxme::DelayTimeResolver
#include <vector>
#include <atomic>

class StereoDelay
{
public:
    StereoDelay();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);

    void assignParameters(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);
    void setOn (bool shouldBeOn);
    bool isOn() const { return on; }
    void setBPM(double bpm);
    double getBPM() const { return currentBPM; }

    /** Pitch that the MIDI-note delay mode locks to. Fed from the note-ons of
        the incoming MIDI; zero until something has played. */
    void setLastNoteHz (float hz);
    float getLastNoteHz() const { return lastNoteHz.load(); }

    /** What a side's delay value currently comes to in seconds, under its own
        mode and the current transport/pitch context. For the GUI read-out. */
    float resolvedSeconds (bool rightSide) const;

    /** Longest delay the lines are sized for. The sync and MIDI-note modes can
        ask for far more than the time parameter's own 0…2 range suggests, so
        the buffer has room beyond it and the resolver clamps to it. Six seconds
        is three whole notes at 120 BPM. */
    static constexpr float maxDelaySeconds = 6.0f;

    /** Delay-mode choices, in the order they are stored as a choice index. */
    static juce::StringArray delayModeChoices()
    {
        return { "Seconds", "DAW sync", "MIDI note" };
    }

    /** Version-1 state stored one meaning only: the time values were beats.
        Re-points such a state at the DAW-sync mode and rescales the values,
        which are now a proportion of a whole note rather than of a beat, so an
        old session or preset keeps the delay time it was saved with. */
    static void migrateLegacyState (juce::XmlElement& state, const juce::String& prefix);

private:
    fxme::DelayTimeResolver makeResolver() const;
    void updateDelayTimes();
    void updateGains();
    void updateFilter();

    double currentSampleRate = 44100.0;
    int maxDelaySamples = 0;

    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;

    fxme::Biquad filterL, filterR;   // damping in the feedback path

    // Parameter cache. The two delay values carry no unit of their own: what
    // they mean is the matching mode's business (seconds, a proportion of a
    // whole note, or a multiple of the last MIDI note's period).
    float delayValueL = 0.5f, delayValueR = 0.75f;
    fxme::DelayTimeMode modeL = fxme::DelayTimeMode::seconds;
    fxme::DelayTimeMode modeR = fxme::DelayTimeMode::seconds;
    std::atomic<float> lastNoteHz { 0.0f };
    float feedbackL = -6.0f, feedbackR = -6.0f;
    float crossFeedback = -60.0f;
    float filterCutoff = 5000.0f, filterQ = 1.0f;
    float dryGain = 0.0f;
    float wetGain = -6.0f;
    bool on = true;

    // Smoothed gains
    double currentBPM = 120.0;
    float feedbackLGain = 0.0f, feedbackRGain = 0.0f;
    float crossFeedbackGain = 0.0f;
    float dryGainLinear = 1.0f, wetGainLinear = 0.5f;

    // Delay times in samples
    int delaySamplesL = 0, delaySamplesR = 0;

    // Parameter Pointers
    std::atomic<float>* delayLParam = nullptr;
    std::atomic<float>* delayRParam = nullptr;
    std::atomic<float>* fdbkLParam = nullptr;
    std::atomic<float>* fdbkRParam = nullptr;
    std::atomic<float>* crossFdbkParam = nullptr;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* qParam = nullptr;
    std::atomic<float>* dryGainParam = nullptr;
    std::atomic<float>* wetGainParam = nullptr;
    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* modeLParam = nullptr;
    std::atomic<float>* modeRParam = nullptr;

    // Last values for change detection
    float lastDelayL = -1.0f, lastDelayR = -1.0f;
    float lastModeL = -1.0f, lastModeR = -1.0f;
    float lastFdbkL = -100.0f, lastFdbkR = -100.0f;
    float lastCrossFdbk = -100.0f;
    float lastCutoff = -1.0f, lastQ = -1.0f;
    float lastDryGain = -100.0f;
    float lastWetGain = -100.0f;
    float lastOn = -1.0f;
};