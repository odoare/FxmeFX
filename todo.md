# FxmeFX: new plugins, plan

Date: 2026-11-05.

Six projects: Gate, De-esser, Crusher (all three in FxmeFX, done), the advanced reverb (a separate project, StairwellToHeaven), a tape saturation (FxmeFX, section 5, done), and a neural amp modeller reading NAM models (FxmeTools + FxmeFX, added 2026-11-09, section 6). Each is done and committed on its own.

## Checklist common to every new FxmeFX plugin

- `Source/<Name>/`, laid out like `Source/Compressor/`: `<Name>.h/.cpp` (DSP and parameters, IDs `"<prefix>_<Short>_<Name>"`, version 1), `<Name>Component.h/.cpp`, `PluginProcessor`, `PluginEditor`, `PdMain.cpp`, `CMakeLists.txt`, `Presets/`.
- Add it to `_all_plugins` in the top `CMakeLists.txt`.
- Add it to its family in `Source/Common/Theme.h` (one enum entry, one accent line).
- Reusable DSP goes in FxmeTools `core/` (JUCE-free, tested), recorded in `doc/api-changes.md`.
- From the start: `fxme::sanitiseBlock` on the input, `sanitiseSample` on every recursive path, `setLogarithmic()` on frequency knobs, On switch with a fade, latency reported if any.
- Factory presets, a screenshot in `doc/`, a README entry, release notes.
- Pd external (see `doc/pdconversion.md`).

## 1. FxmeGate (Dynamics family)

Same look as the Compressor: knobs, a gain-reduction meter, an input/output meter.

DSP, built on `fxme::DynamicGainComputer` (expand mode) where it fits:

- [x] Threshold, Range (depth of the closed gate, -80 dB to 0), Attack, Hold, Release.
- [x] Hysteresis (a close threshold a few dB under the open one) so the gate does not chatter on a decaying note.
- [x] Ratio, from a soft expander (2:1) up to a hard gate (inf). Could be a "Gate / Expander" switch plus a ratio instead.
- [x] Sidechain filter: high-pass and low-pass on the detector only (log knobs), and a "Listen" button to hear the detector signal.
- [x] Optional look-ahead (0 to 5 ms) so the attack does not cut the transient, with the latency reported.
- [x] Stereo linked detection (max of the channels, as in the Compressor).
- [x] External sidechain input (an optional aux bus, the Ext SC switch); added 2026-11-08, with the Compressor's. Not in the Pd externals or embedding hosts.
- [x] GUI: a state LED (open, hold, closed) and the threshold drawn on the input meter.

Done: the hold/hysteresis logic is `fxme::GateStateMachine` in FxmeTools core (tested), for a future multiband gate or ducker.

## 2. FxmeDeEsser (Dynamics family)

- [x] Detector: band-pass or high-pass on the sidechain (`fxme::TptSvf`), Frequency (2 to 12 kHz, log), Q or width.
- [x] Threshold, Range (maximum reduction), Attack and Release with sensible fixed defaults (a de-esser is mostly set-and-forget), maybe exposed as one "Speed" knob.
- [x] Two modes:
  - **Wideband:** the whole signal is turned down when the band triggers (classic, safe).
  - **Split band:** only the band is turned down; a dynamic bell or high shelf on the main path whose gain follows the detector (`fxme::RegaliaMitraEq` driven by the gain computer, as `BandDynamics` already does).
- [x] Relative detection option: the band level compared with the full-band level, so the threshold works at any input level.
- [x] "Listen" button to solo the detector band, which is how people find the sibilance.
- [x] Stereo linked, plus an optional M/S mode (de-ess the Mid only), reusing the Equalizer's M/S code.
- [x] GUI: a small spectrum (`SpectrumTap`) with the detector band drawn on it and its handle draggable, and a gain-reduction meter.

Checked: `BandDynamicsProcessor` is made for one band of a filter bank (its own level convention, the gain applied to the band signal), so the de-esser uses `LevelDetector` / `GainComputer` / `GainBallistics` and a `RegaliaMitraEq` directly. No new FxmeTools class was needed.

## 3. FxmeCrusher (Distortion and tone family)

`fxme::BitCrusher` and `fxme::Downsampler` already exist; the plugin is mostly wiring and sound design. Done: both gained additive options (quantizer laws, dither input, deterministic jitter), tested in FxmeTools core.

