# FxmeFX: open items

Cleaned 2026-11-16. What was done (Gate, De-esser, Crusher, Tape, NeuralAmp,
the side-chain inputs, the look and the rest) is in `release-notes/`; the
neural engine's plan and its measurements are in FxmeTools'
`doc/neural-plan.md`.

## Checklist common to every new FxmeFX plugin

- `Source/<Name>/`, laid out like `Source/Compressor/`: `<Name>.h/.cpp` (DSP and parameters, IDs `"<prefix>_<Short>_<Name>"`, version 1), `<Name>Component.h/.cpp`, `PluginProcessor`, `PluginEditor`, `PdMain.cpp`, `CMakeLists.txt`, `Presets/`.
- Add it to `_all_plugins` in the top `CMakeLists.txt`.
- Add it to its family in `Source/Common/Theme.h` (one enum entry, one accent line).
- Reusable DSP goes in FxmeTools `core/` (JUCE-free, tested), recorded in `doc/api-changes.md`.
- From the start: `fxme::sanitiseBlock` on the input, `sanitiseSample` on every recursive path, `setLogarithmic()` on frequency knobs, On switch with a fade, latency reported if any.
- Factory presets, a screenshot in `doc/`, a README entry, release notes.
- Pd external (see `doc/pdconversion.md`).
- CI: add it to the `_All` and packaging loops of the four workflows.

## Advanced algorithmic reverb: a separate project

**StairwellToHeaven** (`~/src/StairwellToHeaven`), kept out of FxmeFX, which
stays a standard collection. Its `doc/architecture.md` holds the status and
its `doc/todo.md` what remains. Not in FxmeFX's release notes.

## FxmeNeuralAmp, after v1

- [ ] CI: first macOS (universal, `-Xarch_x86_64 -march=x86-64-v3`, the plugin at 10.15) and Windows (`/arch:AVX2`, MSVC's CPU check) builds checked.
- [ ] README screenshot `doc/FxmeNeuralAmp.png` (the README links it).
- [ ] NeuralAudio: the pull request upstream from our fork's `fxme-no-audio-alloc` (`2aeff04`); once merged, pin mikeoliphant/NeuralAudio again.
- [ ] Phase 4, Otomato: one more entry in `Agents/Fxme`'s effects, `fxmefx_attach_neural()`; then Camshaft and Mechanodd if wanted. Later the Raspberry Pi (small models, NeuralAudio's static builds, quality scaling of A2 models).
- [ ] NAM Core's A2 "max" WaveNet allocates about twice per sample on its own backend (not used by the plugin); report to NAM Core.

## FxmeTape, possible additions

- A little even-harmonic asymmetry (an imperfect bias, so the 2nd harmonic appears; today the model is symmetric and has none).
- A cassette speed (1.875 ips, with its own head losses, bump and emphasis).
