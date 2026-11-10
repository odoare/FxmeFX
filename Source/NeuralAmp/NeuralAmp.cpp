/*
  ==============================================================================

    NeuralAmp.cpp

  ==============================================================================
*/

#include "NeuralAmp.h"
#include <FxmeTools/dsp/Sanitise.h>

using fxme::neural::ModelLoader;

namespace
{
    constexpr double kSmoothingSeconds = 0.02;   // gains, mix, the On fade
    constexpr double kModeFadeSeconds  = 0.01;   // the dip around a Channels switch
    constexpr double kPreRollSeconds   = 0.01;   // after the latency, before a fade-in
    constexpr float  kGateHysteresisDb = 6.0f;
    constexpr float  kGateHoldSeconds  = 0.03f;
    constexpr double kBassHz = 150.0, kMidHz = 425.0, kTrebleHz = 1800.0;

    fxme::neural::ModelData toModelData (const juce::MemoryBlock& bytes, const juce::String& fileName)
    {
        fxme::neural::ModelData d;
        d.fileName = fileName.toStdString();
        const auto* p = static_cast<const char*> (bytes.getData());
        d.bytes.assign (p, p + bytes.getSize());
        return d;
    }
}

NeuralAmp::NeuralAmp() = default;

NeuralAmp::~NeuralAmp()
{
    loader.stopThread (4000);
}

