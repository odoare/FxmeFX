/*
  ==============================================================================

    Cab.cpp

  ==============================================================================
*/

#include "Cab.h"
#include <resample.h>

Cab::Cab()
{
    updateGain();
}

Cab::~Cab() = default;

void Cab::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.Reset();

    const int maxChannels = 2;
    wdlInputBuffer.setSize (maxChannels, samplesPerBlock, false, true, true);
    wdlInputPtrs.resize (maxChannels);

    rebuildEngineImpulse();
}

void Cab::process (juce::AudioBuffer<float>& buffer)
{
    juce::ScopedLock sl (lock);
    checkParameters();

    if (! on)
        return;

    const int numSamples       = buffer.getNumSamples();
    const int numInputChannels = buffer.getNumChannels();
    const int numImpulseCh     = impulseBuffer.GetNumChannels();
    const int channelsToProcess = juce::jmin (numInputChannels, numImpulseCh);
    if (channelsToProcess == 0)
        return;

    if (wdlInputBuffer.getNumSamples() < numSamples)
        wdlInputBuffer.setSize (wdlInputBuffer.getNumChannels(), numSamples, false, true, true);

    for (int c = 0; c < channelsToProcess; ++c)
    {
        const auto* src = buffer.getReadPointer (c);
        auto*       dst = wdlInputBuffer.getWritePointer (c);
        for (int i = 0; i < numSamples; ++i)
            dst[i] = (WDL_FFT_REAL) src[i];
        wdlInputPtrs[(size_t) c] = dst;
    }

    engine.Add (wdlInputPtrs.data(), numSamples, channelsToProcess);

    const int avail  = engine.Avail (numSamples);
    WDL_FFT_REAL** out = engine.Get();
    const int toCopy = juce::jmin (avail, numSamples);

    for (int c = 0; c < channelsToProcess; ++c)
    {
        auto* dst = buffer.getWritePointer (c);
        const auto* wet = out[c];
        for (int i = 0; i < toCopy; ++i)
            dst[i] = (float) wet[i] * gainLinear;
        // If the engine doesn't yet have output for the trailing samples,
        // emit silence rather than the dry input — a cabinet emulation is
        // 100% wet and any dry leakage would defeat the modelling.
        for (int i = toCopy; i < numSamples; ++i)
            dst[i] = 0.0f;
    }

    // Clear any output channel beyond what we processed (e.g. stereo bus
    // with a mono IR slot configuration).
    for (int c = channelsToProcess; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    engine.Advance (toCopy);
}

void Cab::setImpulseList (const juce::StringArray& names, const juce::StringArray& resources)
{
    juce::ScopedLock sl (lock);
    irNames = names;
    irResources = resources;

    // Auto-pick something sensible the first time the list arrives so both
    // slots are audible by default (left = first IR, right = second if it
    // exists, otherwise also the first).
    if (! irResources.isEmpty())
    {
        if (currentIndex[0] < 0) selectImpulse (0, 0);
        if (currentIndex[1] < 0) selectImpulse (1, juce::jmin (1, irResources.size() - 1));
    }
}

void Cab::selectImpulse (int channel, int index)
{
    if (channel < 0 || channel >= NumSlots)
        return;
    if (index < 0 || index >= irResources.size())
        return;

    if (currentIndex[(size_t) channel] != index)
    {
        currentIndex[(size_t) channel] = index;
        loadResource (channel, irResources[index]);
        rebuildEngineImpulse();
    }
}

void Cab::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

juce::AudioBuffer<float> Cab::getIR (int channel) const
{
    juce::ScopedLock sl (lock);
    if (channel < 0 || channel >= NumSlots)
        return {};
    return monoIR[(size_t) channel];
}

void Cab::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam   = apvts.getRawParameterValue (prefix + "_Cab_On");
    irLParam  = apvts.getRawParameterValue (prefix + "_Cab_IRL");
    irRParam  = apvts.getRawParameterValue (prefix + "_Cab_IRR");
    gainParam = apvts.getRawParameterValue (prefix + "_Cab_Gain");
}

void Cab::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                             const juce::String& prefix,
                             int numIRs)
{
    const int maxIR = juce::jmax (1, numIRs);

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Cab_On",   1 }, prefix + " Cab On", true));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "_Cab_IRL",  1 }, prefix + " Cab IR L", 1, maxIR, 1));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "_Cab_IRR",  1 }, prefix + " Cab IR R", 1, maxIR, juce::jmin (2, maxIR)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Cab_Gain", 1 }, prefix + " Cab Gain", -24.0f, 24.0f, 0.0f));
}

