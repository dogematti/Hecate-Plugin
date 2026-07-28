# Hecate

A metal guitar amp suite plugin (AU + VST3) built with JUCE 8 and C++17.

## Signal chain

```
Input Gain → Octaver → Noise Gate → Saturator (2x oversampled) → Compressor
→ 4-Band EQ → Cabinet IR → Chorus → Delay → Reverb → Output Gain
```

- **Octaver** — pedal-style sub-octave: dual-tap crossfaded delay-line pitch shifter with
  a low-passed wet signal, blended by the Octave knob
- **Noise Gate** — 10:1 downward expander, 1 ms attack / 60 ms release; threshold on the front panel
- **Saturator** — fixed 80 Hz tightening high-pass into a tanh waveshaper (up to +36 dB of drive)
  run at 2x oversampling to suppress aliasing, with a one-pole low-pass tone control
- **Compressor** — feed-forward, stereo-linked peak detection (5 ms attack / 100 ms release)
- **EQ** — bass low shelf (100 Hz), mid bell (1 kHz), presence bell (4 kHz), treble high shelf (8 kHz)
- **Cabinet IR** — convolution loader for WAV/AIFF/FLAC impulse responses; the IR path is
  saved in the plugin state and restored with the session
- **Chorus** — post-cab modulation (rate / depth / mix)
- **Delay** — stereo delay, 50–1000 ms with smoothed time changes, feedback and mix
- **Reverb** — Freeverb-style: 8 damped combs + 4 allpasses per channel, true stereo with
  width control

## Building

Requires CMake 3.22+ and a C++17 compiler. JUCE is fetched automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On macOS this builds and installs AU + VST3 to `~/Library/Audio/Plug-Ins/`;
on Linux/Windows it builds VST3.
