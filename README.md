# Hecate

A metal guitar amp suite plugin (AU + VST3) built with JUCE 8 and C++17.

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
  asymmetry, +6..+48 dB of gain, run at 2x oversampling. **Tight** sweeps the
  pre-drive high-pass 40–300 Hz; **Boost** adds a screamer-style mid hump (+8 dB
  at 750 Hz) plus 6 dB of level; **Tone** is a 2nd-order low-pass sweep
- **Compressor** — switchable, stereo-linked feed-forward with a gain-reduction meter
- **EQ** — bass low shelf (100 Hz), mid bell (1 kHz), presence bell (4 kHz), treble
  high shelf (8 kHz)
- **Cabinet IR** — ships with a built-in synthesised 4x12 so it sounds finished out
  of the box; load any WAV/AIFF/FLAC IR to replace it (path saved with the session,
  with a fallback search in `~/Documents/Hecate/IRs`)
- **Chorus** — post-cab modulation (rate / depth / mix)
- **Delay** — stereo, 50–1000 ms free or tempo-synced (1/4, 1/8., 1/8, 1/8T, 1/16),
  damped feedback, smoothed time changes
- **Reverb** — Freeverb-style true stereo with width, damping and pre-delay

## Presets

Seven factory presets (Modern Rhythm, Djent, Doom, Solo Lead, Clean Shimmer,
Ambient Swells + Default) exposed as host programs and in the in-plugin preset
menu. User presets save as XML to `~/Documents/Hecate/Presets`.

## UI

Resizable (50%–200%, size remembered per session). Double-click any knob to
reset it; values pop up while dragging. Output, gain-reduction and gate meters
live in the METERS panel.

## Building

Requires CMake 3.22+ and a C++17 compiler. JUCE is fetched automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On macOS this builds and installs AU + VST3 to `~/Library/Audio/Plug-Ins/`;
on Linux/Windows it builds VST3.
