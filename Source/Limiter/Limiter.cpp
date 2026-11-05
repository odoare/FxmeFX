/*
  ==============================================================================

    Limiter.cpp

  ==============================================================================
*/

#include "Limiter.h"

Limiter::Limiter()
{
    updateCoefficients();

    // The true-peak interpolator: a Kaiser-windowed sinc (beta 7) cut at the
    // original Nyquist, 49 taps at the 4x rate, each phase normalised to unity
    // at DC. Phase 0 is then the original sample itself.
    constexpr int length = tpPhases * (tpTaps - 1) + 1;   // 49
    constexpr double centre = (length - 1) / 2.0;          // 24
    constexpr double beta = 7.0;
    auto besselI0 = [] (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 30; ++k)
        {
            term *= (x / (2.0 * k)) * (x / (2.0 * k));
            sum += term;
        }
        return sum;
    };
    for (int k = 0; k < tpPhases; ++k)
    {
        double sum = 0.0;
        for (int j = 0; j < tpTaps; ++j)
        {
            const int i = tpPhases * j + k;
            double h = 0.0;
            if (i < length)
            {
                const double t = (i - centre) / tpPhases;
                const double sinc = t == 0.0 ? 1.0 : std::sin (juce::MathConstants<double>::pi * t) / (juce::MathConstants<double>::pi * t);
                const double r = (i - centre) / centre;
                h = sinc * besselI0 (beta * std::sqrt (juce::jmax (0.0, 1.0 - r * r))) / besselI0 (beta);
            }
            tpCoeffs[(size_t) k][(size_t) j] = (float) h;
            sum += h;
        }
        for (auto& c : tpCoeffs[(size_t) k])
            c = (float) (c / sum);
    }
}

void Limiter::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    lookaheadSamples  = juce::jmax (1, (int) std::round (lookaheadMs * 0.001 * sampleRate));

    // Size for at least stereo so a mono->stereo (or vice versa) block can't index
    // out of range; process() only touches the channels the buffer actually has.
    const int chans = juce::jmax (2, numChannels);
    delayLine.assign ((size_t) chans, std::vector<float> ((size_t) lookaheadSamples, 0.0f));
    writePos = 0;
    gainEnv  = 1.0f;

    // True Peak mode: a longer delay line (the interpolator's delay on top)
    // and the hold and average spans.
    tpHistory.assign ((size_t) chans, TpHistory {});
    tpDelayLine.assign ((size_t) chans, std::vector<float> ((size_t) (lookaheadSamples + tpDelay), 0.0f));
    tpHold = lookaheadSamples + 2;
    holdValue.assign ((size_t) tpHold + 1, 0.0f);
    holdIndex.assign ((size_t) tpHold + 1, 0u);
    boxRing.assign ((size_t) lookaheadSamples, 1.0f);
    resetTruePeak();

    grMeter.prepare (sampleRate);
    updateCoefficients();
}

void Limiter::resetTruePeak()
{
    for (auto& h : tpHistory)
        h = TpHistory {};
    for (auto& line : tpDelayLine)
        std::fill (line.begin(), line.end(), 0.0f);
    tpWritePos = 0;
    holdFront = holdCount = 0;
    sampleCounter = 0;
    tpRelease = 1.0f;
    std::fill (boxRing.begin(), boxRing.end(), 1.0f);
    boxSum = (double) boxRing.size();
    boxPos = 0;
}

void Limiter::updateCoefficients()
{
    // Attack reaches close to target within the look-ahead window so the ducking is
    // largely complete by the time the delayed peak is output; the final clamp mops
    // up any residual overshoot.
    const float attackMs = juce::jmax (0.05f, lookaheadMs * 0.5f);
    attackCoef  = std::exp (-1.0f / (attackMs * 0.001f * (float) currentSampleRate));
    releaseCoef = std::exp (-1.0f / (releaseTimeMs * 0.001f * (float) currentSampleRate));
}

