/*
  ==============================================================================

    Tape.cpp

  ==============================================================================
*/

#include "Tape.h"
#include <FxmeTools/dsp/Sanitise.h>
#include <cmath>

namespace
{
    constexpr double kSmoothingSeconds = 0.05;
    constexpr double kOnFadeSeconds    = 0.02;
    constexpr double kVuSeconds        = 0.3;      // VU-like integration
    constexpr double kTapSeconds       = 0.05;     // the loop display's history
    constexpr float  kBiasShelfHz      = 8000.0f;
    constexpr float  kBiasShelfDbPerUnit = -6.0f;  // per unit of bias over 0.5
    constexpr float  kButterworthQ     = 0.7071f;

    float satGainFor (float sat01) { return juce::Decibels::decibelsToGain (-12.0f + 24.0f * sat01); }
}

Tape::Tape()
{
    hissNoise[0].setSeed (0x7461706530);   // fixed seeds: a bounce renders the same
    hissNoise[1].setSeed (0x7461706531);

    // Sized here too, so a GUI opened before prepare() finds valid rings.
    fieldTap.prepare (currentSampleRate, kTapSeconds);
    magTap  .prepare (currentSampleRate, kTapSeconds);
}

Tape::~Tape() = default;

void Tape::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused (samplesPerBlock, numChannels);
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    // Polyphase IIR half-band stages, integer latency (as the Tube's).
    oversampler2x = std::make_unique<OS> (maxChannels, 1, OS::filterHalfBandPolyphaseIIR, true, true);
    oversampler4x = std::make_unique<OS> (maxChannels, 2, OS::filterHalfBandPolyphaseIIR, true, true);
    oversampler2x->initProcessing ((size_t) kChunk);
    oversampler4x->initProcessing ((size_t) kChunk);

    for (auto& s : scratch)
        s.assign ((size_t) kChunk, 0.0f);
    monoScratch .assign ((size_t) kChunk, 0.0f);
    fieldScratch.assign ((size_t) kChunk, 0.0f);
    magScratch  .assign ((size_t) kChunk, 0.0f);

    inMeter .prepare (currentSampleRate);
    outMeter.prepare (currentSampleRate);
    inMeter .setWindowDuration (kVuSeconds);
    outMeter.setWindowDuration (kVuSeconds);
    fieldTap.prepare (currentSampleRate, kTapSeconds);
    magTap  .prepare (currentSampleRate, kTapSeconds);

    setOversampling (oversamplingFactor);
    updateEq();
    updateTape();

    for (auto* s : { &inGainSmooth, &normSmooth, &outSmooth })
        s->reset (currentSampleRate, kSmoothingSeconds);
    mixSmooth.reset (currentSampleRate, kOnFadeSeconds);
    updateGains (true);
    mixSmooth.setCurrentAndTargetValue (on ? mix01 : 0.0f);
    outSmooth.setCurrentAndTargetValue (on ? juce::Decibels::decibelsToGain (outDb) : 1.0f);
    active = true;
}

void Tape::setOversampling (int factor)
{
    oversamplingFactor = factor == 4 ? 4 : 2;
    os = oversamplingFactor == 4 ? oversampler4x.get() : oversampler2x.get();
    if (os == nullptr)
        return;   // not prepared yet

    os->reset();
    latencySamples = juce::jlimit (0, kMaxLatency - 1, juce::roundToInt (os->getLatencyInSamples()));
    for (auto& r : dryRing)
        r.fill (0.0f);

    for (auto& t : tape)
        t.prepare (currentSampleRate * oversamplingFactor);
    updateTape();
    updateOversampledFilters();
}

void Tape::updateOversampledFilters()
{
    const double rate = currentSampleRate * oversamplingFactor;
    const float fc = (float) juce::jmin (fxme::tape::headLossCutoffHz (speed), 0.45 * rate);
    for (auto& f : headLoss)
        f.c = fxme::BiquadCoeffs::lowpass (rate, fc, kButterworthQ);
}