void Cab::loadResource (int channel, const juce::String& resourceName)
{
    int dataSize = 0;
    const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

    if (data == nullptr)
    {
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
            {
                data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize);
                break;
            }
        }
    }

    if (data == nullptr)
    {
        monoIR[(size_t) channel].clear();
        return;
    }

    auto* stream = new juce::MemoryInputStream (data, (size_t) dataSize, false);
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatReader> reader (wavFormat.createReaderFor (stream, true));

    if (reader != nullptr)
        loadIRFromReader (channel, *reader);
    else
        monoIR[(size_t) channel].clear();
}

void Cab::loadIRFromReader (int channel, juce::AudioFormatReader& reader)
{
    auto& dest = monoIR[(size_t) channel];

    const int  inSamples       = (int) reader.lengthInSamples;
    const int  inChannels      = (int) reader.numChannels;
    const bool needsResample   = currentSampleRate > 0
                              && reader.sampleRate > 0
                              && std::abs (reader.sampleRate - currentSampleRate) > 1.0;

    // Read into a temporary then collapse to mono (sum-to-mono of any extra
    // channels is more faithful than just dropping them).
    juce::AudioBuffer<float> temp (inChannels, inSamples);
    reader.read (&temp, 0, inSamples, 0, true, true);

    juce::AudioBuffer<float> mono (1, inSamples);
    mono.clear();
    for (int c = 0; c < inChannels; ++c)
        mono.addFrom (0, 0, temp, c, 0, inSamples, 1.0f / (float) juce::jmax (1, inChannels));

    if (! needsResample)
    {
        dest = std::move (mono);
        return;
    }

    WDL_Resampler resampler;
    resampler.SetMode (true, 0, true);
    resampler.SetRates (reader.sampleRate, currentSampleRate);
    resampler.SetFeedMode (true);

    WDL_ResampleSample* wdlIn = nullptr;
    resampler.ResamplePrepare (inSamples, 1, &wdlIn);
    for (int i = 0; i < inSamples; ++i)
        wdlIn[i] = (WDL_ResampleSample) mono.getSample (0, i);

    const int maxOut = (int) (inSamples * currentSampleRate / reader.sampleRate) + 1024;
    std::vector<WDL_ResampleSample> wdlOut ((size_t) maxOut);
    const int outSamples = resampler.ResampleOut (wdlOut.data(), inSamples, maxOut, 1);

    dest.setSize (1, outSamples);
    for (int i = 0; i < outSamples; ++i)
        dest.setSample (0, i, (float) wdlOut[(size_t) i]);
}

void Cab::rebuildEngineImpulse()
{
    juce::ScopedLock sl (lock);

    const int lenL = monoIR[0].getNumSamples();
    const int lenR = monoIR[1].getNumSamples();
    const int len  = juce::jmax (lenL, lenR);

    if (len <= 0)
    {
        // Engine refuses zero-length impulses; load a single silent sample
        // so process() can still run safely while no IR is selected.
        impulseBuffer.SetNumChannels (1);
        impulseBuffer.SetLength (1);
        impulseBuffer.samplerate = currentSampleRate;
        impulseBuffer.impulses[0].Get()[0] = 0.0;
        engine.SetImpulse (&impulseBuffer);
        return;
    }

    impulseBuffer.SetNumChannels (NumSlots);
    impulseBuffer.SetLength (len);
    impulseBuffer.samplerate = currentSampleRate;

    for (int c = 0; c < NumSlots; ++c)
    {
        auto* dst = impulseBuffer.impulses[c].Get();
        const auto* src = monoIR[(size_t) c].getReadPointer (0);
        const int n = monoIR[(size_t) c].getNumSamples();
        for (int i = 0; i < n; ++i)
            dst[i] = (WDL_FFT_REAL) src[i];
        for (int i = n; i < len; ++i)
            dst[i] = 0.0;
    }

    engine.SetImpulse (&impulseBuffer);
}

void Cab::checkParameters()
{
    if (onParam && *onParam != lastOn)
    {
        setOn (*onParam > 0.5f);
        lastOn = *onParam;
    }

    if (irLParam && (int) *irLParam != lastIRL)
    {
        const int idx = (int) *irLParam - 1; // params are 1-based
        selectImpulse (0, idx);
        lastIRL = (int) *irLParam;
    }

    if (irRParam && (int) *irRParam != lastIRR)
    {
        const int idx = (int) *irRParam - 1;
        selectImpulse (1, idx);
        lastIRR = (int) *irRParam;
    }

    if (gainParam && *gainParam != lastGain)
    {
        gaindB = *gainParam;
        lastGain = gaindB;
        updateGain();
    }
}

void Cab::updateGain()
{
    gainLinear = juce::Decibels::decibelsToGain (gaindB);
}
