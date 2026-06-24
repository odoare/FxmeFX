/*
  ==============================================================================

    Equalizer.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/dsp/Biquad.h>   // fxme::Biquad / fxme::BiquadCoeffs
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
    static constexpr int NumBands = 5;

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
        const char* suffix;     // APVTS ID suffix (kept for preset back-compat)
        float       minFreq;
        float       maxFreq;
        float       defFreq;
        BandType    defType;
    };

    Equalizer();

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void setOn (bool shouldBeOn);
    bool isOn() const;

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix);

    /** Per-band configuration (legacy suffix, freq range, defaults). */
    static const BandConfig& getBandConfig (int bandIndex) noexcept;
    /** Display names for the band-type choice parameter, in BandType enum order. */
    static juce::StringArray getBandTypeNames();

private:
    struct ChannelStrip
    {
        std::array<fxme::Biquad, NumBands> band;
    };

    struct BandCache
    {
        BandType type = BandType::Peak;
        float    f = 1000.0f, q = 1.0f, g = 0.0f;
    };

    struct BandParamPtrs
    {
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* q    = nullptr;
        std::atomic<float>* gain = nullptr;
    };

    struct BandLast
    {
        float type = -1.0f;
        float freq = -1.0f;
        float q    = -1.0f;
        float gain = -100.0f;
    };

    std::vector<ChannelStrip> channels;
    double currentSampleRate = 44100.0;
    bool   on = true;
    float  postGain = 1.0f;

    std::array<BandCache, NumBands>     bandCache;
    std::array<BandParamPtrs, NumBands> bandParams;
    std::array<BandLast, NumBands>      bandLast;

    std::atomic<float>* onParam       = nullptr;
    std::atomic<float>* postGainParam = nullptr;
    float lastOn       = -1.0f;
    float lastPostGain = -100.0f;

    void updateCoefficients();
    void calcByType (fxme::Biquad& bq, const BandCache& bc);
};