- [x] Bits (1 to 24, fractional), Rate (target sample rate or a decimation factor, log knob), Mix, Output.
- [x] Jitter on the hold length (a little randomness takes the edge off the fixed aliasing pattern), seeded through `DeterministicRandom`.
- [x] Optional anti-alias low-pass before the decimation ("Clean" versus "Dirty"), and a smoothing low-pass after it (the reconstruction filter of an old sampler).
- [x] Optional dither before the quantiser (a "noise" amount), which turns the quantisation distortion into hiss at low bit depths.
- [x] Drive into the quantiser and a DC blocker at the output.
- [x] Quantiser modes to consider: mid-tread versus mid-rise, or a mu-law curve for a telephone and old-sampler colour.
- [x] GUI: a small waveform view (`WaveformTap`) showing the steps.

## 4. Advanced algorithmic reverb (separate project)

Started 2026-11-06 as **StairwellToHeaven** (`~/src/StairwellToHeaven`): phases 0 to 6 implemented (Dattorro tank, no FDN yet), the GUI and presets written; its `doc/architecture.md` holds the plan's status and `doc/todo.md` what remains. Done (confirmed 2026-11-09): every box below is ticked, with where the result differs from the plan noted; what remains is in StairwellToHeaven's `doc/todo.md`.

Kept out of FxmeFX, which stays a standard collection. A new repository built with the FX-Mechanics project template (FetchContent on FxmeTools, `core/`/shell split).

Name candidates (to choose): FxmeHalo, FxmeAether, FxmeNebula, FxmeVerbPro. The DSP core is reusable, so it lives in FxmeTools `core/` (for example `dsp/TankReverb.h`), and the plugin project holds the parameters, modes and GUI.

### Phase 0: project and decisions

