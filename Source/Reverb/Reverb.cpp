/*
  ==============================================================================

    Reverb.cpp

  ==============================================================================
*/

#include "Reverb.h"

namespace
{
    constexpr double kSmoothingSeconds = 0.05;
    constexpr double kButterworthK = 1.4142135623730951;   // 1 / Q, Q = 0.707

    // The loudest input the tail takes (+24 dB): anything beyond, or a NaN or
    // infinity, would sit in the combs for a minute (or for ever, a NaN).
    constexpr float kMaxInput = 16.0f;
    // A wet sample beyond this means the tail holds garbage: it is cleared.
    constexpr float kMaxWet = 1000.0f;

    inline float sanitise (float x) noexcept
    {
        return std::isfinite (x) ? juce::jlimit (-kMaxInput, kMaxInput, x) : 0.0f;
    }
}

void ReverbEffect::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused (numChannels);

    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxBlock = juce::jmax (1, samplesPerBlock);

    reverb.prepare (currentSampleRate, maxBlock);

    const int ring = (int) std::ceil (maxPreDelayMs * 0.001 * currentSampleRate) + 4;
    for (auto& line : preDelay)
        line.assign ((size_t) ring, 0.0f);
    preDelayWrite = 0;

    wetL.assign ((size_t) maxBlock, 0.0f);
    wetR.assign ((size_t) maxBlock, 0.0f);

    for (auto* filters : { &lowCut, &highCut })
        for (auto& f : *filters)
            f.reset();
    updateFilters();

    preDelaySamples.reset (currentSampleRate, kSmoothingSeconds);
    // The pre-delay in samples follows the new rate (a ramp computed at a
    // higher rate could reach past the ring).
    if (lastPreDelay >= 0.0f)
        preDelaySamples.setCurrentAndTargetValue (lastPreDelay * 0.001f * (float) currentSampleRate);
    mixSmooth.reset (currentSampleRate, kSmoothingSeconds);
    outSmooth.reset (currentSampleRate, kSmoothingSeconds);
    mixSmooth.setCurrentAndTargetValue (on ? mix01 : 0.0f);
    if (! on)
        outSmooth.setCurrentAndTargetValue (1.0f);

    tailActive = true;
}

void ReverbEffect::updateFilters()
{
    const double nyquistSafe = 0.45 * currentSampleRate;
    const double lo = juce::jlimit (10.0, nyquistSafe, (double) lowCutHz);
    const double hi = juce::jlimit (lo, nyquistSafe, (double) highCutHz);
    for (auto& f : lowCut)
        f.setup (fxme::TptSvf::gFor (lo, currentSampleRate), kButterworthK);
    for (auto& f : highCut)
        f.setup (fxme::TptSvf::gFor (hi, currentSampleRate), kButterworthK);
}

void ReverbEffect::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                            const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Verb_On", 1 }, prefix + " Reverb On", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_PreDelay", 1 }, prefix + " Reverb Pre-delay",
        juce::NormalisableRange<float> (0.0f, maxPreDelayMs, 0.1f, 0.5f), 10.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_Size", 1 }, prefix + " Reverb Size",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_Damp", 1 }, prefix + " Reverb Damping",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_Width", 1 }, prefix + " Reverb Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_LowCut", 1 }, prefix + " Reverb Low cut",
        juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.4f), 120.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_HighCut", 1 }, prefix + " Reverb High cut",
        juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.4f), 9000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_Mix", 1 }, prefix + " Reverb Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 25.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Verb_Out", 1 }, prefix + " Reverb Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
}

void ReverbEffect::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam       = apvts.getRawParameterValue (prefix + "_Verb_On");
    preDelayParam = apvts.getRawParameterValue (prefix + "_Verb_PreDelay");
    sizeParam     = apvts.getRawParameterValue (prefix + "_Verb_Size");
    dampParam     = apvts.getRawParameterValue (prefix + "_Verb_Damp");
    widthParam    = apvts.getRawParameterValue (prefix + "_Verb_Width");
    lowCutParam   = apvts.getRawParameterValue (prefix + "_Verb_LowCut");
    highCutParam  = apvts.getRawParameterValue (prefix + "_Verb_HighCut");
    mixParam      = apvts.getRawParameterValue (prefix + "_Verb_Mix");
    outParam      = apvts.getRawParameterValue (prefix + "_Verb_Out");
}

