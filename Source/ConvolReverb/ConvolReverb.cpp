/*
  ==============================================================================

    ConvolReverb.cpp

  ==============================================================================
*/

#include "ConvolReverb.h"
#include "../../WDL/WDL/resample.h"

ConvolReverb::ConvolReverb()
{
    updateGains();
}

ConvolReverb::~ConvolReverb()
{
}

void ConvolReverb::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.Reset();
    const int maxChannels = 4; // Pre-allocate for a maximum number of channels
    wdlInputBuffer.setSize(maxChannels, samplesPerBlock, false, true, true);
    wdlInputPtrs.resize(maxChannels);
}

void ConvolReverb::process (juce::AudioBuffer<float>& buffer)
{
    juce::ScopedLock sl (lock);

    checkParameters(); // Update parameters before processing
    
    if (!on) return;

    int numSamples = buffer.getNumSamples();
    int numInputChannels = buffer.getNumChannels();
    int numImpulseChannels = impulseBuffer.GetNumChannels();
    int channelsToProcess = juce::jmin(numInputChannels, numImpulseChannels);

    if (channelsToProcess == 0)
        return;

    if (wdlInputBuffer.getNumSamples() < numSamples)
        wdlInputBuffer.setSize(wdlInputBuffer.getNumChannels(), numSamples, false, true, true);

    for (int c = 0; c < channelsToProcess; ++c)
    {
        auto* src = buffer.getReadPointer (c);
        auto* dst = wdlInputBuffer.getWritePointer(c);
        for (int i = 0; i < numSamples; ++i)
            dst[i] = (WDL_FFT_REAL) src[i];
        wdlInputPtrs[c] = dst;
    }

    engine.Add (wdlInputPtrs.data(), numSamples, channelsToProcess);

    int avail = engine.Avail (numSamples);
    WDL_FFT_REAL** out = engine.Get();
    int toCopy = juce::jmin (avail, numSamples);

    for (int c = 0; c < channelsToProcess; ++c)
    {
        auto* dst = buffer.getWritePointer (c);
        
        // If the engine has output for this channel
        auto* src = out[c];
        for (int i = 0; i < toCopy; ++i)
        {
            float dry = dst[i];
            float wet = (float) src[i];
            dst[i] = dry * dryGainLinear + wet * wetGainLinear;
        }

        if (toCopy < numSamples)
        {
            for (int i = toCopy; i < numSamples; ++i)
                dst[i] = dst[i] * dryGainLinear;
        }
    }

    engine.Advance (toCopy);
}

void ConvolReverb::setImpulseList (const juce::StringArray& names, const juce::StringArray& resources)
{
    irNames = names;
    irResources = resources;
    if (!irResources.isEmpty())
        selectImpulse (0); // Select the first one by default
}

void ConvolReverb::selectImpulse (int index)
{
    const int extIdx = getExternalIndex();
    if (index < 0 || index > extIdx)
        return;

    if (currentIndex != index)
    {
        currentIndex = index;
        if (index == extIdx)
            loadExternalIR();
        else
            loadResource (irResources[currentIndex]);
        updateModifiedIR();
    }
}

void ConvolReverb::setLengthRatio (float ratio)
{
    if (currentLengthRatio != ratio)
    {
        currentLengthRatio = ratio;
        updateModifiedIR();
    }
}

void ConvolReverb::setShapeType (int type)
{
    if (currentShapeType != type)
    {
        currentShapeType = type;
        updateModifiedIR();
    }
}

void ConvolReverb::setStartOffset (float offsetMs)
{
    if (currentStartOffsetMs != offsetMs)
    {
        currentStartOffsetMs = offsetMs;
        updateModifiedIR();
    }
}

void ConvolReverb::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

