# FxmeLimiter True Peak mode: simulation tests

Date: 2026-11-05.

## What was tested

The C++ code was not run. `truepeak_sim.py` is a numpy transcription of the two limiter paths in `Source/Limiter/Limiter.cpp`: sample mode (`Limiter::process`, unchanged) and True Peak mode (`Limiter::processTruePeak`). It uses the same 4x interpolator coefficients, the same look-ahead (2 ms, 96 samples at 48 kHz), the same hold span (look-ahead + 2), and the same release and moving average. The vectorised True Peak function was checked against a sample-by-sample transcription of the C++ loop; the two differ by at most 2e-14.

Settings: 48 kHz, ceiling -1 dB, release 50 ms, mono, 1 s per signal, the first 2000 samples left out of the measures. Each signal was run at drives of 0, 6 and 12 dB.

For each case, `truepeak_results.csv` has these columns:

- `input_tp16_db`: the driven input's true peak.
- `sample_mode_tp16_db`: the true peak of the output in sample mode, for comparison.
- `tp_mode_sample_peak_db`: the largest sample of the output in True Peak mode.
- `tp_mode_tp4_db`: that output's true peak read the plugin's way (the same 4x interpolator, the method of ITU-R BS.1770).
- `tp_mode_tp16_db`: that output's true peak read with 16x resampling (scipy `resample_poly`), a closer estimate of what a DAC rebuilds.

## Results

| signal | drive (dB) | input TP 16x | sample mode TP 16x | TP mode sample peak | TP mode TP 4x | TP mode TP 16x |
|---|---|---|---|---|---|---|
| sine fs/4, 45° | 0 | -0.80 | -0.80 | -4.01 | -1.00 | -0.88 |
| sine fs/4, 45° | 6 | 5.20 | 2.13 | -4.01 | -1.00 | -0.88 |
| sine fs/4, 45° | 12 | 11.20 | 2.13 | -4.01 | -1.00 | -0.88 |
| sine 11025 Hz | 0 | -0.59 | -0.31 | -1.00 | -1.00 | -0.92 |
| sine 11025 Hz | 6 | 5.41 | 0.18 | -1.00 | -1.00 | -0.92 |
| sine 11025 Hz | 12 | 11.41 | 0.46 | -1.00 | -1.00 | -0.92 |
| square 997 Hz + 15 kHz | 0 | 1.73 | 0.91 | -2.13 | -1.00 | -0.79 |
| square 997 Hz + 15 kHz | 6 | 7.73 | 1.08 | -2.13 | -1.00 | -0.79 |
| square 997 Hz + 15 kHz | 12 | 13.73 | 1.08 | -2.13 | -1.00 | -0.79 |
| gated white noise | 0 | 2.06 | 1.06 | -1.00 | -1.00 | -0.41 |
| gated white noise | 6 | 8.06 | 3.11 | -1.00 | -1.00 | -0.39 |
| gated white noise | 12 | 14.06 | 3.78 | -1.00 | -1.00 | -0.39 |
| pink noise | 0 | 1.29 | -0.44 | -1.00 | -1.00 | -0.75 |
| pink noise | 6 | 7.29 | 1.37 | -1.00 | -1.00 | -0.57 |
| pink noise | 12 | 13.29 | 1.95 | -1.00 | -1.00 | -0.57 |

## Findings

**The timing is right.** Measured the plugin's own way (4x), the True Peak output never goes above the ceiling, on every signal and at every drive. The gain is therefore fully down for the samples around each peak when that peak comes out.

**Sample mode lets inter-sample peaks through.** Its output stays at -1 dB in samples, but its true peak reaches +2.1 dB on the fs/4 sine and up to +3.8 dB on the noise. A DAC would clip those. True Peak mode removes 3 to 4 dB of overshoot in these cases.

**True Peak mode pulls samples down further than sample mode does.** On the fs/4 sine at 45° the samples fall 3 dB below the true peak, so the output samples sit at -4 dB for a -1 dB true peak. That is the expected behaviour, not a loss.

**There is a residue above the ceiling at 16x.** It comes from the 4x reading, not from the limiter's timing:

- **Clean, narrow-band tones:** 0.1 dB at most.
- **Square wave with a 15 kHz partial:** 0.2 dB.
- **Pink noise, standing in for a mix:** 0.25 to 0.45 dB.
- **White noise:** 0.6 dB. This is the worst case, because it is loud right up to Nyquist, where a 4x reading falls between the true maxima.

BS.1770 itself warns that a 4x meter can read low by up to about 0.5 to 0.7 dB on such content. A meter of that kind would show the output at -1.0 dBTP. A meter that oversamples more, or the DAC itself, may see up to about 0.6 dB more on very bright material.

**In practice**, a -1 dB ceiling keeps real material under 0 dBTP with margin, which is what streaming loudness guidelines ask for. If a tighter guarantee is ever wanted, the options are 8x detection (twice the detection cost) or an automatic margin of about 0.5 dB in True Peak mode.

## Limits of this test

- The interpolator is the plugin's own (a Kaiser-windowed sinc, β = 7, 49 taps), not the example filter printed in BS.1770. A meter built on that example filter would read slightly differently.
- The test signals are synthetic. It would be worth checking a real bright master in a DAW with a separate true-peak meter.
- The simulation covers one channel. The plugin links channels by taking the louder one, which this does not change.

## Reproducing

From this folder: `python3 truepeak_sim.py` (needs numpy and scipy; about 2 s). It rewrites `truepeak_results.csv` and prints the table.