- [x] Choose the name and create the repository.
- [x] (Dattorro tank only; the FDN is StairwellToHeaven's todo 4.) Choose the topology. Recommendation: a Dattorro-style figure-eight tank as the base (it sounds good with little tuning, and plate/hall are its natural range), with tank size, diffusion and modulation set per mode. An 8-line FDN (Hadamard or Householder matrix) is the alternative, better for rooms and for a calibrated RT60 at any size. One option is to implement both behind the same interface and let the mode choose.
- [x] Fix the internal rate: run at the host rate with delay lengths scaled from 29.761 kHz (Dattorro's reference) rather than resampling.

### Phase 1: the late tank (FxmeTools core)

- [x] Input chain: band-limit (low-pass), then 2 to 4 series allpass diffusers, diffusion amount controllable.
- [x] Figure-eight loop: two branches, each with a modulated allpass, a delay, the damping filter, a decay gain, a second allpass and a second delay, each feeding the other.
- [x] Modulation: LFO (sine plus a slow random component) on the allpass delays, with Rate and Depth, the two branches in quadrature. Read through `ModDelayLine` (Catmull-Rom) or an allpass interpolator.
- [x] Output: multi-tap stereo sum from both branches (Dattorro's taps), Width.
- [x] `sanitiseSample` on the loop input and an out-of-bounds check on the loop state.
- [x] Unit tests: impulse response finite, energy decays, no NaN after a bad sample, bit-exact reset.

### Phase 2: calibrated decay

- [x] Mid RT60 in seconds: per-delay gain g = 10^(-3 d / (RT60 fs)) for a loop of total delay d, so the tail actually takes RT60 seconds to fall 60 dB.
- [x] High damping: a first-order TPT shelf or low-pass in the loop (frequency and amount), so highs decay faster (air absorption).
- [x] Bass multiplier (0.5x to 2.5x): a low shelf crossover in the loop that scales RT60 below the crossover frequency.
- [x] (Verified by T30 on impulse responses and `rt60At()` in `CoreReverbTests`; a per-octave report is StairwellToHeaven's todo 3.) Verify by simulation (Python transcription, as in FxmeFX `doc/Limiter/`): measured RT60 per octave against the setting, report in `doc/`.

### Phase 3: early reflections

- [x] Multi-tap delay, 6 to 8 taps per channel, alternating pan, each tap slightly low-passed (more for later taps).
- [x] (Fixed tables, plus a Stairwell flutter; from `RoomAcoustics` is StairwellToHeaven's todo 6.) Tap patterns per mode (room, hall, plate) scaled by a Size control; maybe derived from `RoomAcoustics.h` (image source) instead of fixed tables.
- [x] ER feeds the tank (option) or runs in parallel.
- [x] ER / Late balance control.

### Phase 4: modes

A mode is a set of topology and voicing values, not a different engine (unless both tank and FDN exist):

- [x] **Plate:** high input diffusion, fast build-up, bright, little ER, light chorus-like modulation.
- [x] **Hall:** lower diffusion at first, long delays, long decay, darker top, wide.
- [x] **Room / Ambience:** strong ER, short decay, tighter image.
- [x] **Shimmer:** Hall plus a pitch shifter in the feedback path (+12 or +7 semitones, amount knob). Use `fxme::SplicePitchShifter` (made for octave shifts), high-pass before the shifter so the bass does not build up, and a gain under unity on the shifted path to keep the loop stable.
- [x] (Fades out, clears, fades in; a true crossfade is StairwellToHeaven's todo 5.) Changing the mode crossfades (or clears the tail) to avoid clicks from jumping delay lengths.

### Phase 5: freeze

- [x] Latching Freeze: loop gain to 1.0, damping and bass scaling bypassed (otherwise the tail still decays), input to the tank muted (ER can still pass, to decide).
- [x] No runaway: the loop is lossless only if every element is unity gain, so keep the modulated allpasses (unity gain), turn the shimmer off while frozen, and add a very gentle soft limiter or energy normaliser in the loop as a safety net.
- [x] Smooth entry and exit (gain ramps over about 50 ms).

### Phase 6: pre-delay sync

- [x] Pre-delay in ms or synced (1/64 to 1/2, dotted and triplet), with `fxme::DelayTimeResolver` and the host tempo; a Sync toggle as in the StereoDelay.

### Phase 7: GUI, presets, docs

- [x] Layout in the FxmeFX style: mode selector, Decay/Size/Pre-delay/Diffusion, Damping and Bass multiplier, Modulation, ER/Late, Width, Mix and Output faders on the right, Freeze button.
- [x] A decay display (RT60 versus frequency, from the damping and bass settings) or a live tail-energy meter.
- [x] Factory presets per mode.
- [x] CPU measurement at 48 and 96 kHz.
- [x] Docs, README, release.

## 5. FxmeTape (Distortion and tone family)

Proposal, 2026-11-06; implemented the same day with the decisions at the end of this section (no wow or flutter, a tape-type menu, 2x or 4x oversampling). Measurements in `doc/Tape/`. The aim is a tape machine that sounds right when pushed, not a tanh with a low-pass: the character of tape comes from magnetic hysteresis (soft, level-dependent, with a memory, so the harmonics and the phase shift change with level and frequency), from the record/playback equalisation around it (highs saturate first), from the playback head's losses and bump, and from the transport (wow and flutter). Each of those is a stage below.

### Signal path

Record side, then the tape, then the playback side, per channel:

1. Input (Drive): the level into the record head, in dB; 0 dB puts a 0 dBFS sine at the tape's nominal "0 VU" (a few dB under where the hysteresis compresses hard).
2. Record pre-emphasis: a high shelf boost, its corner set by the speed and the EQ standard (NAB: 50 and 3180 us at 7.5 and 15 ips; IEC/CCIR: 70 us at 7.5 ips, 35 us at 15; AES: 17.5 us at 30 ips). Highs therefore hit the hysteresis harder, which is why tape compresses cymbals and esses before the rest.
3. Hysteresis, oversampled (2x by default, 4x option), reported as latency: a Jiles-Atherton magnetisation model (the physical one: Holters and Zolzer, DAFx 2016, for the model and its discretisation; Chowdhury, DAFx 2019, "Real-time physical modelling for analog tape machines", for its use on tape), solved per sample with RK4 (RK2 as a cheaper option if CPU demands it). Its parameters (saturation magnetisation, coercivity, loop width, coupling) come from three user controls:
   - Saturation: how hard the loop saturates (Ms and the drive scaling).
   - Bias: AC bias is what makes tape linear; modelled as its effect on the loop (more bias, narrower loop, cleaner and a little darker; under-bias, wider loop, gritty and "crossover"-like at low levels).
   - Tape type (choice, maybe for v2): a few parameter sets (modern high-output, vintage ferric) instead of more knobs.
4. Playback de-emphasis: the inverse of step 2, so a quiet signal comes out flat.
5. Playback head losses, depending on the speed: gap loss (a sinc-shaped high-frequency roll-off, from the gap width against the wavelength v/f), spacing loss (exp (-2 pi d f / v)) and thickness loss, approximated by a short FIR or a low-order filter fitted per speed; and the head bump, a low-frequency resonance whose frequency scales with speed (about 40 Hz at 7.5 ips, 60 Hz at 15, 100 Hz at 30), a `fxme::RegaliaMitraEq` bell with a small dip under it.
6. (Dropped: wow and flutter go to a later tape delay, see the decisions below.) Transport: wow (0.3 to 2 Hz) and flutter (6 to 15 Hz) as a modulated read on a `fxme::ModDelayLine` (Catmull-Rom), the two LFOs plus a slow random drift (filtered noise, seeded, so a bounce renders the same), the same modulation on both channels (one tape) with an optional small offset between them (azimuth). Its centre delay is a few ms, added to the reported latency.
7. Hiss (off by default): noise shaped like tape hiss (high-passed, through the playback de-emphasis), its level relative to the "0 VU" reference, and gated by nothing (tape hiss is always there; an "only while signal" option is not realistic).
8. Mix, Output, and an automatic make-up on Drive (as console tape plugins do: turning Drive up adds saturation, not mostly level), switchable.

Plus the house rules: `sanitiseBlock` on the input and `sanitiseSample` on the hysteresis state (it is recursive), an On switch that fades, presets, Pd external.

### Where the code goes

- FxmeTools core: `dsp/TapeHysteresis.h` (Jiles-Atherton, the solver, the bias and saturation mapping), JUCE-free and tested; probably `dsp/TapeEq.h` too (the emphasis time constants and the head-loss filter per speed), since a tape delay or a cassette effect would reuse both. Recorded in `doc/api-changes.md`.
- Oversampling stays on the JUCE side (`juce::dsp::Oversampling`, as the Tube does), so the hysteresis class takes the sample rate it runs at.
- FxmeFX `Source/Tape/`: the chain, the parameters, the GUI.

### Parameters (first proposal)

On, Drive (-12 to +24 dB), Saturation (0 to 100 %), Bias (0 to 100 %, 50 % = calibrated), Speed (7.5, 15, 30 ips), EQ (NAB, IEC), Wow (0 to 100 %), Flutter (0 to 100 %), Hiss (off to -40 dB), Oversampling (2x, 4x), Auto gain (on/off), Mix, Output.

### GUI

- The knobs above, Speed and EQ as menus, Mix and Output as faders (house layout).
- A small XY display of the hysteresis loop (H against M over the last block): it shows at a glance how hard the tape is driven and what Bias does to it. Needs a lock-free snapshot like `WaveformTap` (two taps, or a small XY tap in FxmeTools if none fits).
- Input and output level meters (VU ballistics, as tape machines have), from FxmeTools.

### Verification

- Core tests: the model's output finite and bounded for any input (NaN, +-inf, huge values), odd symmetry, the loop's area growing with drive and shrinking with bias, a quiet sine coming out nearly linear, reset() bit-exact.
- A Python transcription of the chain (as `doc/Limiter/`), in `doc/Tape/`: frequency response per speed and EQ at a low level (flat within a dB or two, except the head bump and the top end), THD and the third-to-second harmonic ratio against level, and the compression curve at 1 kHz and 10 kHz (the highs should compress first). Results checked against published measurements of real machines where they exist.
- CPU at 48 and 96 kHz with 2x and 4x oversampling.

### Commits

1. FxmeTools: `TapeHysteresis` (and `TapeEq`) with their tests.
2. FxmeFX: the plugin, presets (e.g. "15 ips Glue", "Hot 7.5", "Worn Cassette-ish" with wow and hiss), README, release notes, the simulation in `doc/Tape/`.

### To decide before starting

- Name: FxmeTape, or something else.
> OK
- Tape type menu in v1, or Saturation and Bias only.
> Everything in V1
- Default oversampling (2x is a fair compromise for the JA model; 4x is cleaner at high drive).
> Both, 2x by default
- Whether wow and flutter belong in this plugin at all (they add latency and are a transport effect, not a saturation one), or default to 0 here and live mostly in a later tape delay.
> This is a saturation/colouration plugin, no WOW nor flutter. Indeed these will be in a tape delay later.

## 6. Neural amp modeller (FxmeTools + FxmeFX), reading NAM models

**FxmeNeuralAmp finished, 2026-11-10** (phases 0 to 3; tested in a DAW and through FxmeNet with a local server). What remains is listed under "After v1" below. Proposal, 2026-11-09; decisions taken the same day (see "Decided" at the end of this section). Plays `.nam` amp and pedal models (Steven Atkinson's Neural Amp Modeler format, the ones on TONE3000 / ToneHunt), as an FxmeFX effect, so that every host of FxmeFX effects gets it too: its own plugin, Otomato (`Agents/Fxme` turns an FxmeFX effect class into an agent from a short description), Camshaft, Mechanodd. The shared library side is designed in FxmeTools' `doc/neural-plan.md`; this section is the project.

### What was challenged, and decided in principle

- **Not in FxmeCore.** FxmeCore stays plain C++17 without dependencies (every Pd external and test links it). The neural engine brings Eigen, a JSON parser and a third-party library: it goes in a separate, optional, JUCE-free target of the FxmeTools repo, `FxmeNeural`, linked only by those who ask (as WDL is today: `fxmetools_attach`).
- **Not a 1:1 wrapper of NAM's API**, which is already small (load, then `process (in, out, n)`, mono). What FxmeTools adds is what every consumer would otherwise write again: loading off the audio thread and swapping models without a click, resampling to the model's rate (models are trained at 48 kHz), input and output calibration, and the model kept in the session and in presets.
- **Engine: NeuralAudio (Mike Oliphant, MIT) behind our own interface,** NAM Core (MIT) as its optional fallback. NeuralAudio is designed to give exactly NAM Core's output, faster and lighter; reads A1 and A2 `.nam` models (WaveNet, LSTM) and Keras models (through RTNeural); has quality scaling for A2 slimmable models and SIMD settings for the Raspberry Pi. The neural-amp-modeler-lv2 plugin and Pi Pedal use it. Behind our interface it can be swapped.
- **An FxmeFX effect class, not only a plugin**: that is what makes it free for Otomato and the other hosts.
- **Its name: FxmeNeuralAmp**, so it reads as an implementation of NAM; the description says it plays NAM models (Neural Amp Modeler is Steven Atkinson's project, credited as such).
- **No third-party models shipped.** Models belong to their authors; the user loads them. Factory presets only with models we train ourselves (NAM's trainer, our own amps and pedals), if any.
- Licences: NeuralAudio and NAM Core MIT, RTNeural BSD-3, Eigen MPL-2: compatible with LGPL core and AGPL plugins. Keep their notices (README, a `CREDITS` file).

### The effect (first proposal)

Mono in the model (an amp is mono). Per instance:

1. Input: level in dBu (the calibration the model expects; NeuralAudio's `SetAudioInputLevelDBu`, its recommended input adjustment), then a noise gate before the model (`fxme::GateStateMachine`, as FxmeGate: a high-gain model amplifies the noise floor).
2. The model (`fxme::neural::NeuralAmpEngine`), at the model's rate, resampled when the host is not (44.1 kHz, or a rate the engine cannot run at), its recommended output adjustment applied.
3. A tone stack after it: bass, mid, treble (three `RegaliaMitraEq`/`Biquad` sections, as amp tone stacks are placed in NAM's own plugin).
4. No cabinet in the effect: FxmeCab is chained after it (in a host's chain, Otomato's strip, Camshaft's slots). The effect stays one model, and the cabinet keeps its own presets and its 19 embedded IRs.
5. Output level, Mix, On (with a fade, as every FxmeFX effect).

Channels: **Dual mono** (one model per side: a stereo source keeps its image, twice the CPU) or **Mono** (one model on the sum of the two sides, the result on both: a guitar amp, half the CPU), a choice parameter. Both in v1; a mono input or a host's mono strip runs one model whatever the setting.

House rules: `sanitiseBlock` on the input, latency reported if the resampler adds some, the model kept in the state and in module presets (`ModulePresetTarget::onWriteExtra / onReadExtra`; a `.nam` file is a few hundred kB of JSON), FxmeNet aware (a model change is `notifyStateChanged()`; the mirror never loads models, it shows the name and metadata from the state), the shell, tooltips, presets, docs.

### GUI

- The model's name, its metadata (architecture, the rate it was trained at, its loudness, its author if given), a load button and a browser of a models folder (`FX-Mechanics/Models/NAM`), drag and drop of a `.nam` file.
- Knobs: Input (dBu), Gate threshold, Bass, Mid, Treble, Output, Mix; the Channels choice (Dual mono, Mono); input and output meters; the gate's state LED (as FxmeGate's).
- A CPU hint for the loaded model (its architecture: Nano, Feather, Lite, Standard), since it is the heaviest effect of the catalogue.

### Phases and commits

0. **Measure.** Done 2026-11-09 (FxmeTools `doc/neural-phase0/`: harness, raw results, analysis). NeuralAudio's internal engine gives NAM Core's output exactly for WaveNets, 5 to 6 x faster for nano, 2 to 2.5 x for feather, level for standard and A2 here; LSTMs go through NAM Core (same speed, exact). A standard WaveNet is about 6 % of one core per mono instance at 48 kHz. **The model always runs at its own rate (48 kHz) and our resampler converts:** NeuralAudio's own handling of other rates is 2 to 17 x the CPU at 96 kHz, approximate, and wrong at 44.1 kHz and for LSTMs. Loading from memory works (`CreateFromStream`). The Raspberry Pi later (see Decided).
1. **FxmeTools, core:** `dsp/Resampler.h`. Done 2026-11-09: `fxme::Resampler` (one direction, any ratio) and `fxme::ResamplingContainer` (the round trip around a fixed-rate process, exact integer latency), polyphase windowed sinc, vectorised; `CoreResamplerTests` (round trip error -100 dB at every host rate from 44.1 to 192 kHz, flat to 20 kHz at 44.1 kHz, aliasing under -100 dB, any block sizes, bit-exact reset); 0.3 to 0.55 % of a core at 44.1 kHz, 1 to 2 % at 192 kHz; latency about 3 ms (139 samples at 44.1 kHz, 275 at 96 kHz); `doc/api-changes.md`.
2. **FxmeTools, FxmeNeural:** Done 2026-11-09 (FxmeTools `neural/`, `doc/neural-plan.md` "Phase 2 outcome"): `ModelData`, `ModelLoader` (`PreparedModel`, at the host's rate, the network at its own inside the resampler), `NeuralAmpEngine` (lock-free install, pre-roll + 20 ms crossfade, retired models freed off the audio thread, calibration, NaN-proof), `fxmetools_attach_neural()`, `FXME_NEURAL_ARCH`. `CoreNeuralTests` on NAM Core's MIT example models: identical to NAM Core, the same response at 44.1 / 48 / 96 kHz (0.01 dB), no click on a swap, no allocation or free in `process()`, a bench (standard WaveNet 5.5 to 6 % of a core with AVX2, 10 to 11 % without). Found on the way: LSTMs reach NAM Core only through NeuralAudio's WaveNet mode; NAM Core's registrations need anchoring in a static library; **NeuralAudio's WaveNets allocated on every block**: fixed in our fork (`2aeff04`), to go upstream. The fix lives in our fork, https://github.com/odoare/NeuralAudio (branch `fxme-no-audio-alloc`), pinned by every consumer; the pull request upstream after FxmeNeuralAmp v1 (decided 2026-11-10). Released plugins built for `x86-64-v3` (decided 2026-11-10).
3. **FxmeFX:** Done 2026-11-10. `Source/NeuralAmp/`: the effect class `NeuralAmp` (Input, Gate before the model with its LED, the model in two engines so Channels switches at once (Mono, Dual mono, with a dip and a pre-roll), Bass / Mid / Treble after it, Mix against the delayed dry, Output; Level / Calibrate / Normalize; On fades and stops the models, pre-roll on the way back; a loader thread), `NeuralAmpComponent` (standard and compact; the model panel with metadata, CPU class and status; the browser of `FX-Mechanics/Models/NAM`; Load, Clear, drag and drop), the plugin (`NAMP`, VST3 / CLAP / AU), four settings-only factory presets; `cmake/FxmeNeural.cmake` (`fxmefx_attach_neural()`, `FXME_NEURAL_ARCH` x86-64-v3); NeuralAudio pinned (our fork); the model in the state (FxmeTools' new `fxme::EmbeddedData`, about 40 % of the file) and in module presets' `<Extra>` (a preset without one keeps the model; Level and Calibrate left out of presets); FxmeNet: a mirror never builds networks, a model change calls `notifyStateChanged()`; CI on three OSes and the release (no Pd); README section, LICENSE.md, release notes. Checked offline with the plugin's own processor (a scratch harness, not committed): exact latency and dry path at 44.1 / 48 / 96 kHz, a model loads while processing, a non-model refused, a restored session bit-identical to loading the file, a preset carries the model, Mono = Dual mono on a mono source, Off and Mix 0 exactly the delayed dry, no click switching On / Channels, the gate closed under its threshold, Clear empties the state. After the first listening test (2026-11-10): Level greyed out when it cannot act (Calibrate off, or a model without `input_level_dbu`: checked, 11.5 dB of change on NAM Core's A2.nam, none on a model without it); the knobs on two rows of three (drive side, tone stack), the browser under the model panel, the window 580 x 464; FxmeNet checked end to end in the harness (a mirror never builds the model, its change counted, the state carried to a source instance that plays it bit-identically to a local one) and the server's model state now carried to the mirror by telemetry (`model`: none, loading, playing, failed), shown in the panel. Dropping a file froze the file manager on Linux (the load ran inside JUCE's drop, before its unflushed XdndFinished reply left): the drop now ends first and the model loads 50 ms later. **Not yet:** macOS and Windows builds (CI), a screenshot for the README. No Pd external (loading a model needs a message the Pd host does not have).
4. **Otomato:** one more entry in `Agents/Fxme`'s effects, `fxmefx_attach_neural()`; then Camshaft and Mechanodd if wanted.

### After v1

- [ ] FxmeFX: bump the FxmeTools pin in `cmake/Dependencies.cmake` once FxmeTools' `neural/` and `EmbeddedData` are committed (CI builds from the pins).
- [ ] CI: first macOS (universal, `-Xarch_x86_64 -march=x86-64-v3`, the plugin at 10.15) and Windows (`/arch:AVX2`, MSVC's CPU check) builds.
- [ ] README screenshot `doc/FxmeNeuralAmp.png`.
- [ ] NeuralAudio: the pull request upstream from our fork's `fxme-no-audio-alloc` (`2aeff04`); back to mikeoliphant/NeuralAudio once merged.
- [ ] Phase 4, Otomato (above); later the Raspberry Pi (small models, NeuralAudio's static builds, quality scaling of A2 models).

### Verification

- Engine: the golden test against NAM Core (sample-accurate to a tolerance), a model swap under a running signal without a click, the rate conversion's response (flat to 20 kHz at 44.1 kHz, aliasing rejection), no allocation on the audio thread (load and swap tested from another thread), NaN and infinity at the input.
- Effect: Mix 0 and On off are the dry signal; a session and a preset reload the same model with no file on disk; Mono and Dual mono give the same sound on a mono source; CPU per model architecture at 48 and 96 kHz, Mono and Dual mono, recorded in `doc/`.

### Decided (2026-11-09)

- Backend: NeuralAudio, NAM Core as its optional fallback.
- No IR slot: FxmeCab is chained after the effect.
- The Raspberry Pi (Otomato) is for later: v1 targets desktops; the small models, NeuralAudio's static builds and its quality scaling wait until then (nothing in the design prevents them).
- Name: FxmeNeuralAmp, so it reads as an implementation of NAM.
- Channels in v1: Dual mono (one model per side) and Mono (one model on the sum), a choice parameter.

## Open questions for later

Sections 1 to 5 are done (2026-11-09: Gate, De-esser, Crusher, StairwellToHeaven, Tape; and the side-chain inputs of FxmeGate and FxmeCompressor, added after). Still open:

- Tape, two possible additions: a little even-harmonic asymmetry (an imperfect bias, so the 2nd harmonic appears; today the model is symmetric and has none), and a cassette speed (1.875 ips, with its own head losses, bump and emphasis).
- Neural amp: section 6, decided, to start.