void ConvolReverb::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    irParam = apvts.getRawParameterValue (prefix + "_Rev_IR");
    lengthParam = apvts.getRawParameterValue (prefix + "_Rev_Length");
    shapeParam = apvts.getRawParameterValue (prefix + "_Rev_Shape");
    startOffsetParam = apvts.getRawParameterValue (prefix + "_Rev_StartOffset");
    dryGainParam = apvts.getRawParameterValue (prefix + "_Rev_DryGain");
    wetGainParam = apvts.getRawParameterValue (prefix + "_Rev_WetGain");
    onParam = apvts.getRawParameterValue (prefix + "_Rev_On");

    apvtsRef = &apvts;
    extPathId = externalPathPropertyId (prefix);
    externalPath = apvts.state.getProperty (extPathId, "").toString();
}

void ConvolReverb::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix, int numIRs)
{
    // IR selection: built-ins occupy 1..numIRs and the External slot is at
    // numIRs + 1, so the param max is numIRs + 1.
    int builtinMax = (numIRs > 0) ? numIRs : 1;
    int maxVal = builtinMax + 1; // +1 for the External slot
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { prefix + "_Rev_IR", 1 }, prefix + " Rev IR", 1, maxVal, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Rev_Length", 1 }, prefix + " Rev Length", 0.0f, 1.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { prefix + "_Rev_On", 1 }, prefix + " Rev On", true));
    
    juce::StringArray shapes;
    shapes.add ("Fast Exp");
    shapes.add ("Linear");
    shapes.add ("Slow Log");
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { prefix + "_Rev_Shape", 1 }, prefix + " Rev Shape", shapes, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Rev_StartOffset", 1 }, prefix + " Rev Start Offset", -100.0f, 100.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Rev_DryGain", 1 }, prefix + " Rev Dry Gain", -60.0f, 6.0f, -60.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Rev_WetGain", 1 }, prefix + " Rev Wet Gain", -60.0f, 6.0f, 0.0f));
}

void ConvolReverb::loadResource (const juce::String& resourceName)
{
    int dataSize = 0;
    const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);
    
    if (data == nullptr)
    {
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                { data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize); break; }
        }
    }

    if (data != nullptr)
    {
        auto* stream = new juce::MemoryInputStream (data, (size_t)dataSize, false);
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatReader> reader (wavFormat.createReaderFor (stream, true));

        if (reader)
            loadIRFromReader (*reader);
        else
            originalIR.clear();
    }
    else
    {
        originalIR.clear(); // Clear if resource not found
    }
}

void ConvolReverb::loadIRFromReader (juce::AudioFormatReader& reader)
{
    if (currentSampleRate > 0 && reader.sampleRate > 0 && std::abs (reader.sampleRate - currentSampleRate) > 1.0)
    {
        WDL_Resampler resampler;
        resampler.SetMode (true, 0, true); // Sinc interpolation
        resampler.SetRates (reader.sampleRate, currentSampleRate);
        resampler.SetFeedMode (true);

        int numChannels = (int) reader.numChannels;
        int numInputSamples = (int) reader.lengthInSamples;

        juce::AudioBuffer<float> tempBuffer (numChannels, numInputSamples);
        reader.read (&tempBuffer, 0, numInputSamples, 0, true, true);

        WDL_ResampleSample* wdlIn = nullptr;
        resampler.ResamplePrepare (numInputSamples, numChannels, &wdlIn);

        for (int i = 0; i < numInputSamples; ++i)
            for (int c = 0; c < numChannels; ++c)
                wdlIn[i * numChannels + c] = (WDL_ResampleSample) tempBuffer.getSample (c, i);

        int maxOutSamples = (int) (numInputSamples * currentSampleRate / reader.sampleRate) + 1024;
        std::vector<WDL_ResampleSample> wdlOut (maxOutSamples * numChannels);

        int outSamples = resampler.ResampleOut (wdlOut.data(), numInputSamples, maxOutSamples, numChannels);

        originalIR.setSize (numChannels, outSamples);
        for (int i = 0; i < outSamples; ++i)
            for (int c = 0; c < numChannels; ++c)
                originalIR.setSample (c, i, (float) wdlOut[i * numChannels + c]);
    }
    else
    {
        originalIR.setSize ((int) reader.numChannels, (int) reader.lengthInSamples);
        reader.read (&originalIR, 0, (int) reader.lengthInSamples, 0, true, true);
    }
}

