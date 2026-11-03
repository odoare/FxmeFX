/*
  ==============================================================================

    Freeze.h

    Spectral freeze effect built on fxme::SpectralFreezeMulti (paulstretch-
    style FFT freeze from FxmeTools). Engaging the effect freezes the
    2048-sample window of input *preceding* the switch (a per-channel
    history ring is kept at all times and replayed through the freezer's
    capture in one go at the engage moment), so the wash starts in the very
    next audio block — the only delays left are the host's block latency
    and the freezer's own ~11 ms capture->wash crossfade. Disengaging fades
    back to the dry signal.

    The wet/dry mix is applied here (not through SpectralFreezeMulti's own
    mix) with a per-sample SmoothedValue, so both the Mix knob and the
    on/off switch are click-free: switching off ramps the wash out over the
    smoothing time before the synthesis is stopped.

    Also owns a tap of the mono-summed input (fxme::SpectrumTap) that the
    editor's spectrum display drains from the message thread. Under
    FxmeNet the tap travels as telemetry, so the display of the editor in
    the DAW shows the audio processed on the server.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/SpectralFreeze.h>
#include <FxmeTools/dsp/SpectrumTap.h>
#include <atomic>
#include <cstdint>
#include <vector>

class Freeze
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** True while the freeze switch is on (capture + wash). Safe to read
        from the message thread — the spectrum display freezes on this.
        Read from the parameter itself, so that it is right in an instance
        that does not process (an FxmeNet mirror). */
    bool isEngaged() const
    {
        return onParam != nullptr ? onParam->load() > 0.5f
                                  : engaged.load (std::memory_order_relaxed);
    }

    double getSampleRate() const { return currentSampleRate; }

    /** Message thread: drains up to maxSamples of the mono-summed input tap.
        Returns the number of samples written to dest. */
    int pullDisplaySamples (float* dest, int maxSamples);

    /** The tap the display drains (registered for FxmeNet telemetry). */
    fxme::SpectrumTap& getDisplayTap() noexcept { return displayTap; }

private:
    void pushDisplaySamples (const juce::AudioBuffer<float>& buffer);
    void pushHistory (const juce::AudioBuffer<float>& buffer);
    void captureFromHistory();

    fxme::SpectralFreezeMulti freezer;
    juce::AudioBuffer<float>  dryBuffer;

    // Rolling window of the last kSize input samples per channel, so an
    // engage can freeze the audio *before* the switch was hit.
    juce::AudioBuffer<float> history, historyScratch;
    int histPos = 0;

    double currentSampleRate = 48000.0;

    std::atomic<bool> engaged { false };
    bool washActive     = false;   // synthesis running (until the fade-out ends)
    bool pendingCapture = false;   // engage seen; freeze the history next block

    juce::SmoothedValue<float> mixSmooth;   // 0 when off, Mix value when on
    float mixValue = 0.5f;

    fxme::SpectrumTap displayTap;       // written by the audio thread (or FxmeNet telemetry)
    std::int64_t displayCursor = 0;     // the display's read position (message thread)

    // APVTS pointers
    std::atomic<float>* onParam    = nullptr;
    std::atomic<float>* mixParam   = nullptr;
    std::atomic<float>* widthParam = nullptr;

    // Last seen values (change-detection)
    float lastOn    = -1.0f;
    float lastMix   = -1.0f;
    float lastWidth = -1.0f;
};
