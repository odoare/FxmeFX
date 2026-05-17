# Converting a FxmeFX plugin to a Pure Data external

This is the recipe actually used in this repo. The reusable scaffolding lives
in [`Source/PdCommon/`](../Source/PdCommon/); each plugin just adds one source
file and one CMake block.

## Architecture

A FxmeFX plugin is a `juce::AudioProcessor` (DSP + `AudioProcessorValueTreeState`)
plus a `juce::AudioProcessorEditor` (GUI). The Pd external **reuses the same
`AudioProcessor` unchanged** and discards the editor entirely. The Pd patch
sees:

```
inlet 0  (signal)   audio in L     ──┐
inlet 1  (signal)   audio in R     ──┤
inlet 2  (float)    param 0        ──┤
   …                                 │      ┌─────────────────────────────────┐
inlet N+1 (float)   param N-1      ──┴────► │  fxme_pd::Host<ProcessorType>   │
                                            │  ↳ processBlock(buffer, midi)   │
outlet 0 (signal)   audio out L    ◄────────│                                 │
outlet 1 (signal)   audio out R    ◄────────└─────────────────────────────────┘
```

The N parameter inlets are derived from `processor->getParameters()` at
construction time, in declaration order, **denormalised** (a `t_float` slot
per parameter is wired straight to a `floatinlet_new`, and the host re-
normalises before calling `setValueNotifyingHost` once per audio block).

## What lives in `Source/PdCommon/`

| File | Purpose |
| --- | --- |
| [`m_pd.h`](../Source/PdCommon/m_pd.h) | Pure Data C ABI header (vendored, version-pinned). |
| [`JuceHeaderShim/JuceHeader.h`](../Source/PdCommon/JuceHeaderShim/JuceHeader.h) | Drop-in `<JuceHeader.h>` for non-`juce_add_plugin` targets. Only includes the modules the Pd build actually links — no `juce_audio_devices`, no `juce_audio_plugin_client`. |
| [`PdJuceHost.h`](../Source/PdCommon/PdJuceHost.h) | The reusable runtime: `fxme_pd::JuceLifetime` (ref-counted `ScopedJuceInitialiser_GUI`), `fxme_pd::Host<ProcessorType>` (wraps the AudioProcessor, owns the t_float parameter slots, flushes them into JUCE once per block), and the `FXME_PD_DEFINE_EXTERNAL` macro that emits the Pd boilerplate (`_new` / `_free` / `_dsp` / `_perform` / `_tilde_setup`). |
| [`PdExternal.cmake`](../Source/PdCommon/PdExternal.cmake) | Provides `fxme_add_pd_external(target LIBNAME … SOURCES … INCLUDES … DEFINES … LINK_LIBRARIES …)`. Adds a CMake `MODULE` library, sets the correct extension (`.pd_linux` / `.pd_darwin` / `.dll`), strips the `lib` prefix, defines `FXME_PD_BUILD=1` plus generic `JucePlugin_*` macros, links the JUCE modules required for headless `AudioProcessor` hosting, and on macOS sets `-undefined dynamic_lookup` so the `pd_…` symbols are resolved by the host at load time. |

The host (template `fxme_pd::Host<ProcessorType>`) does three things:

1. Constructs the JUCE processor inside a ref-counted `ScopedJuceInitialiser_GUI`.
2. Builds a `std::vector<t_float>` with one slot per `RangedAudioParameter`,
   pre-loaded with `convertFrom0to1(getDefaultValue())`. Pd's
   `floatinlet_new` writes into those slots directly.
3. On each audio block, walks the slots and — for any that changed — calls
   `setValueNotifyingHost(convertTo0to1(slot))` before invoking
   `processBlock`.

The `FXME_PD_DEFINE_EXTERNAL(ClassName, ProcessorType)` macro emits a fixed
two-in / two-out stereo signal external. The Pd binary is named
`<ClassName>~.pd_<platform>` and Pd calls `<ClassName>_tilde_setup()` on load.

## Per-plugin recipe

Adding PD support to a plugin is three steps. Estimate: **~15 minutes**.

### 1. Add `PdMain.cpp`

A single-line entry-point file next to `PluginProcessor.cpp`:

```cpp
// Source/<Plugin>/PdMain.cpp
#include "../PdCommon/PdJuceHost.h"
#include "PluginProcessor.h"

#include <cstring>

FXME_PD_DEFINE_EXTERNAL (fxmecompressor, FxmeCompressorAudioProcessor)
//                       ^ Pd class name  ^ JUCE AudioProcessor type
//                       (no tilde — the macro appends it)
```

The Pd class name is conventionally `fxme<name>` (lower-case, no underscores).
Pd patches will refer to the object as `fxmecompressor~`.

### 2. Guard `PluginProcessor.cpp` against the editor

Two minimal edits — both already in every existing plugin, copy the pattern:

```cpp
#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"           // <-- guard 1
#endif

…

#ifdef FXME_PD_BUILD                  // <-- guard 2
bool FxmeXxxAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeXxxAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeXxxAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeXxxAudioProcessor::createEditor()
{
    return new FxmeXxxAudioProcessorEditor (*this);
}
#endif
```

Nothing else in `PluginProcessor.cpp` needs to change. **The DSP class
(`Compressor`, `Equalizer`, …) and `AudioProcessorValueTreeState` setup are
identical between the VST3 and PD builds** — that's the whole point.

### 3. Append a PD block to the plugin's `CMakeLists.txt`

After the existing `juce_add_plugin(…) / target_link_libraries(…)` block, add:

