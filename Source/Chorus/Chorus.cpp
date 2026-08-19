/*
  ==============================================================================

    Chorus.cpp

  ==============================================================================
*/

#include "Chorus.h"
#include <FxmeTools/dsp/Lfo.h>

namespace
{
    // Ramp for everything a user can move while audio runs. Long enough that a
    // swept Delay knob glides like tape rather than stepping, short enough that
    // the Mix knob still feels immediate.
    constexpr double kSmoothingSeconds = 0.05;
}

void Chorus::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused (samplesPerBlock, numChannels);

    currentSampleRate = sampleRate;

    // Widest tap the parameters can ask for, plus headroom for the
    // interpolator's guard samples.
    const float maxSeconds = (kMaxDelayMs + kMaxSwingMs) * 0.001f + 0.01f;
    for (auto& line : lines)
        line.prepare (sampleRate, maxSeconds);

    lfo.prepare (sampleRate);
    lfo.setBpm (currentBPM);

    mixSmooth   .reset (sampleRate, kSmoothingSeconds);
    outSmooth   .reset (sampleRate, kSmoothingSeconds);
    centreSmooth.reset (sampleRate, kSmoothingSeconds);
    swingSmooth .reset (sampleRate, kSmoothingSeconds);

    mixSmooth.setCurrentAndTargetValue (on ? mix01 : 0.0f);
    if (! on)
        outSmooth.setCurrentAndTargetValue (1.0f);

    updateDelayTargets();
    centreSmooth.setCurrentAndTargetValue (centreSmooth.getTargetValue());
    swingSmooth .setCurrentAndTargetValue (swingSmooth.getTargetValue());

    linesActive = true;
}

void Chorus::updateDelayTargets()
{
    const float sr = (float) currentSampleRate;

    // The swing never eats into the first 0.5 ms of the line, so the taps stay
    // in the interpolator's valid range whatever the knobs say.
    const float swingMs = depth01 * juce::jmin (delayMs - 0.5f, kMaxSwingMs);

    centreSmooth.setTargetValue (delayMs * 0.001f * sr);
    swingSmooth .setTargetValue (juce::jmax (0.0f, swingMs) * 0.001f * sr);
}

void Chorus::setBPM (double bpm)
{
    if (currentBPM != bpm)
    {
        currentBPM = bpm;
        lfo.setBpm (bpm);
    }
}

void Chorus::setTransport (double bpm, double ppqPosition, bool isPlaying)
{
    setBPM (bpm);
    hostPpq     = ppqPosition;
    hostPlaying = isPlaying;
}

void Chorus::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                            const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Chorus_On", 1 }, prefix + " Chorus On", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Chorus_Rate", 1 }, prefix + " Chorus Rate",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.001f, 0.3f), 0.6f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Chorus_Sync", 1 }, prefix + " Chorus Sync", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Chorus_Div", 1 }, prefix + " Chorus Division",
        juce::StringArray (fxme::Lfo::syncDivisionNames, fxme::Lfo::numSyncDivisions), fxme::Lfo::defaultSyncDivision));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_Chorus_Shape", 1 }, prefix + " Chorus Shape",
        juce::StringArray (fxme::Lfo::shapeNames, fxme::Lfo::numShapes), fxme::Lfo::sine));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Chorus_Depth", 1 }, prefix + " Chorus Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Chorus_Delay", 1 }, prefix + " Chorus Delay",
        juce::NormalisableRange<float> (1.0f, kMaxDelayMs, 0.01f, 0.47f), 12.0f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "_Chorus_Voices", 1 }, prefix + " Chorus Voices",
        2, maxVoices, 2));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Chorus_Width", 1 }, prefix + " Chorus Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Chorus_Mix", 1 }, prefix + " Chorus Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Chorus_Out", 1 }, prefix + " Chorus Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
}

void Chorus::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam     = apvts.getRawParameterValue (prefix + "_Chorus_On");
    rateParam   = apvts.getRawParameterValue (prefix + "_Chorus_Rate");
    syncParam   = apvts.getRawParameterValue (prefix + "_Chorus_Sync");
    divParam    = apvts.getRawParameterValue (prefix + "_Chorus_Div");
    shapeParam  = apvts.getRawParameterValue (prefix + "_Chorus_Shape");
    depthParam  = apvts.getRawParameterValue (prefix + "_Chorus_Depth");
    delayParam  = apvts.getRawParameterValue (prefix + "_Chorus_Delay");
    voicesParam = apvts.getRawParameterValue (prefix + "_Chorus_Voices");
    widthParam  = apvts.getRawParameterValue (prefix + "_Chorus_Width");
    mixParam    = apvts.getRawParameterValue (prefix + "_Chorus_Mix");
    outParam    = apvts.getRawParameterValue (prefix + "_Chorus_Out");
}

