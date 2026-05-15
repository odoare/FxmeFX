/*
  ==============================================================================

    PdJuceHost.h

    Shared scaffolding for hosting a headless JUCE AudioProcessor inside a
    Pure Data external. Each plugin's PdMain.cpp instantiates a
    PdJuceHost<ProcessorType>, exposes its parameters as Pd float inlets, and
    drives processBlock() from its _perform routine.

  ==============================================================================
*/

#pragma once

#include "m_pd.h"

#include <JuceHeader.h>
#include <atomic>
#include <limits>
#include <memory>
#include <vector>

namespace fxme_pd
{

/** Lazily-constructed, reference-counted JUCE initialiser. Created once on the
    first PD object instantiation and torn down when the last instance is
    freed. Pd loads externals lazily, so we cannot rely on a static at file
    scope. */
class JuceLifetime
{
public:
    static void acquire()
    {
        auto& self = instance();
        if (self.refcount++ == 0)
            self.init = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
    }

    static void release()
    {
        auto& self = instance();
        if (--self.refcount == 0)
            self.init.reset();
    }

private:
    static JuceLifetime& instance()
    {
        static JuceLifetime s;
        return s;
    }

    std::atomic<int> refcount { 0 };
    std::unique_ptr<juce::ScopedJuceInitialiser_GUI> init;
};

/** Wraps a JUCE AudioProcessor for use inside a Pd external. The wrapper
    queries the processor's RangedAudioParameter list and stores one t_float
    slot per parameter; PdMain.cpp wires Pd floatinlets to those slots. Each
    audio callback we forward any changed float-slot values back to the
    parameter via the normalised-value path before calling processBlock. */
template <typename ProcessorType>
class Host
{
public:
    Host (int numChannels = 2)
        : ioChannels (juce::jmax (1, numChannels))
    {
        JuceLifetime::acquire();
        processor = std::make_unique<ProcessorType>();

        const auto& params = processor->getParameters();
        rangedParams.reserve ((size_t) params.size());
        paramSlots.reserve ((size_t) params.size());
        lastForwarded.reserve ((size_t) params.size());

        for (auto* p : params)
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p);
            rangedParams.push_back (ranged);

            const float defaultDenorm = (ranged != nullptr)
                ? ranged->convertFrom0to1 (ranged->getDefaultValue())
                : 0.0f;

            paramSlots.push_back (defaultDenorm);
            lastForwarded.push_back (std::numeric_limits<float>::quiet_NaN());
        }
    }

    ~Host()
    {
        if (processor != nullptr)
            processor->releaseResources();
        processor.reset();
        JuceLifetime::release();
    }

    void prepare (double sampleRate, int blockSize)
    {
        processor->setRateAndBufferSizeDetails (sampleRate, blockSize);
        processor->setPlayConfigDetails (ioChannels, ioChannels, sampleRate, blockSize);
        processor->prepareToPlay (sampleRate, blockSize);
    }

    /** Push any updated values from `paramSlots` into the JUCE parameters.
        Called from the audio thread once per block. */
    void flushParameters()
    {
        const auto n = rangedParams.size();
        for (size_t i = 0; i < n; ++i)
        {
            auto* ranged = rangedParams[i];
            if (ranged == nullptr) continue;

            const float v = paramSlots[i];
            if (v == lastForwarded[i]) continue;

            const auto norm = ranged->getNormalisableRange().convertTo0to1 (v);
            ranged->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
            lastForwarded[i] = v;
        }
    }

    /** Run one Pd-sized block. `channels[i]` points at numSamples floats; the
        wrapper rewrites them in-place. Each channel pointer must be writable
        (Pd is happy to alias inlet and outlet, callers should copy first). */
    void process (float* const* channels, int numChannels, int numSamples)
    {
        flushParameters();

        juce::AudioBuffer<float> buffer (const_cast<float**> (channels),
                                         numChannels, numSamples);
        juce::MidiBuffer midi;
        processor->processBlock (buffer, midi);
    }

    int  numParams() const noexcept              { return (int) paramSlots.size(); }
    t_float* slot (int i) noexcept                { return &paramSlots[(size_t) i]; }
    int  numChannels() const noexcept             { return ioChannels; }
    ProcessorType& get() noexcept                 { return *processor; }

