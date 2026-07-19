/*
  ==============================================================================

    Equalizer.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/Biquad.h>       // fxme::Biquad / fxme::BiquadCoeffs
#include <FxmeTools/dsp/SpectrumTap.h>  // fxme::SpectrumTap
#include <array>
#include <vector>
#include <atomic>

/**
 * @class Equalizer
 * @brief A 5-band equalizer where each band can be Low Pass, High Pass, Peak,
 *        Low Shelf or High Shelf.
 *
 * Backwards-compatibility note: band 0 defaults to Low Shelf, band 4 to High
 * Shelf, bands 1-3 to Peak. Existing presets that did not store a band-type
 * parameter therefore behave exactly as before.
 */
class Equalizer
{
public:
    /** Compile-time capacity: the largest band count any instance may request.
        Fixed-size arrays are sized to this; only the first getNumBands() entries
        are used. */
    static constexpr int MaxBands = 8;
    /** Default band count. Kept at the historical value so existing projects
        that construct an Equalizer without arguments are unchanged. */
    static constexpr int DefaultNumBands = 5;

    enum class BandType
    {
        Lowpass = 0,
        Highpass,
        Peak,
        Lowshelf,
        Highshelf
    };

    struct BandConfig
    {
        juce::String suffix;    // APVTS ID suffix (kept for preset back-compat)
        float        minFreq;
        float        maxFreq;
        float        defFreq;
        BandType     defType;
    };

    /** @param numBandsToUse  number of active bands, in [2, MaxBands]. Defaults
                              to the historical value (DefaultNumBands).
        @param enableSpectrum when true, the input/output spectrum taps are fed
                              during process() so the editor can display live
                              spectra. Off by default so components embedded in
                              other projects keep their previous behaviour. */
    explicit Equalizer (int numBandsToUse = DefaultNumBands, bool enableSpectrum = false);

    /** Number of active bands for this instance (set at construction). */
    int getNumBands() const noexcept               { return numBands; }

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void setOn (bool shouldBeOn);
    bool isOn() const;

    /** True when this instance feeds its spectrum taps (set at construction). */
    bool hasSpectrum() const noexcept              { return spectrumEnabled; }
    /** Mono-sum spectrum taps (pre- and post-EQ). Valid only when hasSpectrum(). */
    fxme::SpectrumTap& getInputTap()  noexcept     { return inputTap;  }
    fxme::SpectrumTap& getOutputTap() noexcept     { return outputTap; }
    double getSampleRate() const noexcept          { return currentSampleRate; }

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix,
                               int numBands = DefaultNumBands);

    /** Per-band configuration (suffix, freq range, defaults) for a layout of
        `numBands` bands. Band 0 is a low shelf, the last a high shelf, the rest
        peaks. The 5-band layout reproduces the historical defaults exactly. */
    static BandConfig getBandConfig (int bandIndex, int numBands) noexcept;
    /** Display names for the band-type choice parameter, in BandType enum order. */
    static juce::StringArray getBandTypeNames();

private:
    struct ChannelStrip
    {
        std::array<fxme::Biquad, MaxBands> band;
    };

    struct BandCache
    {
        BandType type = BandType::Peak;
        float    f = 1000.0f, q = 1.0f, g = 0.0f;
        bool     on = true;
    };

    struct BandParamPtrs
    {
        std::atomic<float>* on   = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* q    = nullptr;
        std::atomic<float>* gain = nullptr;
    };

    struct BandLast
    {
        float on   = -1.0f;
        float type = -1.0f;
        float freq = -1.0f;
        float q    = -1.0f;
        float gain = -100.0f;
    };

    std::vector<ChannelStrip> channels;
    int    numBands = DefaultNumBands;
    double currentSampleRate = 44100.0;
    bool   on = true;
    float  postGain = 1.0f;

    bool              spectrumEnabled = false;
    fxme::SpectrumTap inputTap, outputTap;
    void pushSum (fxme::SpectrumTap& tap, const juce::AudioBuffer<float>& buffer) noexcept;

    std::array<BandCache, MaxBands>     bandCache;
    std::array<BandParamPtrs, MaxBands> bandParams;
    std::array<BandLast, MaxBands>      bandLast;

    std::atomic<float>* onParam       = nullptr;
    std::atomic<float>* postGainParam = nullptr;
    float lastOn       = -1.0f;
    float lastPostGain = -100.0f;

    void updateCoefficients();
    void calcByType (fxme::Biquad& bq, const BandCache& bc);
};