void Limiter::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam      = apvts.getRawParameterValue (prefix + "_Lim_On");
    driveParam   = apvts.getRawParameterValue (prefix + "_Lim_Drive");
    ceilingParam = apvts.getRawParameterValue (prefix + "_Lim_Ceiling");
    releaseParam = apvts.getRawParameterValue (prefix + "_Lim_Release");
    truePeakParam = apvts.getRawParameterValue (prefix + "_Lim_TruePeak");
}

void Limiter::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Lim_On", 1 }, prefix + " Lim On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Lim_Drive", 1 }, prefix + " Lim Drive", 0.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Lim_Ceiling", 1 }, prefix + " Lim Ceiling",
        juce::NormalisableRange<float> (-24.0f, 0.0f, 0.1f), -0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_Lim_Release", 1 }, prefix + " Lim Release",
        juce::NormalisableRange<float> (1.0f, 500.0f, 1.0f, 0.4f), 50.0f));

    // Appended last: the Pd external exposes parameters as inlets in this
    // order. Off by default, the limiter as it was.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_Lim_TruePeak", 2 }, prefix + " Lim True Peak", false));
}

void Limiter::checkParameters()
{
    if (onParam && *onParam != lastOn) { on = *onParam > 0.5f; lastOn = *onParam; }
    if (driveParam && *driveParam != lastDrive)
    {
        driveGain = juce::Decibels::decibelsToGain (driveParam->load());
        lastDrive = *driveParam;
    }
    if (ceilingParam && *ceilingParam != lastCeiling)
    {
        ceilingLin = juce::Decibels::decibelsToGain (ceilingParam->load());
        lastCeiling = *ceilingParam;
    }
    if (releaseParam && *releaseParam != lastRelease)
    {
        releaseTimeMs = *releaseParam;
        lastRelease = *releaseParam;
        updateCoefficients();
    }
    if (truePeakParam && *truePeakParam != lastTruePeak)
    {
        const bool tp = *truePeakParam > 0.5f;
        if (tp && ! truePeak)
            resetTruePeak();   // its own delay line starts empty
        if (! tp && truePeak)
        {
            // Back to sample mode: its delay line holds audio from before.
            for (auto& line : delayLine)
                std::fill (line.begin(), line.end(), 0.0f);
            writePos = 0;
            gainEnv = 1.0f;
        }
        truePeak = tp;
        lastTruePeak = *truePeakParam;
    }
}

void Limiter::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    if (truePeak && ! tpDelayLine.empty())
    {
        processTruePeak (buffer);
        return;
    }

    const int numCh      = juce::jmin (buffer.getNumChannels(), (int) delayLine.size());
    const int numSamples = buffer.getNumSamples();
    auto* ch = buffer.getArrayOfWritePointers();

    const float oneMinusAtt = 1.0f - attackCoef;
    const float oneMinusRel = 1.0f - releaseCoef;

    for (int i = 0; i < numSamples; ++i)
    {
        // Apply input drive and find the linked peak across channels.
        float peak = 0.0f;
        for (int c = 0; c < numCh; ++c)
        {
            const float s = ch[c][i] * driveGain;
            ch[c][i] = s;
            peak = juce::jmax (peak, std::abs (s));
        }

        // Gain that would bring this peak down to the ceiling.
        const float target = (peak > ceilingLin && peak > 0.0f) ? ceilingLin / peak : 1.0f;

        // Branching smoother: fast attack while the gain must fall (a louder peak is
        // entering the look-ahead window), slower release while it recovers. Because
        // the output is delayed by the look-ahead, the envelope has that long to reach
        // the target before the peak emerges.
        if (target < gainEnv) gainEnv = attackCoef  * gainEnv + oneMinusAtt * target;
        else                  gainEnv = releaseCoef * gainEnv + oneMinusRel * target;

        float grLinear = gainEnv;
        grMeter.process (&grLinear, 1);

        // Output the delayed sample with the anticipating gain, store the current
        // (drive-applied) sample, and hard-clamp at the ceiling as the absolute guard.
        for (int c = 0; c < numCh; ++c)
        {
            const float delayed = delayLine[(size_t) c][(size_t) writePos];
            delayLine[(size_t) c][(size_t) writePos] = ch[c][i];
            ch[c][i] = juce::jlimit (-ceilingLin, ceilingLin, delayed * gainEnv);
        }

        if (++writePos >= lookaheadSamples)
            writePos = 0;
    }
}