private:
    std::unique_ptr<ProcessorType> processor;
    std::vector<juce::RangedAudioParameter*> rangedParams;
    std::vector<t_float> paramSlots;
    std::vector<float>   lastForwarded;
    int ioChannels;
};

} // namespace fxme_pd

// ─────────────────────────────────────────────────────────────────────────────
// FXME_PD_DEFINE_EXTERNAL — defines all entry points for a Pd signal external
// that wraps a JUCE AudioProcessor.
//
//   ClassName       e.g. fxmecompressor         (no tilde)
//   ProcessorType   e.g. FxmeCompressorAudioProcessor
//
// Pd convention: the binary is named "<ClassName>~.pd_linux" and Pd calls
// "<ClassName>_tilde_setup()" on load. Each instance has:
//   inlet  0  (implicit, CLASS_MAINSIGNALIN)   — audio L
//   inlet  1  (signal)                         — audio R
//   inlet  2..N+1  (float)                     — one per JUCE parameter,
//                                                 in declaration order
//   outlet 0  (signal)                         — audio L
//   outlet 1  (signal)                         — audio R
// ─────────────────────────────────────────────────────────────────────────────
#define FXME_PD_DEFINE_EXTERNAL(ClassName, ProcessorType)                       \
    namespace {                                                                 \
        using HostT_##ClassName = ::fxme_pd::Host<ProcessorType>;               \
        t_class* ClassName##_class = nullptr;                                   \
        struct t_##ClassName {                                                  \
            t_object x_obj;                                                     \
            t_sample f_dummy;                                                   \
            HostT_##ClassName* host;                                            \
        };                                                                      \
        t_int* ClassName##_perform (t_int* w) {                                 \
            auto* x    = reinterpret_cast<t_##ClassName*>(w[1]);                \
            auto* inL  = reinterpret_cast<t_sample*>     (w[2]);                \
            auto* inR  = reinterpret_cast<t_sample*>     (w[3]);                \
            auto* outL = reinterpret_cast<t_sample*>     (w[4]);                \
            auto* outR = reinterpret_cast<t_sample*>     (w[5]);                \
            const int n = static_cast<int>               (w[6]);                \
            if (x->host == nullptr) return w + 7;                               \
            if (outL != inL) std::memcpy (outL, inL, sizeof (t_sample)*(size_t)n);\
            if (outR != inR) std::memcpy (outR, inR, sizeof (t_sample)*(size_t)n);\
            float* channels[2] = { outL, outR };                                \
            x->host->process (channels, 2, n);                                  \
            return w + 7;                                                       \
        }                                                                       \
        void ClassName##_dsp (t_##ClassName* x, t_signal** sp) {                \
            x->host->prepare (sp[0]->s_sr, sp[0]->s_n);                         \
            dsp_add (ClassName##_perform, 6, x,                                 \
                     sp[0]->s_vec, sp[1]->s_vec,                                \
                     sp[2]->s_vec, sp[3]->s_vec,                                \
                     (t_int) sp[0]->s_n);                                       \
        }                                                                       \
        void* ClassName##_new() {                                               \
            auto* x = reinterpret_cast<t_##ClassName*>(pd_new (ClassName##_class));\
            x->host = new HostT_##ClassName (2);                                \
            inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);       \
            for (int i = 0; i < x->host->numParams(); ++i)                      \
                floatinlet_new (&x->x_obj, x->host->slot (i));                  \
            outlet_new (&x->x_obj, &s_signal);                                  \
            outlet_new (&x->x_obj, &s_signal);                                  \
            return x;                                                           \
        }                                                                       \
        void ClassName##_free (t_##ClassName* x) {                              \
            delete x->host;                                                     \
            x->host = nullptr;                                                  \
        }                                                                       \
    }                                                                           \
    extern "C" void ClassName##_tilde_setup (void) {                            \
        ClassName##_class = class_new (gensym (#ClassName "~"),                 \
            reinterpret_cast<t_newmethod>(ClassName##_new),                     \
            reinterpret_cast<t_method>   (ClassName##_free),                    \
            sizeof (t_##ClassName), CLASS_DEFAULT, A_NULL);                     \
        class_addmethod (ClassName##_class,                                     \
            reinterpret_cast<t_method>(ClassName##_dsp),                        \
            gensym ("dsp"), A_CANT, 0);                                         \
        CLASS_MAINSIGNALIN (ClassName##_class, t_##ClassName, f_dummy);         \
    }