```cmake
# ── Pure Data external ────────────────────────────────────────────────────────
include(${CMAKE_CURRENT_LIST_DIR}/../PdCommon/PdExternal.cmake)
fxme_add_pd_external(FxmeCompressor_pd
    LIBNAME   fxmecompressor~                # final binary file name
    SOURCES
        PdMain.cpp
        PluginProcessor.cpp                  # reused as-is, FXME_PD_BUILD branches strip the editor
        Compressor.cpp                       # the DSP class
        ../VuMeter/VuMeter.cpp               # anything else the DSP path touches
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../VuMeter
    DEFINES
        JucePlugin_Name="FxmeCompressor"     # the only JucePlugin_* macro that varies per plugin
    # LINK_LIBRARIES FxmeXxxBinaryData       # if the plugin embeds binary data (e.g. ConvolReverb, Cab)
)
```

Notes:

- **Do not list `CMakeLists.txt`-local `*Component.cpp` files** — the editor
  components and FxmeJuceTools helpers are never compiled into the PD external.
- **Binary-data targets pass through `LINK_LIBRARIES`**. The PD CMake reuses
  the same `juce_add_binary_data(…)` target the VST build already created
  (e.g. `FxmeConvolReverbBinaryData`, `FxmeCabBinaryData`).
- **No need to change the parameter list for PD** unless the plugin has
  parameters that don't make sense without a UI (the canonical case is
  ConvolReverb's "External IR file" slot — see
  [`ConvolReverb::addParameters`](../Source/ConvolReverb/ConvolReverb.cpp),
  which uses `#ifdef FXME_PD_BUILD` to shrink the parameter range).

## Build & test

```bash
# A single plugin's PD external (fast):
cmake -S Source/Compressor -B Source/Compressor/build-pd -DCMAKE_BUILD_TYPE=Release
cmake --build Source/Compressor/build-pd --target FxmeCompressor_pd --parallel 4

# Produces:
#   Source/Compressor/build-pd/fxmecompressor~.pd_linux   (or .pd_darwin / .dll)

# From the project root, all PD externals at once:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target FxmeCompressor_pd FxmeEqualizer_pd … --parallel 4
```

`build-pd/` is in [`.gitignore`](../.gitignore).

In Pure Data, drop the produced file into a patch's directory (or
`Pd/extra/`) and instantiate `[fxmecompressor~]`. Inlet count:
**2 signal + N float**, where N is the number of `RangedAudioParameter`s the
processor exposes. Pd's right-click → "Help" works once you add a help
patch — that's outside this guide.

## Gotchas (lessons learned the slow way)

1. **`JuceHeaderShim` is a separate include path from the normal Projucer-
   style `JuceLibraryCode/`.** PD builds use the shim because we explicitly
   *don't* want `juce_audio_plugin_client` (it injects a `createPluginFilter`
   entry point that conflicts with our Pd entry) and the device modules
   (they pull in ALSA / Core Audio / WASAPI for no reason). Don't try to
   share a JuceHeader.h between the VST and PD builds.

2. **`juce::AudioProcessorValueTreeState` transitively depends on
   `juce_gui_*`.** That is why `PdExternal.cmake` still links the JUCE GUI
   modules even though `hasEditor()` is false. On Linux this means a PD
   external loads X11/freetype/fontconfig at `dlopen` time — none of them
   are instantiated, they're just present in the link graph. Don't try to
   prune them; APVTS will not compile without them.

3. **Pd loads externals lazily, instances come and go.** That's why
   `JuceLifetime` is ref-counted. Don't hoist `ScopedJuceInitialiser_GUI`
   to a file-scope static — it'll either run too early (before Pd has set
   up its threads) or never tear down.

4. **`floatinlet_new` writes denormalised values.** The Pd patch sees the
   parameter in its natural units (dB, Hz, ratio, …), not 0..1. The host
   converts before forwarding to JUCE. This is the right choice
   ergonomically (`[400(` to `[oct_detect_inlet]` "just works") but the
   `fxme_pd::Host::flushParameters` path is the only place the conversion
   happens — don't bypass it.

5. **The `LIBNAME` includes the tilde.** Pd's loader matches by file name,
   not by symbol. Get this wrong and Pd silently fails to find the class —
   no error, the object just appears as a red box.

6. **`-undefined dynamic_lookup` is required on macOS.** Pd's ABI symbols
   (`pd_new`, `outlet_new`, `dsp_add`, …) are resolved against the host
   process at load time. On Linux this is the default GNU ld behaviour;
   on macOS you have to ask for it explicitly. PdExternal.cmake does this
   already — don't override it.

7. **Plugin-specific `#ifdef FXME_PD_BUILD` branches belong in the DSP class
   or `addParameters`, not in `PdMain.cpp`.** `PdMain.cpp` is a single macro
   call by design — keep it that way.

## Adding a new plugin: checklist

Once the infrastructure above exists, adding PD support to a new effect is:

- [ ] Create `Source/<Plugin>/PdMain.cpp` (one macro call).
- [ ] Add `#ifndef FXME_PD_BUILD` around the `PluginEditor.h` include and
      wrap `hasEditor()` / `createEditor()` in `Source/<Plugin>/PluginProcessor.cpp`.
- [ ] Append the `fxme_add_pd_external(…)` block to
      `Source/<Plugin>/CMakeLists.txt`.
- [ ] Pick a Pd class name (lower-case, no underscores; conventionally
      `fxme<name>`).
- [ ] List any binary-data targets the DSP depends on in `LINK_LIBRARIES`.
- [ ] Build the `Fxme<Plugin>_pd` target and load the resulting
      `fxme<name>~.pd_<platform>` into a test patch.

That's the whole conversion. The first time we did this took a couple of
hours of design work to land on the
`Host` / `JuceLifetime` / `FXME_PD_DEFINE_EXTERNAL` split; subsequent
plugins should be ~15 minutes each.