//==============================================================================
void NeuralAmp::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix)
{
    using Range = juce::NormalisableRange<float>;
    auto id = [&] (const char* s) { return juce::ParameterID { prefix + "_NAmp_" + s, 1 }; };
    auto name = [&] (const char* s) { return prefix + " NeuralAmp " + s; };
    auto toneParam = [&] (const char* s)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (id (s), name (s), Range (-12.0f, 12.0f, 0.1f), 0.0f));
    };

    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("On"), name ("On"), true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("In"), name ("Input"),
                          Range (-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Gate"), name ("Gate"),
                          Range (kGateOffDb, -20.0f, 0.1f), kGateOffDb,
                          juce::AudioParameterFloatAttributes()
                              .withStringFromValueFunction ([] (float v, int)
                              {
                                  return v <= kGateOffDb + 0.05f ? juce::String ("Off")
                                                                 : juce::String (v, 1) + " dB";
                              })
                              .withValueFromStringFunction ([] (const juce::String& t)
                              {
                                  return t.trim().toLowerCase().startsWith ("off") ? kGateOffDb
                                                                                   : t.getFloatValue();
                              })));
    toneParam ("Bass");
    toneParam ("Mid");
    toneParam ("Treble");
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Out"), name ("Output"),
                          Range (-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Mix"), name ("Mix"),
                          Range (0.0f, 100.0f, 0.1f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (id ("Chan"), name ("Channels"),
                          juce::StringArray { "Dual mono", "Mono" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (id ("Level"), name ("Level"),
                          Range (0.0f, 24.0f, 0.1f), 12.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("Calib"), name ("Calibrate"), true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (id ("Norm"), name ("Normalize"), true));
}

void NeuralAmp::assignParameters (juce::AudioProcessorValueTreeState& state, const juce::String& p)
{
    apvts = &state;
    prefix = p;
    auto get = [&] (const char* s) { return state.getRawParameterValue (p + "_NAmp_" + s); };
    onParam     = get ("On");
    inParam     = get ("In");
    gateParam   = get ("Gate");
    bassParam   = get ("Bass");
    midParam    = get ("Mid");
    trebleParam = get ("Treble");
    mixParam    = get ("Mix");
    outParam    = get ("Out");
    chanParam   = get ("Chan");
    levelParam  = get ("Level");
    calibParam  = get ("Calib");
    normParam   = get ("Norm");
}

//==============================================================================
void NeuralAmp::prepare (double sampleRate, int samplesPerBlock, int)
{
    maxBlock = juce::jmax (16, samplesPerBlock);
    {
        const juce::ScopedLock sl (loaderLock);
        currentSampleRate = sampleRate;
        for (auto& e : engines)
            e.prepare (sampleRate, maxBlock);   // drops the models: they were built for the old rate
        builtGeneration = -1;
        latencySamples = engines[0].getLatencySamples();

        // The model, built here and now: the host is not processing yet, and
        // the session then starts with its amp rather than a moment of dry.
        Request r;
        {
            const juce::ScopedLock rl (requestLock);
            r = wanted;
        }
        if (! isMirror() && r.bytes.getSize() > 0)
        {
            juce::String error;
            const bool ok = loadInto (r, error);
            setStatus ([&] (ModelStatus& s)
            {
                s.loading = false;
                s.ready = ok;
                s.error = error;
            });
        }
        builtGeneration = r.generation;
    }

    const int latency = latencySamples.load();
    dryLength = latency;
    for (auto& d : dryDelay)
        d.assign ((size_t) juce::jmax (1, latency), 0.0f);
    dryPos = 0;
    for (auto& w : wet)
        w.assign ((size_t) maxBlock, 0.0f);
    monoScratch.assign ((size_t) maxBlock, 0.0f);

    inMeter.prepare (sampleRate);
    outMeter.prepare (sampleRate);
    inMeter.setWindowDuration (0.3f);
    outMeter.setWindowDuration (0.3f);

    for (auto* s : { &inSmooth, &mixSmooth, &outSmooth })
        s->reset (sampleRate, kSmoothingSeconds);
    modeSmooth.reset (sampleRate, kModeFadeSeconds);

    gate.prepare (sampleRate);
    gate.setHold (kGateHoldSeconds);
    gate.reset();
    peakEnv = 0.0f;
    peakFall = (float) std::exp (-1.0 / (0.010 * sampleRate));
    gateOpenCoef = (float) (1.0 - std::exp (-1.0 / (0.001 * sampleRate)));
    gateCloseCoef = (float) (1.0 - std::exp (-1.0 / (0.060 * sampleRate)));
    gateGain = 1.0f;

    for (auto& ch : tone)
        for (auto& f : ch)
            f.reset();

    preRoll = 0;
    modePreRoll = 0;
    active = true;
    checkParameters();
    channels = pendingChannels;
    updateToneStack();
    for (auto* s : { &inSmooth, &mixSmooth, &outSmooth })
        s->setCurrentAndTargetValue (s->getTargetValue());
    modeSmooth.setCurrentAndTargetValue (1.0f);

    if (! loader.isThreadRunning())
        loader.startThread (juce::Thread::Priority::low);
}

//==============================================================================
void NeuralAmp::checkParameters()
{
    if (onParam == nullptr)
        return;

    const bool wasOn = on;
    on        = onParam->load() > 0.5f;
    inDb      = inParam->load();
    gateDb    = gateParam->load();
    mix01     = mixParam->load() * 0.01f;
    outDb     = outParam->load();
    levelDbu  = levelParam->load();
    calibrate = calibParam->load() > 0.5f;
    normalise = normParam->load() > 0.5f;

    const float b = bassParam->load(), m = midParam->load(), t = trebleParam->load();
    if (! juce::exactlyEqual (b, bassDb) || ! juce::exactlyEqual (m, midDb) || ! juce::exactlyEqual (t, trebleDb))
    {
        bassDb = b;
        midDb = m;
        trebleDb = t;
        updateToneStack();
    }

    for (auto& e : engines)
    {
        e.setInputLevelDbu (levelDbu);
        e.setCalibrateInput (calibrate);
        e.setNormaliseOutput (normalise);
    }

    gate.setThresholds (gateDb, kGateHysteresisDb);

    // Back on after the fade had finished: the models stood still, so they
    // run unheard until their history is the present again.
    if (on && ! wasOn && ! active && currentSampleRate > 0.0)
    {
        active = true;
        preRoll = latencySamples.load() + (int) (kPreRollSeconds * currentSampleRate);
        mixSmooth.setCurrentAndTargetValue (0.0f);
    }

    inSmooth.setTargetValue (juce::Decibels::decibelsToGain (inDb));
    mixSmooth.setTargetValue (on && preRoll == 0 ? mix01 : 0.0f);
    outSmooth.setTargetValue (on ? juce::Decibels::decibelsToGain (outDb) : 1.0f);

    const auto ch = (Channels) juce::jlimit (0, 1, juce::roundToInt (chanParam->load()));
    if (ch != pendingChannels)
    {
        pendingChannels = ch;
        modeSmooth.setTargetValue (0.0f);   // down, switch, back up (processChunk)
    }
}

void NeuralAmp::updateToneStack()
{
    if (currentSampleRate <= 0.0)
        return;
    const auto bass = fxme::BiquadCoeffs::lowShelf (currentSampleRate, (float) kBassHz, bassDb);
    const auto mid = fxme::BiquadCoeffs::peaking (currentSampleRate, (float) kMidHz, 0.7071f, midDb);
    const auto treble = fxme::BiquadCoeffs::highShelf (currentSampleRate, (float) kTrebleHz, trebleDb);
    for (auto& ch : tone)
    {
        ch[0].c = bass;
        ch[1].c = mid;
        ch[2].c = treble;
    }
}

//==============================================================================
void NeuralAmp::process (juce::AudioBuffer<float>& buffer)
{
    checkParameters();
    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);
    const int numSamples = buffer.getNumSamples();
    if (numChannels == 0 || currentSampleRate <= 0.0)
        return;

    fxme::sanitiseBlock (buffer.getArrayOfWritePointers(), numChannels, numSamples, fxme::kMainPathLimit);

    for (int done = 0; done < numSamples; done += maxBlock)
    {
        const int n = juce::jmin (maxBlock, numSamples - done);
        float* chunk[maxChannels] {};
        for (int c = 0; c < numChannels; ++c)
            chunk[c] = buffer.getWritePointer (c, done);
        processChunk (chunk, numChannels, n);
    }
}

void NeuralAmp::processChunk (float* const* data, int numChannels, int n)
{
    // The input level, the two sides averaged.
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < numChannels; ++c)
            s += data[c][i];
        monoScratch[(size_t) i] = s / (float) numChannels;
    }
    inMeter.process (monoScratch.data(), n);

    auto delayInPlace = [this, data, numChannels, n]
    {
        if (dryLength == 0)
            return;
        int pos = dryPos;
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < numChannels; ++c)
            {
                auto& ring = dryDelay[(size_t) c];
                const float y = ring[(size_t) pos];
                ring[(size_t) pos] = data[c][i];
                data[c][i] = y;
            }
            if (++pos == dryLength)
                pos = 0;
        }
        dryPos = pos;
    };

    if (! active)
    {
        // Faded out: the dry signal only, still delayed.
        delayInPlace();
        gateState = -1.0f;
        for (int i = 0; i < n; ++i)
            monoScratch[(size_t) i] = data[0][i];
        outMeter.process (monoScratch.data(), n);
        return;
    }

    // The wet path starts from the input, then `data` becomes the delayed dry.
    for (int c = 0; c < numChannels; ++c)
        std::copy (data[c], data[c] + n, wet[(size_t) c].data());
    delayInPlace();

    // Input gain.
    for (int i = 0; i < n; ++i)
    {
        const float g = inSmooth.getNextValue();
        for (int c = 0; c < numChannels; ++c)
            wet[(size_t) c][(size_t) i] *= g;
    }

    // Gate, before the model.
    if (gateDb > kGateOffDb + 0.05f)
    {
        auto st = fxme::GateStateMachine::State::closed;
        for (int i = 0; i < n; ++i)
        {
            float level = 0.0f;
            for (int c = 0; c < numChannels; ++c)
                level = juce::jmax (level, std::abs (wet[(size_t) c][(size_t) i]));
            peakEnv = level > peakEnv ? level : peakEnv * peakFall;
            st = gate.process (juce::Decibels::gainToDecibels (peakEnv, -120.0f));
            const float target = st != fxme::GateStateMachine::State::closed ? 1.0f : 0.0f;
            gateGain += (target - gateGain) * (target > gateGain ? gateOpenCoef : gateCloseCoef);
            for (int c = 0; c < numChannels; ++c)
                wet[(size_t) c][(size_t) i] *= gateGain;
        }
        gateState = (float) (int) st;
    }
    else
    {
        gateGain = 1.0f;
        gateState = -1.0f;
    }

    // A Channels switch: once the dip has reached silence, switch, let the
    // engine that was idle fill its history unheard, then come back up.
    if (channels != pendingChannels && modeSmooth.getCurrentValue() <= 0.0f && ! modeSmooth.isSmoothing())
    {
        channels = pendingChannels;
        modePreRoll = latencySamples.load() + (int) (kPreRollSeconds * currentSampleRate);
    }

    // The model(s).
    if (numChannels == 1 || channels == Channels::mono)
    {
        if (numChannels == 2)
        {
            for (int i = 0; i < n; ++i)
                wet[0][(size_t) i] = 0.5f * (wet[0][(size_t) i] + wet[1][(size_t) i]);
        }
        engines[0].process (wet[0].data(), n);
        if (numChannels == 2)
            std::copy (wet[0].begin(), wet[0].begin() + n, wet[1].begin());
    }
    else
    {
        for (int c = 0; c < numChannels; ++c)
            engines[(size_t) c].process (wet[(size_t) c].data(), n);
    }

    // Tone stack.
    for (int c = 0; c < numChannels; ++c)
        for (auto& f : tone[(size_t) c])
            f.processBlock (wet[(size_t) c].data(), n);

    // Mix, the Channels dip, output.
    for (int i = 0; i < n; ++i)
    {
        const float m = mixSmooth.getNextValue();
        const float o = outSmooth.getNextValue();
        const float d = modeSmooth.getNextValue();
        for (int c = 0; c < numChannels; ++c)
        {
            float& y = data[c][i];
            y = (y * (1.0f - m) + wet[(size_t) c][(size_t) i] * m * d) * o;
        }
    }

    if (preRoll > 0)
    {
        preRoll = juce::jmax (0, preRoll - n);
        if (preRoll == 0)
            mixSmooth.setTargetValue (on ? mix01 : 0.0f);
    }
    modePreRoll = juce::jmax (0, modePreRoll - n);
    if (channels == pendingChannels && modePreRoll == 0 && modeSmooth.getTargetValue() < 1.0f)
        modeSmooth.setTargetValue (1.0f);

    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < numChannels; ++c)
            s += data[c][i];
        monoScratch[(size_t) i] = s / (float) numChannels;
    }
    outMeter.process (monoScratch.data(), n);

    if (! on && ! mixSmooth.isSmoothing() && ! outSmooth.isSmoothing())
    {
        active = false;
        gate.reset();
        peakEnv = 0.0f;
        gateGain = 1.0f;
    }
}

