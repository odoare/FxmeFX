/*
  ==============================================================================

    Crusher.cpp

  ==============================================================================
*/

#include "Crusher.h"
#include <FxmeTools/dsp/Sanitise.h>
#include <cmath>

namespace
{
    constexpr double kSmoothingSeconds = 0.05;
    constexpr double kOnFadeSeconds    = 0.02;
    constexpr float  kDcCutoffHz       = 10.0f;
    constexpr float  kFilterRatio      = 0.45f;   // filter cutoff, against the target rate
    constexpr std::uint64_t kJitterSeed = 0x6372757368ull;   // "crush"
    constexpr int    kTapChunk         = 256;
    constexpr double kTapSeconds       = 0.02;
}

Crusher::Crusher()
{
    // Fixed seeds: a bounce renders the same every time.
    ditherNoise[0].setSeed (0x4c656674);
    ditherNoise[1].setSeed (0x5269676874);

    // The defaults: checkParameters() only applies what differs from them.
    quantizer.setBits (bits);

    antiAlias.setType (fxme::SvfPassCascade::Type::lowPass);
    antiAlias.setNumSections (4);   // 48 dB/octave
    smoothing.setType (fxme::SvfPassCascade::Type::lowPass);
    smoothing.setNumSections (2);   // 24 dB/octave

    // Sized here too, so a GUI opened before prepare() finds a valid ring.
    inputTap .prepare (currentSampleRate, kTapSeconds);
    outputTap.prepare (currentSampleRate, kTapSeconds);
    tapInScratch .assign ((size_t) kTapChunk, 0.0f);
    tapOutScratch.assign ((size_t) kTapChunk, 0.0f);
}

void Crusher::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused (samplesPerBlock, numChannels);
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    antiAlias.prepare (currentSampleRate);
    smoothing.prepare (currentSampleRate);
    updateRate();
    antiAlias.setFrequency (kFilterRatio * rateHz, true);
    smoothing.setFrequency (kFilterRatio * rateHz, true);

    for (auto& h : holds)
        h.reset();

    dcCoef = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * kDcCutoffHz / currentSampleRate);
    dcX1 = {};
    dcY1 = {};

    mixSmooth  .reset (currentSampleRate, kOnFadeSeconds);
    outSmooth  .reset (currentSampleRate, kSmoothingSeconds);
    driveSmooth.reset (currentSampleRate, kSmoothingSeconds);
    cleanSmooth.reset (currentSampleRate, kSmoothingSeconds);
    smoothSmooth.reset (currentSampleRate, kSmoothingSeconds);

    mixSmooth  .setCurrentAndTargetValue (on ? mix01 : 0.0f);
    outSmooth  .setCurrentAndTargetValue (on ? juce::Decibels::decibelsToGain (outDb) : 1.0f);
    driveSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (driveDb));
    cleanSmooth.setCurrentAndTargetValue (clean ? 1.0f : 0.0f);
    smoothSmooth.setCurrentAndTargetValue (smooth ? 1.0f : 0.0f);
    active = true;

    inputTap .prepare (currentSampleRate, kTapSeconds);
    outputTap.prepare (currentSampleRate, kTapSeconds);
    tapFill = 0;
}

void Crusher::updateRate()
{
    decimating = rateHz < kMaxRateHz && rateHz < (float) currentSampleRate;
    const float factor = decimating ? (float) currentSampleRate / rateHz : 1.0f;
    for (auto& h : holds)
        h.setFactor (factor);
    antiAlias.setFrequency (kFilterRatio * rateHz);
    smoothing.setFrequency (kFilterRatio * rateHz);
}

void Crusher::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                             const juce::String& prefix)
{
    using Range = juce::NormalisableRange<float>;
    auto addFloat = [&] (const char* id, const char* name, Range range, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "_Crush_" + id, 1 }, prefix + " Crush " + name, range, def));
    };
    auto addBool = [&] (const char* id, const char* name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { prefix + "_Crush_" + id, 1 }, prefix + " Crush " + name, def));
    };

    addBool  ("On",     "On", true);
    addFloat ("Drive",  "Drive",  Range (0.0f, 24.0f, 0.1f),                     0.0f);
    addFloat ("Bits",   "Bits",   Range (1.0f, 24.0f, 0.01f, 0.5f),              8.0f);
    // The rate reads "Off" at the top of its range (no decimation), in kHz above 1 kHz.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Crush_Rate", 1 }, prefix + " Crush Rate",
        Range (kMinRateHz, kMaxRateHz, 1.0f, 0.25f), 11025.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                if (v >= kMaxRateHz - 0.5f)
                    return juce::String ("Off");
                return v >= 1000.0f ? juce::String (v * 0.001f, 1) + " kHz"
                                    : juce::String (juce::roundToInt (v)) + " Hz";
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                const auto t = text.trim().toLowerCase();
                if (t.startsWith ("off"))
                    return kMaxRateHz;
                const float v = t.getFloatValue();
                return t.contains ("k") ? v * 1000.0f : v;
            })));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Crush_Mode", 1 }, prefix + " Crush Mode",
        juce::StringArray { "Mid-tread", "Mid-rise", "Mu-law" }, 0));
    addFloat ("Jitter", "Jitter", Range (0.0f, 100.0f, 0.1f),                    0.0f);
    addFloat ("Noise",  "Noise",  Range (0.0f, 100.0f, 0.1f),                    0.0f);
    addBool  ("Clean",  "Clean",  false);
    addBool  ("Smooth", "Smooth", false);
    addFloat ("Mix",    "Mix",    Range (0.0f, 100.0f, 0.1f),                  100.0f);
    addFloat ("Out",    "Output", Range (-24.0f, 24.0f, 0.1f),                   0.0f);
}

