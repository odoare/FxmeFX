# FxmeFX

A small collection of JUCE audio-effect plugins by **FX-Mechanics**.

Each effect is a self-contained DSP class plus a matching GUI component, wrapped
in a minimal `AudioProcessor` / `AudioProcessorEditor` so they can be used both
as standalone plugins (the targets built here) and as building blocks inside
larger projects (the same classes are reused, for example, in the FxmeSampler
drum kits).

## Plugins

| Name             | Plugin code | Description                                    |
| ---------------- | ----------- | ---------------------------------------------- |
| FxmeCompressor   | `COMP`      | Compressor with peak/RMS detector, three release modes (Linear / Opto / Vintage), soft-knee and gain-reduction meter. |
| FxmeEqualizer    | `EQUL`      | 5-band parametric EQ. Each band switches between Lowpass, Highpass, Peak, Low Shelf and High Shelf, with an interactive frequency-response graph. |
| FxmeTube         | `TUBE`      | Valve / tube saturation with four models (Standard, Dynamic, Triode, Class-AB), bias, tone and power-supply sag. |
| FxmeTransient    | `TRNS`      | SPL-style transient designer using the Differential Envelope Technique - independent attack and sustain shaping. |
| FxmeStereoDelay  | `SDEL`      | Tempo-synced stereo delay with cross-feedback and a state-variable lowpass in the feedback path. |
| FxmeConvolReverb | `CREV`      | Convolution reverb (WDL engine) with six embedded impulse responses, length / shape / start-offset shaping, plus an "External…" slot for loading a user IR. |

All plugins build as **VST3**, **AU** (macOS) and a **Standalone** application.

## Repository layout

```
FxmeFX/
├── CMakeLists.txt              # root: builds every plugin
├── Source/
│   ├── Compressor/             # Compressor.{h,cpp} + CompressorComponent.{h,cpp}
│   │                           # + PluginProcessor / PluginEditor / CMakeLists.txt
│   ├── Equalizer/
│   ├── Tube/
│   │   └── img/                # tube.png / tube_bw.png (embedded as binary data)
│   ├── Transient/
│   ├── StereoDelay/
│   ├── ConvolReverb/
│   │   └── ir/                 # built-in impulse responses (embedded as binary data)
│   └── VuMeter/                # shared VU-meter DSP + component
├── WDL/                        # WDL submodule (FFT, convolution engine, resampler)
└── .github/workflows/          # CI: per-OS builds + tag-driven release
```

## Prerequisites

JUCE and the FxmeJuceTools user-module are expected as **siblings of this
repository** (CI mirrors the same layout):

```
<workspace>/
├── FxmeFX/                                  # this repo
├── JUCE/                                    # https://github.com/juce-framework/JUCE
│   └── usermodules/
│       └── FxmeJuceTools/                   # https://github.com/odoare/FxmeJuceTools (its module/ subdir)
└── ...
```

Quick setup:

```bash
git clone https://github.com/juce-framework/JUCE.git ../JUCE
git clone https://github.com/odoare/FxmeJuceTools.git /tmp/_fxme
mkdir -p ../JUCE/usermodules
mv /tmp/_fxme/module/FxmeJuceTools ../JUCE/usermodules/FxmeJuceTools

git submodule update --init --recursive          # pulls in WDL
```

### Linux system dependencies

```bash
sudo apt install -y \
    libasound2-dev libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev
```

## Building

### All plugins from the repository root

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Built artefacts land in `build/Source/<Plugin>/Fxme<Plugin>_artefacts/Release/`.

### A single plugin from the root

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DPLUGIN=Compressor
cmake --build build --parallel
```

`PLUGIN` accepts: `Compressor`, `Equalizer`, `Tube`, `Transient`,
`StereoDelay`, `ConvolReverb`.

### A single plugin standalone

Each plugin's `CMakeLists.txt` also works on its own — useful when iterating
on one effect:

```bash
cmake -S Source/Compressor -B Source/Compressor/build -DCMAKE_BUILD_TYPE=Release
cmake --build Source/Compressor/build --parallel
```

## Releases

Pushing a version tag like `v0.1.0` triggers
[`release.yml`](.github/workflows/release.yml), which builds every plugin on
Linux (x86_64 + arm64), macOS (universal arm64+x86_64) and Windows (x86_64),
zips them per platform, and publishes a GitHub Release with the artefacts
attached.

```bash
git tag v0.1.0
git push origin v0.1.0
```

## License

FxmeFX is released under the **GNU Lesser General Public License, version 3**
(LGPL-3.0) - see [LICENSE](LICENSE).

Vendored / external code keeps its own terms:

- [WDL](WDL/) - zlib-style license (see `WDL/WDL/LICENSE.md`).
- [JUCE](https://juce.com/) - used per its end-user license; the GPL build is
  the one exercised here.
- [FxmeJuceTools](https://github.com/odoare/FxmeJuceTools) - MIT.