void ReverbEffect::checkParameters()
{
    auto changed = [] (std::atomic<float>* p, float& last)
    {
        if (p == nullptr)
            return false;
        const float v = p->load();
        if (! (v < last || v > last))
            return false;
        last = v;
        return true;
    };

    if (changed (preDelayParam, lastPreDelay))
        preDelaySamples.setTargetValue (lastPreDelay * 0.001f * (float) currentSampleRate);
    if (changed (sizeParam, lastSize))
        reverb.setRoomSize (lastSize * 0.01f);
    if (changed (dampParam, lastDamp))
        reverb.setDamping (lastDamp * 0.01f);
    if (changed (widthParam, lastWidth))
        reverb.setWidth (lastWidth * 0.01f);

    bool filters = false;
    if (changed (lowCutParam, lastLowCut))
    {
        lowCutHz = lastLowCut;
        filters = true;
    }
    if (changed (highCutParam, lastHighCut))
    {
        highCutHz = lastHighCut;
        filters = true;
    }
    if (filters)
        updateFilters();

    if (changed (mixParam, lastMix))
    {
        mix01 = lastMix * 0.01f;
        if (on)
            mixSmooth.setTargetValue (mix01);
    }
    if (changed (outParam, lastOut) && on)
        outSmooth.setTargetValue (juce::Decibels::decibelsToGain (lastOut));

    if (changed (onParam, lastOn))
    {
        on = lastOn > 0.5f;
        // A fade between the mix and the dry signal, so the switch never
        // clicks; the tail keeps running until the fade is over.
        mixSmooth.setTargetValue (on ? mix01 : 0.0f);
        outSmooth.setTargetValue (on ? juce::Decibels::decibelsToGain (outParam != nullptr ? outParam->load() : 0.0f) : 1.0f);
        if (on)
            tailActive = true;
    }
}

void ReverbEffect::process (juce::AudioBuffer<float>& buffer)
{
    if (! on && ! tailActive)
        return;

    const int numChannels = buffer.getNumChannels();
    if (numChannels <= 0)
        return;
    float* left = buffer.getWritePointer (0);
    float* right = numChannels > 1 ? buffer.getWritePointer (1) : left;
    const int ringSize = (int) preDelay[0].size();

    for (int start = 0; start < buffer.getNumSamples(); start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, buffer.getNumSamples() - start);

        // Pre-delay: write the dry signal, read it back late (linear
        // interpolation, so a moving pre-delay glides). The read index is
        // integer arithmetic on the write index, so it never leaves the ring.
        for (int i = 0; i < n; ++i)
        {
            preDelay[0][(size_t) preDelayWrite] = sanitise (left[start + i]);
            preDelay[1][(size_t) preDelayWrite] = sanitise (right[start + i]);
            const float d = juce::jlimit (0.0f, (float) (ringSize - 2), preDelaySamples.getNextValue());
            const int whole = (int) d;
            const float f = d - (float) whole;
            int r0 = preDelayWrite - whole;
            if (r0 < 0)
                r0 += ringSize;
            const int r1 = r0 > 0 ? r0 - 1 : ringSize - 1;     // one sample older
            wetL[(size_t) i] = preDelay[0][(size_t) r0] + f * (preDelay[0][(size_t) r1] - preDelay[0][(size_t) r0]);
            wetR[(size_t) i] = preDelay[1][(size_t) r0] + f * (preDelay[1][(size_t) r1] - preDelay[1][(size_t) r0]);
            if (++preDelayWrite >= ringSize)
                preDelayWrite = 0;
        }

        reverb.process (wetL.data(), wetR.data(), wetL.data(), wetR.data(), n);

        // A tail that went bad (which the input guard should prevent) is
        // cleared rather than left ringing: a block of silence, then clean.
        bool healthy = true;
        for (int i = 0; i < n && healthy; ++i)
            healthy = std::abs (wetL[(size_t) i]) <= kMaxWet && std::abs (wetR[(size_t) i]) <= kMaxWet;  // false for a NaN
        if (! healthy)
        {
            clearTail();
            std::fill (wetL.begin(), wetL.begin() + n, 0.0f);
            std::fill (wetR.begin(), wetR.begin() + n, 0.0f);
        }

        for (int i = 0; i < n; ++i)
        {
            const float wl = highCut[0].tick (lowCut[0].tick (wetL[(size_t) i]).high).low;
            const float wr = highCut[1].tick (lowCut[1].tick (wetR[(size_t) i]).high).low;
            const float mix = mixSmooth.getNextValue();
            const float outG = outSmooth.getNextValue();
            const float dl = left[start + i], dr = right[start + i];
            left[start + i] = (dl + mix * (wl - dl)) * outG;
            if (numChannels > 1)
                right[start + i] = (dr + mix * (wr - dr)) * outG;
        }
    }

    // The switch-off fade has finished: leave the audio path and clear the
    // tail, so turning it back on starts clean.
    if (! on && ! mixSmooth.isSmoothing() && ! outSmooth.isSmoothing())
    {
        tailActive = false;
        clearTail();
    }
}

void ReverbEffect::clearTail()
{
    reverb.reset();
    for (auto& line : preDelay)
        std::fill (line.begin(), line.end(), 0.0f);
    for (auto* filters : { &lowCut, &highCut })
        for (auto& f : *filters)
            f.reset();
}