void Tape::updateEq()
{
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        preEmphasis[(size_t) ch].setup (speed, eq, true,  currentSampleRate);
        deEmphasis [(size_t) ch].setup (speed, eq, false, currentSampleRate);
    }

    const auto hb = fxme::tape::headBumpFor (speed);
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        bump  [(size_t) ch].c = fxme::BiquadCoeffs::peaking (currentSampleRate, hb.freqHz, hb.q, hb.gainDb);
        dip   [(size_t) ch].c = fxme::BiquadCoeffs::peaking (currentSampleRate, hb.dipHz, hb.dipQ, hb.dipDb);
        lowCut[(size_t) ch].c = fxme::BiquadCoeffs::highpass (currentSampleRate, hb.lowCutHz, kButterworthQ);
    }
    updateOversampledFilters();

    // The hiss is added before the de-emphasis, which takes some of its
    // power away: measure how much (seeded noise, no allocation), so the Hiss
    // knob reads the level that comes out.
    fxme::tape::EmphasisFilter probe;
    probe.setup (speed, eq, false, currentSampleRate);
    fxme::Random rng (12345);
    double inPower = 0.0, outPower = 0.0;
    for (int i = 0; i < 4096; ++i)
    {
        const float x = rng.nextFloat() * 2.0f - 1.0f;
        const float y = probe.process (x);
        if (i >= 1024)
        {
            inPower += (double) x * x;
            outPower += (double) y * y;
        }
    }
    hissCompensation = outPower > 0.0 ? (float) std::sqrt (inPower / outPower) : 1.0f;
}

void Tape::updateTape()
{
    const auto p = fxme::TapeHysteresis::paramsFor (type, bias01);
    for (auto& t : tape)
        t.setParams (p);

    const float shelfDb = kBiasShelfDbPerUnit * (bias01 - 0.5f);
    for (auto& f : biasShelf)
        f.c = fxme::BiquadCoeffs::highShelf (currentSampleRate, kBiasShelfHz, shelfDb);

    updateGains (false);
}

void Tape::updateGains (bool snap)
{
    const auto p = fxme::TapeHysteresis::paramsFor (type, bias01);
    const float headroom = juce::Decibels::decibelsToGain (fxme::TapeHysteresis::headroomDb (type));
    const float drive = juce::Decibels::decibelsToGain (driveDb);
    const float sat = satGainFor (saturation01);

    // Into the tape: Drive and Saturation push, headroom holds back. Out of
    // it: Saturation and headroom undone, the tape's own gain normalised, and
    // Drive undone too with Auto gain. A quiet signal comes out at Drive's
    // gain (or unity with Auto gain), whatever the type, bias and saturation.
    const float in = drive * sat / headroom;
    const float norm = headroom / (sat * fxme::TapeHysteresis::approxGain (p)) / (autoGain ? drive : 1.0f);

    if (snap)
    {
        inGainSmooth.setCurrentAndTargetValue (in);
        normSmooth  .setCurrentAndTargetValue (norm);
    }
    else
    {
        inGainSmooth.setTargetValue (in);
        normSmooth  .setTargetValue (norm);
    }

    hissGain = hissDb <= kHissOffDb + 0.05f
                 ? 0.0f
                 : juce::Decibels::decibelsToGain (hissDb + fxme::TapeHysteresis::hissOffsetDb (type))
                       * hissCompensation * 1.7320508f;   // uniform noise: RMS is 1/sqrt 3 of the peak
}