//==============================================================================
// The model.

void NeuralAmp::setStatus (std::function<void (ModelStatus&)> change)
{
    {
        const juce::ScopedLock sl (statusLock);
        change (status);
        // A mirror's value is the server's, brought by FxmeNet's telemetry.
        if (! isMirror())
            modelState = ! status.hasModel ? 0.0f
                       : status.loading ? 1.0f
                       : status.ready ? 2.0f
                       : 3.0f;
    }
    ++statusGeneration;
}

NeuralAmp::ModelStatus NeuralAmp::getModelStatus() const
{
    ModelStatus s;
    {
        const juce::ScopedLock sl (statusLock);
        s = status;
    }
    // FxmeNet: a mirror plays nothing; what the server's instance does
    // arrives as telemetry (while the editor is open).
    if (isMirror() && s.hasModel && s.error.isEmpty())
    {
        const int v = juce::roundToInt (modelState.load());
        s.loading = v == 1;
        s.ready = v == 2;
        if (v == 3)
            s.error = "The FxmeNet server could not load this model";
        s.remote = true;
    }
    return s;
}

void NeuralAmp::request (juce::MemoryBlock bytes, const juce::String& fileName)
{
    const bool has = bytes.getSize() > 0;
    fxme::neural::ModelInfo info;
    juce::String error;
    if (has)
    {
        std::string e;
        if (auto i = ModelLoader::readInfo (toModelData (bytes, fileName), &e))
            info = *i;
        else
            error = "Not a model: " + juce::String (e);
    }
    const int stored = apvts != nullptr ? fxme::EmbeddedData::getStoredSize (apvts->state, slotId()) : 0;

    {
        const juce::ScopedLock rl (requestLock);
        wanted.bytes = std::move (bytes);
        wanted.fileName = fileName;
        wanted.generation = ++nextGeneration;
    }

    const bool mirror = isMirror();
    setStatus ([&] (ModelStatus& s)
    {
        s.hasModel = has;
        s.loading = has && ! mirror && error.isEmpty();
        s.ready = false;
        s.fileName = fileName;
        s.error = error;
        s.info = info;
        s.storedBytes = stored;
    });
    loader.notify();
}

