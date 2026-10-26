# Changelog

All notable changes to MidSide-670 are documented in this file. The project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-10-26

**Studio-hardware redesign with a live waveform screen.** The audio engine,
parameter set, and factory presets are unchanged.

### Added

- **Central live waveform screen** — a large obsidian display fed by lock-free
  min/max telemetry from the audio thread. The trace follows the true stereo
  envelope (so anti-phase and side-only material stays visible), updates at
  roughly 750 columns per second, and is drawn as a three-layer crimson glow
  over a dB/time grid with live MID/SIDE gain-reduction readouts.
- Waveform telemetry coverage in the DSP test suite: column ordering and live
  signal presence.

### Changed

- **Complete visual redesign** — MidSide-670 now renders as a dark, brushed
  gunmetal desktop unit on a shadowed studio surface, under a flush, screwless
  glass panel with a muted crimson LED edge-glow and upper-left soft-box
  reflections.
- Replaced `RetroLookAndFeel` with `StudioLookAndFeel` and rebuilt the theme
  palette around gunmetal, obsidian, smoked glass, and crimson.
- Rebuilt every control: heavy-machined low-profile knurled knobs with crimson
  under-glow, ultra-thin recessed glass sliders, thin red-bordered tactile
  buttons, and slim glass input/output meters.
- Moved preset navigation into a slender top-centre pill readout with `<` / `>`
  chevrons and a click-to-open factory preset menu.
- Added a fixed `1180 × 720` design canvas with aspect-locked, resolution-
  independent scaling from 885 × 540 up to 1770 × 1080.
- Refreshed the README and the reference UI screenshot.

### Compatibility

- Audio engine, parameter IDs and ranges, preset data, and state layout are
  unchanged. Sessions and presets saved with v1.0.0 recall with identical
  settings and sound.
- Constant-latency bypass is preserved.

## [1.0.0] - 2026-10-24

- First public release: true Mid/Side variable-mu compressor with linked
  STEREO mode, six Fairchild-style time constants, 4× oversampled tube input
  stage, sidechain high-pass, parallel mix, stereo width, automatic make-up,
  and latency-matched bypass.
- VST3 and Standalone builds for Windows x64, Linux x64, and macOS Universal
  (`arm64` + `x86_64`), published with SHA-256 checksums.

[2.0.0]: https://github.com/briansilberbauer-gamatam/MidSide-670/releases/tag/v2.0.0
[1.0.0]: https://github.com/briansilberbauer-gamatam/MidSide-670/releases/tag/v1.0.0