void ConvolReverb::loadExternalIR()
{
    // Refresh path from APVTS state in case it was restored after assignParameters.
    if (apvtsRef != nullptr)
        externalPath = apvtsRef->state.getProperty (extPathId, externalPath).toString();

    juce::File f (externalPath);
    if (externalPath.isEmpty() || ! f.existsAsFile())
    {
        // Fall back silently to the first built-in IR so the wet path keeps
        // producing audio when the saved file is missing.
        if (! irResources.isEmpty())
            loadResource (irResources[0]);
        else
            originalIR.clear();
        return;
    }

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));

    if (reader != nullptr)
        loadIRFromReader (*reader);
    else if (! irResources.isEmpty())
        loadResource (irResources[0]);
    else
        originalIR.clear();
}

void ConvolReverb::setExternalIRPath (const juce::String& path)
{
    if (apvtsRef != nullptr)
        apvtsRef->state.setProperty (extPathId, path, nullptr);

    juce::ScopedLock sl (lock);
    externalPath = path;
    if (currentIndex == getExternalIndex())
    {
        loadExternalIR();
        updateModifiedIR();
    }
}

juce::String ConvolReverb::getExternalIRPath() const
{
    juce::ScopedLock sl (lock);
    if (apvtsRef != nullptr)
        return apvtsRef->state.getProperty (extPathId, externalPath).toString();
    return externalPath;
}

juce::AudioBuffer<float> ConvolReverb::getModifiedIR() const
{
    juce::ScopedLock sl (lock);
    return modifiedIR;
}

void ConvolReverb::updateModifiedIR()
{
    juce::ScopedLock sl (lock);
    if (originalIR.getNumSamples() == 0)
    {
        modifiedIR.clear();
        loadImpulseToEngine (modifiedIR); // Load empty IR
        return;
    }

    // Create a temporary buffer with length and shape applied
    int newLength = (int)(originalIR.getNumSamples() * currentLengthRatio);
    if (newLength < 16) newLength = 16; // Minimum safety length
    if (newLength > originalIR.getNumSamples()) newLength = originalIR.getNumSamples();

    juce::AudioBuffer<float> shapedIR;
    shapedIR.makeCopyOf(originalIR);
    shapedIR.setSize(shapedIR.getNumChannels(), newLength, true, true);

    // Apply Envelope
    for (int ch = 0; ch < shapedIR.getNumChannels(); ++ch)
    {
        auto* data = shapedIR.getWritePointer (ch);
        for (int i = 0; i < newLength; ++i)
        {
            float normPos = (float)i / (float)newLength;
            float gain = 1.0f;

            if (currentShapeType == 0) // Fast Exp
                gain = std::pow (1.0f - normPos, 2.0f);
            else if (currentShapeType == 1) // Linear
                gain = 1.0f - normPos;
            else if (currentShapeType == 2) // Slow Log
                gain = std::sqrt (1.0f - normPos); // Simple approximation

            data[i] *= gain;
        }
        
        // Fade out last few samples to avoid clicks
        int fadeLen = juce::jmin(100, newLength);
        if (newLength > fadeLen)
            shapedIR.applyGainRamp (ch, newLength - fadeLen, fadeLen, 1.0f, 0.0f);
    }

    // Now apply offset
    int offsetInSamples = (int)(currentStartOffsetMs * currentSampleRate / 1000.0);

    if (offsetInSamples > 0)
    {
        // Positive offset: add silence at the start
        modifiedIR.setSize(shapedIR.getNumChannels(), shapedIR.getNumSamples() + offsetInSamples, false, true, true);
        modifiedIR.clear();
        for (int ch = 0; ch < shapedIR.getNumChannels(); ++ch)
        {
            modifiedIR.copyFrom(ch, offsetInSamples, shapedIR, ch, 0, shapedIR.getNumSamples());
        }
    }
    else if (offsetInSamples < 0)
    {
        // Negative offset: trim samples from the start
        int trimSamples = -offsetInSamples;
        if (trimSamples >= shapedIR.getNumSamples())
        {
            modifiedIR.clear();
        }
        else
        {
            const int numSamplesToCopy = shapedIR.getNumSamples() - trimSamples;
            modifiedIR.setSize (shapedIR.getNumChannels(), numSamplesToCopy, false, true, true);

            for (int ch = 0; ch < shapedIR.getNumChannels(); ++ch)
            {
                modifiedIR.copyFrom (ch, 0, shapedIR.getReadPointer (ch, trimSamples), numSamplesToCopy);
            }
        }
    }
    else // offsetInSamples == 0
    {
        modifiedIR.makeCopyOf(shapedIR);
    }

    loadImpulseToEngine(modifiedIR);
}

