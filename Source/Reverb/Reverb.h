/*
  ==============================================================================

    Reverb.h

    Algorithmic stereo reverb: the Freeverb-derived WDL_ReverbEngine (through
    fxme::Reverb, FxmeTools dsp/Reverb.h) with what a mixing reverb needs
    around it:

      - pre-delay (0 to 200 ms) before the tail, so it separates from the dry
        attack;
      - Size, Damping and Width of the tail;
      - Low cut and High cut on the wet signal only (12 dB per octave TPT
        filters), to keep a tail out of the bass and take the fizz off its top;
      - Mix (dry / wet balance) and Output.

    The On switch fades between the mix and the untouched dry signal, and the
    tail is cleared once the fade is over, so re-engaging never replays a
    stale tail. No latency.

    Guarded against bad input: a NaN or infinity in the signal (one stray
    sample from upstream is enough) would stay in the tail for ever, and a
    huge spike for a minute, as loud noise. The input to the tail is
    cleaned (non-finite samples to zero, clamped at +24 dB), and a tail
    whose output goes out of bounds anyway is cleared.

    Parameter IDs: "<prefix>_Verb_<Name>".

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/Reverb.h>
#include <FxmeTools/dsp/TptSvf.h>
#include <array>
#include <vector>

class ReverbEffect
{
public:
    static constexpr float maxPreDelayMs = 200.0f;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

private:
    void updateFilters();
    void clearTail();

    double currentSampleRate = 44100.0;
    int maxBlock = 512;

    fxme::Reverb reverb;

    // Pre-delay: one ring per channel, long enough for maxPreDelayMs.
    std::array<std::vector<float>, 2> preDelay;
    int preDelayWrite = 0;
    juce::SmoothedValue<float> preDelaySamples;

    std::array<fxme::TptSvf, 2> lowCut, highCut;     // [channel], on the wet signal
    std::vector<float> wetL, wetR;

    juce::SmoothedValue<float> mixSmooth, outSmooth;

    bool on = true;
    bool tailActive = true;          // false once the switch-off fade has finished
    float mix01 = 0.25f, lowCutHz = 120.0f, highCutHz = 9000.0f;

    // APVTS pointers
    std::atomic<float>* onParam       = nullptr;
    std::atomic<float>* preDelayParam = nullptr;
    std::atomic<float>* sizeParam     = nullptr;
    std::atomic<float>* dampParam     = nullptr;
    std::atomic<float>* widthParam    = nullptr;
    std::atomic<float>* lowCutParam   = nullptr;
    std::atomic<float>* highCutParam  = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* outParam      = nullptr;

    // Last seen values (change detection)
    float lastOn = -1.0f, lastPreDelay = -1.0f, lastSize = -1.0f, lastDamp = -1.0f, lastWidth = -1000.0f;
    float lastLowCut = -1.0f, lastHighCut = -1.0f, lastMix = -1.0f, lastOut = -1000.0f;
};