bool NeuralAmp::loadInto (const Request& r, juce::String& error)
{
    if (r.bytes.getSize() == 0)
    {
        for (auto& e : engines)
            e.unload();
        return false;
    }

    const auto data = toModelData (r.bytes, r.fileName);
    std::array<std::unique_ptr<fxme::neural::PreparedModel>, maxChannels> models;
    for (auto& m : models)
    {
        auto result = ModelLoader::load (data, currentSampleRate, maxBlock);
        if (! result)
        {
            error = juce::String (result.error);
            return false;
        }
        m = std::move (result.model);
    }

    const auto info = models[0]->getInfo();   // completed by the loader (backend, receptive field)
    for (size_t c = 0; c < models.size(); ++c)
        if (! engines[c].install (std::move (models[c])))
        {
            error = "the engine was prepared again meanwhile";
            return false;
        }

    setStatus ([&] (ModelStatus& s) { s.info = info; });
    return true;
}

void NeuralAmp::serviceLoader()
{
    Request r;
    {
        const juce::ScopedLock rl (requestLock);
        r = wanted;   // a copy: the bytes stay valid while building
    }

    const juce::ScopedLock sl (loaderLock);
    if (r.generation != builtGeneration && currentSampleRate > 0.0)
    {
        builtGeneration = r.generation;
        if (isMirror())
            return;   // a mirror shows, it does not play

        juce::String error;
        const bool ok = loadInto (r, error);
        {
            // Only if nothing newer was asked for meanwhile.
            const juce::ScopedLock rl (requestLock);
            if (wanted.generation != r.generation)
                return;
        }
        setStatus ([&] (ModelStatus& s)
        {
            s.loading = false;
            s.ready = ok;
            if (r.bytes.getSize() > 0)
                s.error = error;
        });
    }

    for (auto& e : engines)
        e.collectGarbage();
}