void ConvolReverb::loadImpulseToEngine (const juce::AudioBuffer<float>& buffer)
{
    juce::ScopedLock sl (lock);

    int nch = buffer.getNumChannels();
    int len = buffer.getNumSamples();

    // Safety check: WDL_ImpulseBuffer crashes if initialized with 0 channels/length
    if (nch == 0 || len == 0)
    {
        // Load a dummy silent impulse
        nch = 1;
        len = 1;
        impulseBuffer.SetNumChannels (nch);
        impulseBuffer.SetLength (len);
        impulseBuffer.samplerate = currentSampleRate;
        auto* dest = impulseBuffer.impulses[0].Get();
        dest[0] = 0.0;
    }
    else
    {
        // Promote a mono impulse to stereo by duplicating it onto both engine
        // channels; otherwise process() only convolves the left side of a
        // stereo input.
        const int engineChannels = (nch == 1) ? 2 : nch;
        impulseBuffer.SetNumChannels (engineChannels);
        impulseBuffer.SetLength (len);
        impulseBuffer.samplerate = currentSampleRate;

        for (int c = 0; c < engineChannels; ++c)
        {
            auto* dest = impulseBuffer.impulses[c].Get();
            auto* src = buffer.getReadPointer (juce::jmin (c, nch - 1));
            for (int i = 0; i < len; ++i)
                dest[i] = (WDL_FFT_REAL) src[i];
        }
    }

    engine.SetImpulse (&impulseBuffer);
}

void ConvolReverb::checkParameters()
{
    if (irParam && (int)*irParam != lastIR)
    {
        // Parameter is 1-based (to match ComboBox IDs), selectImpulse is 0-based.
        // Index irResources.size() is the External slot.
        int val = (int)*irParam;
        int index = val - 1;

        if (index >= 0 && index <= (int) irResources.size())
            selectImpulse (index);

        lastIR = val;
    }

    if (lengthParam && *lengthParam != lastLengthRatio)
    {
        setLengthRatio (*lengthParam);
        lastLengthRatio = *lengthParam;
    }

    if (shapeParam && (int)*shapeParam != lastShapeType)
    {
        setShapeType ((int)*shapeParam);
        lastShapeType = (int)*shapeParam;
    }

    if (startOffsetParam && *startOffsetParam != lastStartOffset)
    {
        setStartOffset(*startOffsetParam);
        lastStartOffset = *startOffsetParam;
    }

    if (dryGainParam && *dryGainParam != lastDryGain)
    {
        dryGain = *dryGainParam;
        lastDryGain = dryGain;
        updateGains();
    }
    if (wetGainParam && *wetGainParam != lastWetGain)
    {
        wetGain = *wetGainParam;
        lastWetGain = wetGain;
        updateGains();
    }

    if (onParam && *onParam != lastOn)
    {
        setOn (*onParam > 0.5f);
        lastOn = *onParam;
    }
}

void ConvolReverb::updateGains()
{
    dryGainLinear = juce::Decibels::decibelsToGain(dryGain);
    wetGainLinear = juce::Decibels::decibelsToGain(wetGain);
}
