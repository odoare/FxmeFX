#!/usr/bin/env python3
"""
FxmeLimiter: simulation of the sample and True Peak modes.

A numpy transcription of Limiter::process (sample mode) and
Limiter::processTruePeak (True Peak mode), Source/Limiter/Limiter.cpp, run on
test signals at several drives. For each case it reports the output's sample
peak, its true peak as the plugin itself reads it (4x, the same interpolator)
and as a finer reference reads it (16x polyphase resampling, scipy).

Writes truepeak_results.csv next to this script and prints a Markdown table.
Mono signals: the limiter links its channels, so one channel shows it all.

Usage: python3 truepeak_sim.py
Needs numpy and scipy.
"""

import csv
import os
import numpy as np
from scipy.ndimage import maximum_filter1d
from scipy.signal import resample_poly, lfilter

FS = 48000
SECONDS = 1.0
CEILING_DB = -1.0
RELEASE_MS = 50.0
LOOKAHEAD_MS = 2.0
SKIP = 2000          # samples left out of the measures (filter start-up)

# --- The plugin's 4x interpolator (Limiter::Limiter) -----------------------
PHASES, TAPS, TP_DELAY = 4, 13, 6


def interpolator():
    length = PHASES * (TAPS - 1) + 1          # 49
    centre = (length - 1) / 2.0               # 24
    beta = 7.0
    i = np.arange(length)
    t = (i - centre) / PHASES
    r = (i - centre) / centre
    h = np.sinc(t) * np.i0(beta * np.sqrt(np.clip(1.0 - r * r, 0.0, None))) / np.i0(beta)
    co = np.zeros((PHASES, TAPS))
    for k in range(PHASES):
        for j in range(TAPS):
            if PHASES * j + k < length:
                co[k, j] = h[PHASES * j + k]
        co[k] /= co[k].sum()
    return co


CO = interpolator()


def peak4x(x):
    """Per sample n: the largest |value| of the four phases, i.e. the true
    peak over [n - 6, n - 5) as the plugin reads it."""
    return np.max(np.abs(np.stack([lfilter(CO[k], [1.0], x) for k in range(PHASES)])), axis=0)


def peak16x(x):
    return np.max(np.abs(resample_poly(x[SKIP:], 16, 1)))


def db(v):
    return 20.0 * np.log10(max(v, 1e-12))


# --- Sample mode (Limiter::process) ----------------------------------------
def limit_sample_mode(x, ceiling):
    L = max(1, round(LOOKAHEAD_MS * 0.001 * FS))
    attack_ms = max(0.05, LOOKAHEAD_MS * 0.5)
    att = np.exp(-1.0 / (attack_ms * 0.001 * FS))
    rel = np.exp(-1.0 / (RELEASE_MS * 0.001 * FS))
    env = 1.0
    gains = np.empty_like(x)
    for n, s in enumerate(np.abs(x)):
        target = ceiling / s if s > ceiling else 1.0
        env = att * env + (1 - att) * target if target < env else rel * env + (1 - rel) * target
        gains[n] = env
    delayed = np.concatenate([np.zeros(L), x[:-L]])
    return np.clip(delayed * gains, -ceiling, ceiling)


# --- True Peak mode (Limiter::processTruePeak) -----------------------------
def limit_true_peak(x, ceiling):
    L = max(1, round(LOOKAHEAD_MS * 0.001 * FS))
    hold = L + 2
    rel = np.exp(-1.0 / (RELEASE_MS * 0.001 * FS))

    tp = peak4x(x)
    # Held over the last `hold` samples (window ending at n).
    held = maximum_filter1d(tp, size=hold, origin=(hold - 1) // 2, mode="constant", cval=0.0)
    target = np.where(held > ceiling, ceiling / np.maximum(held, 1e-12), 1.0)

    r = np.empty_like(x)
    env = 1.0
    for n, t in enumerate(target):
        env = t if t < env else rel * env + (1 - rel) * t
        r[n] = env
    # Moving average over the look-ahead (the ring starts full of 1).
    padded = np.concatenate([np.ones(L - 1), r])
    gain = np.minimum(1.0, np.convolve(padded, np.ones(L) / L, mode="valid"))

    D = L + TP_DELAY
    delayed = np.concatenate([np.zeros(D), x[:-D]])
    return np.clip(delayed * gain, -ceiling, ceiling)


# --- Test signals -----------------------------------------------------------
def signals():
    rng = np.random.default_rng(1)
    n = int(FS * SECONDS)
    t = np.arange(n) / FS
    white = rng.standard_normal(n)
    # Pink-ish noise (Voss-McCartney approximation by a 3-pole filter), a
    # stand-in for a mix.
    pink = lfilter([0.049922035, -0.095993537, 0.050612699, -0.004408786],
                   [1, -2.494956002, 2.017265875, -0.522189400], white)
    pink *= 0.25 / np.sqrt(np.mean(pink ** 2))
    return {
        "sine fs/4, 45 deg": 0.9 * np.sin(2 * np.pi * 12000 * t + np.pi / 4),
        "sine 11025 Hz": 0.9 * np.sin(2 * np.pi * 11025 * t + 0.785),
        "square 997 Hz + 15 kHz": 0.8 * np.sign(np.sin(2 * np.pi * 997 * t)) + 0.2 * np.sin(2 * np.pi * 15000 * t),
        "gated white noise": 0.3 * white * (np.sin(2 * np.pi * 3 * t) > 0),
        "pink noise": pink,
    }


def main():
    ceiling = 10 ** (CEILING_DB / 20)
    rows = []
    for name, sig in signals().items():
        for drive_db in (0.0, 6.0, 12.0):
            x = sig * 10 ** (drive_db / 20)
            sm = limit_sample_mode(x, ceiling)
            tp = limit_true_peak(x, ceiling)
            rows.append({
                "signal": name,
                "drive_db": drive_db,
                "input_tp16_db": round(db(peak16x(x)), 2),
                "sample_mode_tp16_db": round(db(peak16x(sm)), 2),
                "tp_mode_sample_peak_db": round(db(np.max(np.abs(tp[SKIP:]))), 2),
                "tp_mode_tp4_db": round(db(np.max(peak4x(tp)[SKIP:])), 2),
                "tp_mode_tp16_db": round(db(peak16x(tp)), 2),
            })

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "truepeak_results.csv")
    with open(out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    cols = list(rows[0].keys())
    print("| " + " | ".join(cols) + " |")
    print("|" + "---|" * len(cols))
    for r in rows:
        print("| " + " | ".join(str(r[c]) for c in cols) + " |")


if __name__ == "__main__":
    main()