void NeuralAmp::Loader::run()
{
    while (! threadShouldExit())
    {
        owner.serviceLoader();
        wait (200);   // notify() wakes it at once on a request
    }
}

bool NeuralAmp::loadModelFile (const juce::File& file, juce::String& error)
{
    juce::MemoryBlock bytes;
    if (! file.existsAsFile() || ! file.loadFileAsData (bytes) || bytes.getSize() == 0)
    {
        error = "Cannot read " + file.getFileName();
        return false;
    }

    std::string e;
    const auto info = ModelLoader::readInfo (toModelData (bytes, file.getFileName()), &e);
    if (! info || info->architecture.empty())
    {
        error = file.getFileName() + " is not a model (a .nam file, or a Keras .json)";
        return false;
    }

    if (apvts != nullptr)
        fxme::EmbeddedData::embed (apvts->state, slotId(), file.getFileName(), bytes.getData(), bytes.getSize());
    request (std::move (bytes), file.getFileName());
    return true;
}

void NeuralAmp::clearModel()
{
    if (apvts != nullptr)
        fxme::EmbeddedData::removeEmbedded (apvts->state, slotId());
    request ({}, {});
}

void NeuralAmp::syncModelFromState()
{
    if (apvts == nullptr)
        return;
    auto bytes = fxme::EmbeddedData::read (apvts->state, slotId());
    const auto name = fxme::EmbeddedData::getEmbeddedName (apvts->state, slotId());
    {
        const juce::ScopedLock rl (requestLock);
        if (bytes == wanted.bytes && name == wanted.fileName)
            return;   // already the one
    }
    request (std::move (bytes), name);
}

void NeuralAmp::writePresetExtra (juce::XmlElement& extra) const
{
    const juce::ScopedLock rl (requestLock);
    if (wanted.bytes.getSize() == 0)
        return;
    auto* model = extra.createNewChildElement ("Model");
    if (! fxme::EmbeddedData::writeEntry (*model, wanted.fileName, wanted.bytes.getData(), wanted.bytes.getSize()))
        extra.removeChildElement (model, true);
}

void NeuralAmp::readPresetExtra (const juce::XmlElement* extra)
{
    // No model in the preset: keep the one playing (the preset is settings).
    const auto* model = extra != nullptr ? extra->getChildByName ("Model") : nullptr;
    if (model == nullptr)
        return;
    juce::String name;
    auto bytes = fxme::EmbeddedData::readEntry (*model, &name);
    if (bytes.getSize() == 0)
        return;
    if (apvts != nullptr)
        fxme::EmbeddedData::embed (apvts->state, slotId(), name, bytes.getData(), bytes.getSize());
    request (std::move (bytes), name);
}

juce::File NeuralAmp::getModelsFolder()
{
    // <user data>/FX-Mechanics/Modules/NeuralAmp/Presets -> <user data>/FX-Mechanics
    return fxme::PresetManager::getModulePresetDirectory ("NeuralAmp")
               .getParentDirectory().getParentDirectory().getParentDirectory()
               .getChildFile ("Models").getChildFile ("NAM");
}

bool NeuralAmp::isModelFile (const juce::File& f)
{
    return f.hasFileExtension ("nam;json");
}