void Tape::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                          const juce::String& prefix)
{
    using Range = juce::NormalisableRange<float>;
    auto id = [&] (const char* s) { return juce::ParameterID { prefix + "_Tape_" + s, 1 }; };
    auto name = [&] (const char* s) { return prefix + " Tape " + s; };

    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("On"), name ("On"), true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Drive"), name ("Drive"),
                          Range (-12.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Sat"), name ("Saturation"),
                          Range (0.0f, 100.0f, 0.1f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Bias"), name ("Bias"),
                          Range (0.0f, 100.0f, 0.1f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("Type"), name ("Type"),
                          juce::StringArray { "Modern", "Vintage", "High output" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("Speed"), name ("Speed"),
                          juce::StringArray { "7.5 ips", "15 ips", "30 ips" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("EQ"), name ("EQ"),
                          juce::StringArray { "NAB", "IEC" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Hiss"), name ("Hiss"),
                          Range (kHissOffDb, -40.0f, 0.1f), kHissOffDb,
                          juce::AudioParameterFloatAttributes()
                              .withStringFromValueFunction ([] (float v, int)
                              {
                                  return v <= kHissOffDb + 0.05f ? juce::String ("Off")
                                                                 : juce::String (v, 1) + " dB";
                              })
                              .withValueFromStringFunction ([] (const juce::String& t)
                              {
                                  return t.trim().toLowerCase().startsWith ("off") ? kHissOffDb
                                                                                   : t.getFloatValue();
                              })));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("OS"), name ("Oversampling"),
                          juce::StringArray { "2x", "4x" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("Auto"), name ("Auto gain"), true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Mix"), name ("Mix"),
                          Range (0.0f, 100.0f, 0.1f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Out"), name ("Output"),
                          Range (-24.0f, 24.0f, 0.1f), 0.0f));
}

void Tape::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    auto p = [&] (const char* id) { return apvts.getRawParameterValue (prefix + "_Tape_" + id); };
    onParam    = p ("On");
    driveParam = p ("Drive");
    satParam   = p ("Sat");
    biasParam  = p ("Bias");
    typeParam  = p ("Type");
    speedParam = p ("Speed");
    eqParam    = p ("EQ");
    hissParam  = p ("Hiss");
    osParam    = p ("OS");
    autoParam  = p ("Auto");
    mixParam   = p ("Mix");
    outParam   = p ("Out");
}

void Tape::checkParameters()
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
    auto readChoice = [] (std::atomic<float>* param, int current)
    {
        return param != nullptr ? juce::roundToInt (param->load()) : current;
    };

    // Oversampling first: it re-prepares the tape.
    const int factor = readChoice (osParam, oversamplingFactor == 4 ? 1 : 0) == 1 ? 4 : 2;
    if (factor != oversamplingFactor)
        setOversampling (factor);

    const auto newSpeed = (fxme::tape::Speed) juce::jlimit (0, 2, readChoice (speedParam, (int) speed));
    const auto newEq    = (fxme::tape::EqStandard) juce::jlimit (0, 1, readChoice (eqParam, (int) eq));
    if (newSpeed != speed || newEq != eq)
    {
        speed = newSpeed;
        eq = newEq;
        updateEq();
    }

    bool tapeChanged = false;
    const auto newType = (fxme::TapeHysteresis::TapeType) juce::jlimit (0, 2, readChoice (typeParam, (int) type));
    if (newType != type) { type = newType; tapeChanged = true; }
    float biasPct = bias01 * 100.0f;
    if (read (biasParam, biasPct)) { bias01 = biasPct * 0.01f; tapeChanged = true; }
    if (tapeChanged)
        updateTape();   // gains too

    bool gainsChanged = false;
    if (read (driveParam, driveDb)) gainsChanged = true;
    float satPct = saturation01 * 100.0f;
    if (read (satParam, satPct)) { saturation01 = satPct * 0.01f; gainsChanged = true; }
    if (readBool (autoParam, autoGain)) gainsChanged = true;
    if (read (hissParam, hissDb)) gainsChanged = true;
    if (gainsChanged)
        updateGains (false);

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
        // A fade between the tape and the dry signal (delayed alike).
        mixSmooth.setTargetValue (on ? mix01 : 0.0f);
        outSmooth.setTargetValue (on ? juce::Decibels::decibelsToGain (outDb) : 1.0f);
        if (on)
            active = true;
    }
}

void Tape::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);
    if (numChannels <= 0 || numSamples <= 0 || os == nullptr)
        return;

    // The emphasis filters, the hysteresis and the oversampler hold state:
    // keep NaN and infinities out of them.
    fxme::sanitiseBlock (buffer.getArrayOfWritePointers(), numChannels, numSamples, fxme::kMainPathLimit);

    auto* const* data = buffer.getArrayOfWritePointers();
    for (int start = 0; start < numSamples; start += kChunk)
    {
        float* chunk[maxChannels] = {};
        for (int ch = 0; ch < numChannels; ++ch)
            chunk[ch] = data[ch] + start;
        processChunk (chunk, numChannels, juce::jmin (kChunk, numSamples - start));
    }
}

