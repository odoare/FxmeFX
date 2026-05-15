Developer Guide: Converting a JUCE AudioProcessor to a Native Pure Data External
1. Architectural Overview

A standard JUCE VST consists of an AudioProcessor (DSP) and an AudioProcessorEditor (GUI). For a Pure Data external, the GUI layer is discarded entirely.

The Pure Data C API will serve as a wrapper around your headless JUCE AudioProcessor. The external will dynamically query your JUCE plugin for its parameters, generate a dedicated physical float inlet for each one, and stream the audio data directly through JUCE's processBlock.

                  ┌─────────────────────────────────────────┐
                  │            Pure Data Patch              │
                  └────────────────────┬────────────────────┘
                                       │
         ┌─────────────────────────────┼─────────────────────────────┐
         ▼                             ▼                             ▼
   [ Audio In L ]               [ Param 1 Inlet ]             [ Param 2 Inlet ]
         │                             │                             │
         │ (Pd Audio Buffer)           │ (Updates t_float)           │ (Updates t_float)
         ▼                             ▼                             ▼
   ┌─────────────┐               ┌─────────────┐               ┌─────────────┐
   │  pd_input   │               │   param[0]  │               │   param[1]  │
   └─────┬───────┘               └──────┬──────┘               └──────┬──────┘
         │                              │                             │
         │ Wrapper converts to          └──────────────┬──────────────┘
         │ juce::AudioBuffer                           │ Updates JUCE
         ▼                                             ▼ Parameters
   ┌─────────────────────────────────────────────────────────────────────────┐
   │                     JUCE AudioProcessor Engine                          │
   │   - processBlock(juce_buffer, midi)                                     │
   └───────────────────────────────────┬─────────────────────────────────────┘
                                       │
                                       ▼ (Pd Audio Buffer)
                                 [ Audio Out L ]

2. Prerequisites & Workspace Setup

    Locate m_pd.h: Grab this from your local Pure Data installation directory (usually under PureData/src/) or directly from the pure-data GitHub repository. Place it in your project's include path.

    Isolate JUCE Files: Ensure your compiler has access to your existing JUCE source modules (juce_audio_basics, juce_audio_processors, etc.).

    Compiler Flag: Ensure your compiler is building a shared dynamic library and is compiling with C++17 or C++20 enabled (required by modern JUCE).

3. Step-by-Step Implementation Instructions
Step 1: Strip the GUI

In your original JUCE source code, locate your AudioProcessor implementation file (PluginProcessor.cpp). Ensure the following methods are stripped down or neutralized:

    hasEditor() must return false.

    createEditor() must return nullptr.

Step 2: Define the Pure Data Object Structure

In your C++ wrapper file, define the object structure. It must hold a pointer to your JUCE class, a block storage array for your parameter values, and pointers to the dynamically generated inlets.
C++

extern "C" {
    #include "m_pd.h"
}
#include "PluginProcessor.h" // Your JUCE AudioProcessor header

static t_class *juce_pd_class;

struct t_juce_pd {
    t_object  x_obj;             // Standard Pd object handle
    t_sample  f;                 // Dummy variable for signal objects
    
    // JUCE Engine Pointer
    YourJuceAudioProcessor* processor; 
    
    // Parameter Tracking
    int       num_params;
    t_float* param_values;      // Internal array where Pd inlets write values
};

Step 3: Dynamic Parameter and Inlet Allocation (_new Constructor)

When a user instantiates your object in a Pd patch, the constructor queries JUCE to find out how many parameters exist. It then dynamically allocates an array of floats and creates a dedicated inlet for each parameter.
C++

