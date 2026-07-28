# Changelog

## 0.3.0 — 2026-07-28

### Added
- **Amp channels**: Clean / Rhythm / Lead / Thall / Doom — full voicing-table
  rows (gain staging, interstage filtering, asymmetry, clip curve, boost
  corner), not just clip switches. Voicing lives in one table in Saturator.cpp
- **Real cabinet IR as the built-in default** ("Hecate Cab", captured by the
  project author, embedded in the binary) replacing the synthesised 4x12
- **Clean Blend** knob: parallel undistorted path mixed post-cab — clarity
  under high gain, the thall trick
- **Drop Tune**: -1..-4 semitone pitch shift on the raw guitar
  (signalsmith-stretch); Off is a true zero-latency bypass, and host latency
  updates when engaged
- **Thall factory preset** (channel, deep 420 Hz scoop, clean blend, doubler)
- **Offline render harness** (HecateRender): bounces a deterministic low-B
  DI — or any WAV you pass it — through every factory preset, checks
  NaN/silence/clipping, writes the renders; runs in CI as a DSP regression test

### Changed
- Tap tempo + expanded FX controls (chorus delay/feedback, delay damping and
  ping-pong, doubler spread/drift) from the 0.2.x line are included
- Clean Shimmer re-voiced on the Clean channel; Solo Lead on Lead; Doom on
  the fuzz Doom channel

## 0.2.0 — 2026-07-28

### Fixed
- Split-band drive: the low band is now delayed to match the oversampler's
  latency, removing phase ripple in the low-mid overlap
- Factory preset changes are applied atomically on the message thread with
  proper automation gestures (was a VST3 threading violation and could render
  one block of the default patch mid-switch)
- Reverb no longer latches silent if a non-finite sample enters the feedback
  loop; cabinet slot B clearing no longer races the audio thread
- Meter fall time is now identical at every buffer size
- A/B switching no longer reloads unchanged IRs (no more tail dropouts)
- Doubler now works on mono buses (single ghost-take thickening)

### Added
- **Tuner**: chromatic tuner on the raw input (TUNER button in the header) —
  note, cents needle, frequency readout
- **Cabinet response display**: the CAB tab plots the frequency response of
  both IR slots (built-in cab included) with live low/high-cut markers
- **EQ curve overlay**: dragging any EQ or power-section knob shows the
  combined post-drive tone curve over the artwork
- Undo/redo (Cmd+Z / Shift+Cmd+Z in the plugin window)
- Mono-in → stereo-out bus support (the typical guitar-track setup)
- MIDI program changes switch presets
- Preset polish: unsaved-edit asterisk, save-in-place for user presets,
  host program sync, A/B starts from the current sound
- Missing-IR warning on the CAB tab when a session references an IR this
  machine doesn't have
- Tooltips on every control; typed values ("-42 dB", "450 Hz") parse in hosts
- Version number in the UI header
- CI: Windows build job, JUCE/ccache caching, pluginval strictness 10,
  per-commit plugin artifacts
- Release pipeline: pushing a `v*` tag publishes macOS (ad-hoc-signed) and
  Windows zips on GitHub Releases

## 0.1.0 — 2026-07-28

Initial release: split-band three-stage drive with Tube/Modern/Fuzz voicings,
octave-down, gate, pre-drive compressor, semi-parametric EQ, power amp
(sag/presence/depth), dual cabinet IR with mic-placement browser, chorus,
doubler, tempo-synced delay, reverb, limiter, 9 factory presets, resizable
three-tab UI over the Hecate artwork.