void Tape::processChunk (float* const* data, int numChannels, int numSamples)
{
    const int latency = latencySamples.load (std::memory_order_relaxed);

    // The dry input, kept for the mix and the input meter.
    for (int ch = 0; ch < numChannels; ++ch)
        std::copy (data[ch], data[ch] + numSamples, scratch[(size_t) ch].begin());
    for (int i = 0; i < numSamples; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            s += scratch[(size_t) ch][(size_t) i];
        monoScratch[(size_t) i] = s / (float) numChannels;
    }
    inMeter.process (monoScratch.data(), numSamples);

    // Off, and the fade is over: the dry signal through its delay only, so
    // the latency stays what the host was told.
    if (! on && ! active)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const int readPos = (dryPos - latency + kMaxLatency) % kMaxLatency;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& ring = dryRing[(size_t) ch];
                ring[(size_t) dryPos] = scratch[(size_t) ch][(size_t) i];
                data[ch][i] = ring[(size_t) readPos];
            }
            dryPos = (dryPos + 1) % kMaxLatency;
        }
        for (int i = 0; i < numSamples; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                s += data[ch][i];
            monoScratch[(size_t) i] = s / (float) numChannels;
        }
        outMeter.process (monoScratch.data(), numSamples);
        return;
    }

    // Record side: gain into the head, pre-emphasis.
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = inGainSmooth.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            data[ch][i] = preEmphasis[(size_t) ch].process (data[ch][i] * g);
    }

    // The tape, oversampled: hysteresis, then the playback head's loss.
    juce::dsp::AudioBlock<float> block (data, (size_t) numChannels, (size_t) numSamples);
    auto up = os->processSamplesUp (block);
    const int factor = oversamplingFactor;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* u = up.getChannelPointer ((size_t) ch);
        auto& model = tape[(size_t) ch];
        auto& loss = headLoss[(size_t) ch];
        for (int j = 0; j < numSamples * factor; ++j)
        {
            const float h = u[j];
            const float m = model.process (h);
            if (ch == 0 && j % factor == factor - 1)
            {
                fieldScratch[(size_t) (j / factor)] = h;
                magScratch  [(size_t) (j / factor)] = m;
            }
            u[j] = loss.processSample (m);
        }
    }
    os->processSamplesDown (block);
    fieldTap.push (fieldScratch.data(), numSamples);
    magTap  .push (magScratch.data(),   numSamples);

    // Playback side.
    for (int i = 0; i < numSamples; ++i)
    {
        const float norm = normSmooth.getNextValue();
        const float mix  = mixSmooth.getNextValue();
        const float outG = outSmooth.getNextValue();
        const int readPos = (dryPos - latency + kMaxLatency) % kMaxLatency;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto c = (size_t) ch;
            float y = data[ch][i] * norm;
            if (hissGain > 0.0f)
                y += hissGain * (hissNoise[c].nextFloat() * 2.0f - 1.0f);
            y = deEmphasis[c].process (y);
            y = lowCut[c].processSample (dip[c].processSample (bump[c].processSample (y)));
            y = biasShelf[c].processSample (y);

            auto& ring = dryRing[c];
            ring[(size_t) dryPos] = scratch[c][(size_t) i];
            const float dry = ring[(size_t) readPos];
            data[ch][i] = (dry + mix * (y - dry)) * outG;
        }
        dryPos = (dryPos + 1) % kMaxLatency;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            s += data[ch][i];
        monoScratch[(size_t) i] = s / (float) numChannels;
    }
    outMeter.process (monoScratch.data(), numSamples);

    // The switch-off fade has finished: out of the path, clean state for the
    // next time it is switched on.
    if (! on && ! mixSmooth.isSmoothing() && ! outSmooth.isSmoothing())
    {
        for (int ch = 0; ch < maxChannels; ++ch)
        {
            const auto c = (size_t) ch;
            tape[c].reset();
            preEmphasis[c].reset();
            deEmphasis[c].reset();
            for (auto* f : { &headLoss[c], &bump[c], &dip[c], &lowCut[c], &biasShelf[c] })
                f->reset();
        }
        os->reset();
        active = false;
    }
}