void *juce_pd_new(void) {
    t_juce_pd *x = (t_juce_pd *)pd_new(juce_pd_class);
    
    // 1. Instantiate the JUCE Processor
    juce::initialiseJUCE_NonGUI(); 
    x->processor = new YourJuceAudioProcessor();
    
    // 2. Query JUCE Parameters
    const auto& parameters = x->processor->getParameters();
    x->num_params = parameters.size();
    
    // 3. Allocate memory to store incoming values from inlets
    x->param_values = (t_float *)getbytes(x->num_params * sizeof(t_float));
    
    // 4. Create Standard Audio Inlets / Outlets
    // (Note: First signal inlet is created automatically by dsp_new)
    dsp_new(&x->x_obj, 1); 
    outlet_new(&x->x_obj, &s_signal); // Audio Outlet Left
    
    // 5. DYNAMICALLY CREATE PARAMETER INLETS
    // We map each inlet directly to its corresponding slot in our array
    for (int i = 0; i < x->num_params; ++i) {
        // Initialize with the default JUCE parameter value
        x->param_values[i] = parameters[i]->getDefaultValue();
        
        // This automatically redirects incoming floats on this inlet to &x->param_values[i]
        floatinlet_new(&x->x_obj, &x->param_values[i]);
    }

    return (void *)x;
}

Step 4: Map the Audio Graph (_dsp Method)

This function tells Pure Data where to find your audio signal vectors and registers your real-time processing loop.
C++

void juce_pd_dsp(t_juce_pd *x, t_signal **sp) {
    // Pass current Sample Rate and Block Size to JUCE
    double sample_rate = sp[0]->s_sr;
    int block_size = sp[0]->s_n;
    
    x->processor->setRateAndBufferSizeDetails(sample_rate, block_size);
    x->processor->prepareToPlay(sample_rate, block_size);

    // Register perform routine: pass object, block size, input vector, output vector
    dsp_add(juce_pd_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

Step 5: The Real-Time Audio Loop (_perform Method)

Inside the high-priority audio thread, the wrapper updates the JUCE parameters from the float array before asking JUCE to process the audio buffer.
C++

t_int *juce_pd_perform(t_int *w) {
    t_juce_pd *x = (t_juce_pd *)(w[1]);
    int block_size = (int)(w[2]);
    t_sample *in   = (t_sample *)(w[3]);
    t_sample *out  = (t_sample *)(w[4]);
    
    if (x->processor == nullptr) return (w + 5);

    // 1. Flush updated float values from inlets into JUCE parameters
    const auto& parameters = x->processor->getParameters();
    for (int i = 0; i < x->num_params; ++i) {
        parameters[i]->setValueNotifyingHost(x->param_values[i]);
    }

    // 2. Prepare Zero-Copy Audio Buffer Wrapper for JUCE
    float* channels[] = { out };
    std::memcpy(out, in, block_size * sizeof(float)); // Copy input to output for in-place processing
    juce::AudioBuffer<float> buffer(channels, 1, block_size);

    // 3. Execute JUCE DSP Engine
    juce::MidiBuffer midi;
    x->processor->processBlock(buffer, midi);

    return (w + 5);
}

Step 6: Memory Cleanup (_free Destructor)

Failing to release JUCE from memory will cause Pure Data to crash on patch closure. Clean up your dynamic memory allocations properly.
C++

void juce_pd_free(t_juce_pd *x) {
    dsp_free(&x->x_obj);
    
    if (x->processor != nullptr) {
        x->processor->releaseResources();
        delete x->processor;
    }
    
    if (x->param_values != nullptr) {
        freebytes(x->param_values, x->num_params * sizeof(t_float));
    }
    
    juce::shutdownJUCE();
}

Step 7: Main Setup Entry Hook

This function runs the exact moment Pure Data loads your external library bin file.
C++

extern "C" void your_plugin_name_setup(void) {
    juce_pd_class = class_new(gensym("your_plugin_name~"),
                              (t_newmethod)juce_pd_new,
                              (t_method)juce_pd_free,
                              sizeof(t_juce_pd),
                              CLASS_DEFAULT, 
                              A_DEFFLOAT, 0);

    class_addmethod(juce_pd_class, (t_method)juce_pd_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(juce_pd_class, t_juce_pd, f);
}

4. Compilation Order of Operations

When configuring your build script (via Make or CMake):

    Compile JUCE source modules as static objects (.o / .obj).

    Compile your AudioProcessor classes while defining the macro JUCE_AUDIOPROCESSOR_NO_GUI=1.

    Compile your Pd wrapper code referencing m_pd.h.

    Link everything together into a single binary named your_plugin_name~.pd_linux (Linux), your_plugin_name~.pd_darwin (macOS), or your_plugin_name~.dll (Windows).