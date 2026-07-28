# Changelog

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