void Chorus::checkParameters()
{
    if (rateParam && *rateParam != lastRate)
    {
        lfo.setFrequency (*rateParam);
        lastRate = *rateParam;
    }
    if (syncParam && *syncParam != lastSync)
    {
        lfo.setSynced (*syncParam > 0.5f);
        lastSync = *syncParam;
    }
    if (divParam && *divParam != lastDiv)
    {
        lfo.setSyncBeats (fxme::Lfo::syncDivisionBeats ((int) *divParam));
        lastDiv = *divParam;
    }
    if (shapeParam && *shapeParam != lastShape)
    {
        lfo.setShape ((int) *shapeParam);
        lastShape = *shapeParam;
    }
    if (depthParam && *depthParam != lastDepth)
    {
        depth01   = *depthParam * 0.01f;
        lastDepth = *depthParam;
        updateDelayTargets();
    }
    if (delayParam && *delayParam != lastDelay)
    {
        delayMs   = *delayParam;
        lastDelay = *delayParam;
        updateDelayTargets();
    }
    if (voicesParam && *voicesParam != lastVoices)
    {
        voices     = juce::jlimit (1, maxVoices, (int) *voicesParam);
        lastVoices = *voicesParam;
    }
    if (widthParam && *widthParam != lastWidth)
    {
        widthOffset = *widthParam * 0.01f * 0.5f;
        lastWidth   = *widthParam;
    }
    if (mixParam && *mixParam != lastMix)
    {
        mix01   = *mixParam * 0.01f;
        lastMix = *mixParam;
        if (on)
            mixSmooth.setTargetValue (mix01);
    }
    if (outParam && *outParam != lastOut)
    {
        lastOut = *outParam;
        if (on)
            outSmooth.setTargetValue (juce::Decibels::decibelsToGain (outParam->load()));
    }
    if (onParam && *onParam != lastOn)
    {
        on     = *onParam > 0.5f;
        lastOn = *onParam;

        // Switching is a fade between the wet blend and the untouched dry
        // signal, so the button never clicks. The lines keep running until
        // the fade is over (see process()).
        mixSmooth.setTargetValue (on ? mix01 : 0.0f);
        outSmooth.setTargetValue (on ? juce::Decibels::decibelsToGain (outParam != nullptr ? outParam->load() : 0.0f)
                                     : 1.0f);
        if (on)
            linesActive = true;
    }
}

void Chorus::process (juce::AudioBuffer<float>& buffer)
{
    if (! on && ! linesActive)
        return;

    // A synced sweep is re-anchored to the timeline once per block; free-running
    // (or a stopped transport) just keeps its own phase.
    if (lfo.isSynced() && hostPlaying)
        lfo.setPhaseFromPpq (hostPpq);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), (int) lines.size());
    if (numChannels <= 0)
        return;

    const float voiceGain = 1.0f / (float) voices;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix    = mixSmooth.getNextValue();
        const float outG   = outSmooth.getNextValue();
        const float centre = centreSmooth.getNextValue();
        const float swing  = swingSmooth.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float chOffset = (ch == 0 ? 0.0f : widthOffset);

            float wet = 0.0f;
            for (int v = 0; v < voices; ++v)
            {
                const float phaseOffset = chOffset + (float) v / (float) voices;
                wet += lines[(size_t) ch].read (centre + swing * lfo.valueAt (phaseOffset));
            }
            wet *= voiceGain;

            auto* data    = buffer.getWritePointer (ch);
            const float x = data[i];
            lines[(size_t) ch].write (x);
            data[i] = (x + mix * (wet - x)) * outG;
        }

        lfo.advance();
    }

    // The switch-off fade has finished: drop out of the audio path entirely and
    // clear the lines, so re-engaging never replays a stale tail.
    if (! on && ! mixSmooth.isSmoothing() && ! outSmooth.isSmoothing()
        && mixSmooth.getCurrentValue() <= 0.0f)
    {
        for (auto& line : lines)
            line.reset();
        linesActive = false;
    }
}
