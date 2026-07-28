# Hecate

A metal guitar amp suite plugin (AU + VST3) built with JUCE 8 and C++17.

![Hecate](Assets/background.png)

## Signal chain

```
Gate → Octaver → Amp (tight / boost / gain / tone, 2x oversampled)
→ Compressor → 4-Band EQ → Cabinet IR → Chorus → Delay → Reverb → Output
```

- **Octaver** — pedal-style octave-down with independent **Direct** (guitar) and
  **Octave** (sub) level knobs: turn Direct off to play the sub-octave voice alone
- **Noise Gate** — keyed off the raw input, 10:1, 1 ms attack / 60 ms release,
  switchable, with an activity LED
- **Amp** — cascaded three-stage tanh preamp with interstage filtering and slight
  asymmetry, +12..+60 dB of gain, run at 2x oversampling. **Tight** sweeps the
  pre-drive high-pass 40–300 Hz; **Boost** adds a screamer-style mid hump (+8 dB
  at 750 Hz) plus 6 dB of level; **Tone** is a 2nd-order low-pass sweep
- **Compressor** — switchable, stereo-linked feed-forward with a gain-reduction meter
- **EQ** — bass low shelf (100 Hz), mid bell (1 kHz), presence bell (4 kHz), treble
  high shelf (8 kHz)
- **Cabinet IR** — ships with a built-in synthesised 4x12 so it sounds finished out
  of the box; load any WAV/AIFF/FLAC IR to replace it
- **Chorus** — post-cab modulation (rate / depth / mix)
- **Delay** — stereo, 50–1000 ms free or tempo-synced (1/4, 1/8., 1/8, 1/8T, 1/16),
  damped feedback, smoothed time changes
- **Reverb** — Freeverb-style true stereo with width, damping and pre-delay

## Cabinet IR + mic placement browser

The CAB tab loads impulse responses and understands mic-organised packs: if the
loaded IR lives in a layout like `<pack>/<mic>/<position>.wav` (for example the
free [overdriven.fr](https://overdriven.fr) sets), the **Mic** and **Position**
menus step through the pack, keeping the position when you switch mics. The IR
path is saved with the session, with a fallback search in `~/Documents/Hecate/IRs`
if the file moves.

Third-party IR files are not included in this repository — most IR licenses
(including overdriven.fr's) forbid redistributing them with software. Download
packs yourself and keep them in `~/Documents/Hecate/IRs`.

## UI

Three tabs — **AMP** (octave, gate, amp, EQ, dynamics, output), **FX** (chorus,
delay, reverb) and **CAB** (IR loader + mic browser) — laid over the artwork at
its native aspect ratio, with controls set into the lower half. The header keeps
the preset menu and slim output / gain-reduction / gate meters visible on every
tab. The window resizes 50–200% (size remembered), knobs reset on double-click,
and values pop up while dragging.

## Presets

Seven factory presets (Modern Rhythm, Djent, Doom, Solo Lead, Clean Shimmer,
Ambient Swells + Default) exposed as host programs and in the in-plugin preset
menu. User presets save as XML to `~/Documents/Hecate/Presets`.

## Building

Requires CMake 3.22+ and a C++17 compiler. JUCE is fetched automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On macOS this builds and installs AU + VST3 to `~/Library/Audio/Plug-Ins/`
(run `auval -v aufx Hct1 Hcte` to re-validate after changes); on Linux/Windows
it builds VST3.