void Crusher::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    auto p = [&] (const char* id) { return apvts.getRawParameterValue (prefix + "_Crush_" + id); };
    onParam     = p ("On");
    driveParam  = p ("Drive");
    bitsParam   = p ("Bits");
    rateParam   = p ("Rate");
    modeParam   = p ("Mode");
    jitterParam = p ("Jitter");
    noiseParam  = p ("Noise");
    cleanParam  = p ("Clean");
    smoothParam = p ("Smooth");
    mixParam    = p ("Mix");
    outParam    = p ("Out");
}

void Crusher::checkParameters()
{
    // Reads one parameter into `value`; true when it changed.
    auto read = [] (std::atomic<float>* param, float& value)
    {
        if (param == nullptr)
            return false;
        const float v = param->load();
        if (v == value)
            return false;
        value = v;
        return true;
    };
    auto readBool = [] (std::atomic<float>* param, bool& value)
    {
        if (param == nullptr || (param->load() > 0.5f) == value)
            return false;
        value = ! value;
        return true;
    };

    if (read (bitsParam, bits))
        quantizer.setBits (bits);

    float modeF = (float) mode;
    if (read (modeParam, modeF))
    {
        mode = juce::jlimit (0, 2, (int) modeF);
        quantizer.setMode ((fxme::BitCrusher::Mode) mode);
    }

    if (read (rateParam, rateHz))
        updateRate();

    float jitterPct = jitter01 * 100.0f;
    if (read (jitterParam, jitterPct))
    {
        jitter01 = jitterPct * 0.01f;
        for (auto& h : holds)
            h.setJitter (jitter01, kJitterSeed);
    }

    float noisePct = dither01 * 100.0f;
    if (read (noiseParam, noisePct))
        dither01 = noisePct * 0.01f;

    if (readBool (cleanParam, clean))
        cleanSmooth.setTargetValue (clean ? 1.0f : 0.0f);
    if (readBool (smoothParam, smooth))
        smoothSmooth.setTargetValue (smooth ? 1.0f : 0.0f);

    if (read (driveParam, driveDb))
        driveSmooth.setTargetValue (juce::Decibels::decibelsToGain (driveDb));

    float mixPct = mix01 * 100.0f;
    if (read (mixParam, mixPct))
    {
        mix01 = mixPct * 0.01f;
        if (on)
            mixSmooth.setTargetValue (mix01);
    }
    if (read (outParam, outDb) && on)
        outSmooth.setTargetValue (juce::Decibels::decibelsToGain (outDb));

    if (readBool (onParam, on))
    {
        // A fade between the crushed blend and the untouched signal.
        mixSmooth.setTargetValue (on ? mix01 : 0.0f);
        outSmooth.setTargetValue (on ? juce::Decibels::decibelsToGain (outDb) : 1.0f);
        if (on)
            active = true;
    }
}

void Crusher::flushTaps()
{
    if (tapFill <= 0)
        return;
    inputTap .push (tapInScratch.data(),  tapFill);
    outputTap.push (tapOutScratch.data(), tapFill);
    tapFill = 0;
}

void Crusher::process (juce::AudioBuffer<float>& buffer)
{
    if (! on && ! active)
        return;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), (int) holds.size());
    if (numChannels <= 0 || numSamples <= 0)
        return;

    // The filters and the DC blocker hold state: keep NaN and infinities out.
    fxme::sanitiseBlock (buffer.getArrayOfWritePointers(), numChannels, numSamples, fxme::kMainPathLimit);
    auto* const* data = buffer.getArrayOfWritePointers();

    const bool tapping = inputTap.isEnabled() || outputTap.isEnabled();

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix    = mixSmooth.getNextValue();
        const float outG   = outSmooth.getNextValue();
        const float drive  = driveSmooth.getNextValue();
        const float cleanA = cleanSmooth.getNextValue();
        const float smoothA = smoothSmooth.getNextValue();
        if (decimating)
        {
            antiAlias.advance();
            smoothing.advance();
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = data[ch][i];

            // Into the converter: driven, clipped at full scale.
            float x = juce::jlimit (-1.0f, 1.0f, dry * drive);
            if (decimating && cleanA > 0.0f)
                x += cleanA * (antiAlias.process (ch, x) - x);
            else if (decimating)
                antiAlias.process (ch, x);   // keeps the filter warm for a crossfade in

            const float dither = dither01 > 0.0f
                                   ? dither01 * (ditherNoise[(size_t) ch].nextFloat() - ditherNoise[(size_t) ch].nextFloat())
                                   : 0.0f;
            float y = holds[(size_t) ch].processSample (quantizer.processSample (x, dither));

            if (decimating)
            {
                const float s = smoothing.process (ch, y);
                y += smoothA * (s - y);
            }

            // DC blocker.
            const float dc = y - dcX1[(size_t) ch] + dcCoef * dcY1[(size_t) ch];
            dcX1[(size_t) ch] = y;
            dcY1[(size_t) ch] = std::abs (dc) < 1.0e-15f ? 0.0f : dc;

            const float out = (dry + mix * (dc - dry)) * outG;
            data[ch][i] = out;

            if (tapping && ch == 0)
            {
                tapInScratch [(size_t) tapFill] = dry;
                tapOutScratch[(size_t) tapFill] = out;
            }
        }

        if (tapping && ++tapFill == kTapChunk)
            flushTaps();
    }
    if (tapping)
        flushTaps();

    // The switch-off fade has finished: out of the audio path, and clean
    // state for the next time it is switched on.
    if (! on && ! mixSmooth.isSmoothing() && ! outSmooth.isSmoothing())
    {
        antiAlias.reset();
        smoothing.reset();
        for (auto& h : holds)
            h.reset();
        dcX1 = {};
        dcY1 = {};
        active = false;
    }
}
