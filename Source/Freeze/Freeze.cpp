/*
  ==============================================================================

    Freeze.cpp

  ==============================================================================
*/

#include "Freeze.h"

namespace
{
    // Identity of the deterministic phase streams: fixed, so a given capture
    // always reproduces the same wash. The low 8 bits of the tag stay clear —
    // SpectralFreezeMulti fills them with the channel index.
    constexpr uint64_t kSeed = 0x46584d4546525aull;   // "FXMEFRZ"
    constexpr uint64_t kTag  = 0x467265657a6500ull;   // "Freeze" << 8
}

void Freeze::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;

    freezer.prepare (numChannels);
    freezer.setIdentity (kSeed, kTag);
    freezer.setMix (1.0f);   // wet/dry is applied here, smoothed per sample

    dryBuffer.setSize (juce::jmax (1, numChannels), juce::jmax (1, samplesPerBlock));

    history.setSize (juce::jmax (1, numChannels), fxme::SpectralFreeze::kSize);
    history.clear();
    historyScratch.setSize (history.getNumChannels(), fxme::SpectralFreeze::kSize);
    histPos = 0;
    pendingCapture = false;

    mixSmooth.reset (sampleRate, 0.05);
    mixSmooth.setCurrentAndTargetValue (engaged.load() ? mixValue : 0.0f);

    displayRing.assign ((size_t) kDisplayFifoSize, 0.0f);
    displayFifo.reset();

    washActive = engaged.load();
    if (washActive)
        freezer.startCapture();
}

void Freeze::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam    = apvts.getRawParameterValue (prefix + "_Freeze_On");
    mixParam   = apvts.getRawParameterValue (prefix + "_Freeze_Mix");
    widthParam = apvts.getRawParameterValue (prefix + "_Freeze_Width");
}

void Freeze::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                            const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Freeze_On", 1 }, prefix + " Freeze On", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Freeze_Mix", 1 }, prefix + " Freeze Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    // Stereo width of the wash: 100 % leaves the decorrelated channels
    // untouched, 0 % collapses them to the identical mono blend
    // (equal-power L' = a·L + b·R inside SpectralFreezeMulti).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Freeze_Width", 1 }, prefix + " Freeze Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
}

void Freeze::checkParameters()
{
    if (mixParam && *mixParam != lastMix)
    {
        mixValue = mixParam->load() * 0.01f;
        lastMix  = *mixParam;
        if (engaged.load())
            mixSmooth.setTargetValue (mixValue);
    }

    if (widthParam && *widthParam != lastWidth)
    {
        freezer.setWidth (widthParam->load() * 0.01f);
        lastWidth = *widthParam;
    }

    if (onParam && *onParam != lastOn)
    {
        const bool on = *onParam > 0.5f;
        if (on && ! engaged.load())
        {
            // Freeze the history (the audio *before* the press) at the top
            // of the next process() call. The mix can jump straight to its
            // target: the freezer's own capture->wash crossfade starts from
            // the dry signal, so there is nothing to click.
            pendingCapture = true;
            washActive     = true;
            mixSmooth.setCurrentAndTargetValue (mixValue);
        }
        else
        {
            mixSmooth.setTargetValue (on ? mixValue : 0.0f);
        }
        engaged.store (on);
        lastOn = *onParam;
    }
}

void Freeze::process (juce::AudioBuffer<float>& buffer)
{
    pushDisplaySamples (buffer);

    // Freeze the pre-press window first, then let this block's input roll
    // into the history — the capture must not include the current block.
    if (pendingCapture)
    {
        captureFromHistory();
        pendingCapture = false;
    }
    pushHistory (buffer);

    if (! washActive)
        return;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels());

    // The wash faded out completely after switch-off: stop the synthesis.
    if (! engaged.load() && ! mixSmooth.isSmoothing() && mixSmooth.getCurrentValue() <= 0.0f)
    {
        freezer.reset();
        washActive = false;
        return;
    }

    if (dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (dryBuffer.getNumChannels(), numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    freezer.process (buffer.getArrayOfWritePointers(), numChannels, numSamples);

    // Smoothed wet/dry: out = dry + m * (wet - dry). While capturing (and
    // when idle) the wash equals the input, so engaging is click-free.
    for (int i = 0; i < numSamples; ++i)
    {
        const float m = mixSmooth.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* wet       = buffer.getWritePointer (ch);
            const auto* dry = dryBuffer.getReadPointer (ch);
            wet[i] = dry[i] + m * (wet[i] - dry[i]);
        }
    }
}

void Freeze::pushHistory (const juce::AudioBuffer<float>& buffer)
{
    constexpr int kSize = fxme::SpectralFreeze::kSize;
    const int numChannels = juce::jmin (buffer.getNumChannels(), history.getNumChannels());

    // Only the last kSize samples of a (huge) block can matter.
    int numSamples = buffer.getNumSamples();
    int srcOffset  = 0;
    if (numSamples > kSize)
    {
        srcOffset  = numSamples - kSize;
        numSamples = kSize;
    }

    const int firstRun = juce::jmin (kSize - histPos, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        history.copyFrom (ch, histPos, buffer, ch, srcOffset, firstRun);
        if (numSamples > firstRun)
            history.copyFrom (ch, 0, buffer, ch, srcOffset + firstRun, numSamples - firstRun);
    }
    histPos = (histPos + numSamples) % kSize;
}

void Freeze::captureFromHistory()
{
    constexpr int kSize = fxme::SpectralFreeze::kSize;
    const int numChannels = history.getNumChannels();

    // Unroll the ring (oldest sample first) and replay it through the
    // freezer's capture in one go: the spectrum is computed and the wash
    // primed right here, so the freeze is audible from this block on. The
    // scratch output (the width-blended pass-through) is discarded.
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dst       = historyScratch.getWritePointer (ch);
        const auto* src = history.getReadPointer (ch);
        juce::FloatVectorOperations::copy (dst, src + histPos, kSize - histPos);
        juce::FloatVectorOperations::copy (dst + (kSize - histPos), src, histPos);
    }

    freezer.startCapture();
    freezer.process (historyScratch.getArrayOfWritePointers(), numChannels, kSize);
}

void Freeze::pushDisplaySamples (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmax (1, buffer.getNumChannels());
    const float chGain    = 1.0f / (float) numChannels;

    int start1, size1, start2, size2;
    displayFifo.prepareToWrite (numSamples, start1, size1, start2, size2);

    auto writeRun = [&] (int start, int size, int offset)
    {
        for (int i = 0; i < size; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                sum += buffer.getReadPointer (ch)[offset + i];
            displayRing[(size_t) (start + i)] = sum * chGain;
        }
    };

    writeRun (start1, size1, 0);
    writeRun (start2, size2, size1);
    displayFifo.finishedWrite (size1 + size2);
}

int Freeze::pullDisplaySamples (float* dest, int maxSamples)
{
    const int ready = juce::jmin (displayFifo.getNumReady(), maxSamples);
    if (ready <= 0)
        return 0;

    int start1, size1, start2, size2;
    displayFifo.prepareToRead (ready, start1, size1, start2, size2);

    std::copy_n (displayRing.data() + start1, size1, dest);
    std::copy_n (displayRing.data() + start2, size2, dest + size1);

    displayFifo.finishedRead (size1 + size2);
    return size1 + size2;
}