void Limiter::processTruePeak (juce::AudioBuffer<float>& buffer)
{
    const int numCh      = juce::jmin (buffer.getNumChannels(), (int) tpDelayLine.size());
    const int numSamples = buffer.getNumSamples();
    auto* ch = buffer.getArrayOfWritePointers();

    const int delayLength = lookaheadSamples + tpDelay;
    const int boxLength   = (int) boxRing.size();
    const int holdCap     = (int) holdValue.size();
    const float oneMinusRel = 1.0f - releaseCoef;

    for (int i = 0; i < numSamples; ++i)
    {
        // The true peak of the driven input over [n - 6, n - 5): the sample
        // itself and the three points the interpolator puts after it.
        float peak = 0.0f;
        for (int c = 0; c < numCh; ++c)
        {
            const float s = ch[c][i] * driveGain;
            ch[c][i] = s;

            auto& h = tpHistory[(size_t) c];
            h.pos = h.pos == 0 ? tpTaps - 1 : h.pos - 1;
            h.x[(size_t) h.pos] = h.x[(size_t) (h.pos + tpTaps)] = s;
            const float* x = h.x.data() + h.pos;   // x[j] is the input j samples ago

            for (int k = 0; k < tpPhases; ++k)
            {
                const auto& coeffs = tpCoeffs[(size_t) k];
                float y = 0.0f;
                for (int j = 0; j < tpTaps; ++j)
                    y += coeffs[(size_t) j] * x[j];
                peak = juce::jmax (peak, std::abs (y));
            }
        }

        // The loudest of the last tpHold of them (a monotonic queue).
        const std::uint32_t n = sampleCounter++;
        while (holdCount > 0 && holdValue[(size_t) ((holdFront + holdCount - 1) % holdCap)] <= peak)
            --holdCount;
        const int back = (holdFront + holdCount) % holdCap;
        holdValue[(size_t) back] = peak;
        holdIndex[(size_t) back] = n;
        ++holdCount;
        while (n - holdIndex[(size_t) holdFront] >= (std::uint32_t) tpHold)
        {
            holdFront = (holdFront + 1) % holdCap;
            --holdCount;
        }
        const float held = holdValue[(size_t) holdFront];

        // Its gain: down at once, back up at the release rate, then averaged
        // over the look-ahead, which keeps it smooth and never above the
        // target of any peak still in the delay line.
        const float target = (held > ceilingLin && held > 0.0f) ? ceilingLin / held : 1.0f;
        tpRelease = target < tpRelease ? target : releaseCoef * tpRelease + oneMinusRel * target;

        boxSum += (double) tpRelease - (double) boxRing[(size_t) boxPos];
        boxRing[(size_t) boxPos] = tpRelease;
        if (++boxPos >= boxLength)
        {
            boxPos = 0;
            boxSum = 0.0;   // once per lap, rid the sum of rounding drift
            for (const float v : boxRing)
                boxSum += (double) v;
        }
        const float gain = juce::jmin (1.0f, (float) (boxSum / (double) boxLength));

        float grLinear = gain;
        grMeter.process (&grLinear, 1);

        // Out comes the sample from lookahead + tpDelay ago, the one whose
        // true peak entered the hold lookahead samples ago; the sample clamp
        // stays as the last guard.
        for (int c = 0; c < numCh; ++c)
        {
            auto& line = tpDelayLine[(size_t) c];
            const float delayed = line[(size_t) tpWritePos];
            line[(size_t) tpWritePos] = ch[c][i];
            ch[c][i] = juce::jlimit (-ceilingLin, ceilingLin, delayed * gain);
        }
        if (++tpWritePos >= delayLength)
            tpWritePos = 0;
    }
}
