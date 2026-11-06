# FxmeFX: new plugins, plan

Date: 2026-11-05.

Four projects, in the suggested order: Gate, De-esser, Crusher (all three in FxmeFX), then the advanced reverb (a separate project). Each is done and committed on its own.

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
- [ ] Optional external sidechain input (bus layout); left out of v1 (keeps the Pd external and the embedding hosts simple).
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

Kept out of FxmeFX, which stays a standard collection. A new repository built with the FX-Mechanics project template (FetchContent on FxmeTools, `core/`/shell split).

Name candidates (to choose): FxmeHalo, FxmeAether, FxmeNebula, FxmeVerbPro. The DSP core is reusable, so it lives in FxmeTools `core/` (for example `dsp/TankReverb.h`), and the plugin project holds the parameters, modes and GUI.

### Phase 0: project and decisions

- [ ] Choose the name and create the repository.
- [ ] Choose the topology. Recommendation: a Dattorro-style figure-eight tank as the base (it sounds good with little tuning, and plate/hall are its natural range), with tank size, diffusion and modulation set per mode. An 8-line FDN (Hadamard or Householder matrix) is the alternative, better for rooms and for a calibrated RT60 at any size. One option is to implement both behind the same interface and let the mode choose.
- [ ] Fix the internal rate: run at the host rate with delay lengths scaled from 29.761 kHz (Dattorro's reference) rather than resampling.

### Phase 1: the late tank (FxmeTools core)

- [ ] Input chain: band-limit (low-pass), then 2 to 4 series allpass diffusers, diffusion amount controllable.
- [ ] Figure-eight loop: two branches, each with a modulated allpass, a delay, the damping filter, a decay gain, a second allpass and a second delay, each feeding the other.
- [ ] Modulation: LFO (sine plus a slow random component) on the allpass delays, with Rate and Depth, the two branches in quadrature. Read through `ModDelayLine` (Catmull-Rom) or an allpass interpolator.
- [ ] Output: multi-tap stereo sum from both branches (Dattorro's taps), Width.
- [ ] `sanitiseSample` on the loop input and an out-of-bounds check on the loop state.
- [ ] Unit tests: impulse response finite, energy decays, no NaN after a bad sample, bit-exact reset.

### Phase 2: calibrated decay

- [ ] Mid RT60 in seconds: per-delay gain g = 10^(-3 d / (RT60 fs)) for a loop of total delay d, so the tail actually takes RT60 seconds to fall 60 dB.
- [ ] High damping: a first-order TPT shelf or low-pass in the loop (frequency and amount), so highs decay faster (air absorption).
- [ ] Bass multiplier (0.5x to 2.5x): a low shelf crossover in the loop that scales RT60 below the crossover frequency.
- [ ] Verify by simulation (Python transcription, as in FxmeFX `doc/Limiter/`): measured RT60 per octave against the setting, report in `doc/`.

### Phase 3: early reflections

- [ ] Multi-tap delay, 6 to 8 taps per channel, alternating pan, each tap slightly low-passed (more for later taps).
- [ ] Tap patterns per mode (room, hall, plate) scaled by a Size control; maybe derived from `RoomAcoustics.h` (image source) instead of fixed tables.
- [ ] ER feeds the tank (option) or runs in parallel.
- [ ] ER / Late balance control.

### Phase 4: modes

A mode is a set of topology and voicing values, not a different engine (unless both tank and FDN exist):

- [ ] **Plate:** high input diffusion, fast build-up, bright, little ER, light chorus-like modulation.
- [ ] **Hall:** lower diffusion at first, long delays, long decay, darker top, wide.
- [ ] **Room / Ambience:** strong ER, short decay, tighter image.
- [ ] **Shimmer:** Hall plus a pitch shifter in the feedback path (+12 or +7 semitones, amount knob). Use `fxme::SplicePitchShifter` (made for octave shifts), high-pass before the shifter so the bass does not build up, and a gain under unity on the shifted path to keep the loop stable.
- [ ] Changing the mode crossfades (or clears the tail) to avoid clicks from jumping delay lengths.

### Phase 5: freeze

- [ ] Latching Freeze: loop gain to 1.0, damping and bass scaling bypassed (otherwise the tail still decays), input to the tank muted (ER can still pass, to decide).
- [ ] No runaway: the loop is lossless only if every element is unity gain, so keep the modulated allpasses (unity gain), turn the shimmer off while frozen, and add a very gentle soft limiter or energy normaliser in the loop as a safety net.
- [ ] Smooth entry and exit (gain ramps over about 50 ms).

### Phase 6: pre-delay sync

- [ ] Pre-delay in ms or synced (1/64 to 1/2, dotted and triplet), with `fxme::DelayTimeResolver` and the host tempo; a Sync toggle as in the StereoDelay.

### Phase 7: GUI, presets, docs

- [ ] Layout in the FxmeFX style: mode selector, Decay/Size/Pre-delay/Diffusion, Damping and Bass multiplier, Modulation, ER/Late, Width, Mix and Output faders on the right, Freeze button.
- [ ] A decay display (RT60 versus frequency, from the damping and bass settings) or a live tail-energy meter.
- [ ] Factory presets per mode.
- [ ] CPU measurement at 48 and 96 kHz.
- [ ] Docs, README, release.

## Open questions for later

- Gate: external sidechain in v1 or not.
- Crusher: named FxmeCrusher.
- Reverb: name; tank only, or tank plus FDN.
