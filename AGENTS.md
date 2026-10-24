# MidSide-670 — DeepSeek Harness (`dsh`) Workspace Instructions

## 1. Mandatory Skill Activation on Turn 1
Before planning, scaffolding, writing C++ DSP/UI code, or running builds in this repository, **immediately load the project skill** using the `skill` tool:
```json
{ "name": "vst3-80s-plugin" }
```
Then read its companion reference files using the `read` tool as needed:
- `.dsh/skills/vst3-80s-plugin/references/look-and-feel-recipes.md` — JUCE 8 `Graphics` code recipes for the 1980s silver/red plastic look & feel.
- `.dsh/skills/vst3-80s-plugin/references/github-actions-ci-release.md` — Cross-platform GitHub Actions workflow (`.github/workflows/build.yml`) for Windows x64, Linux x64, macOS Universal (`arm64` + `x86_64`), and automated GitHub Release publishing.

---

## 2. Core Project & Look-and-Feel Mandates
- **Plugin Format & Framework**: JUCE 8.0.8 (`VST3` + `Standalone`), C++17, CMake (`CMAKE_SKIP_RPATH TRUE`).
- **Offline Dependencies**: Reuse `/home/brians/bls/sound/plugin_poc/external/JUCE` and `/home/brians/bls/sound/plugin_poc/deps/sysroot` (copy or symlink into this workspace so everything builds offline inside `workspace-write` sandbox permissions).
- **1980s Silver & Red Plastic Electronics Look & Feel (Boombox / Tape-Radio Combo)**:
  - **Base**: Injection-molded metallic silver ABS plastic (`#DCE1E6` -> `#C6CCD3` -> `#B5BCC4`) with crisp `1px` dark/light parting seams (`#5A6068` / `#EDF1F5`), corner screw wells, and top ventilation louvers.
  - **Highlights**: 1980s Signal / Record Red (`#D9232A`) silkscreened section borders, solid red header badges, molded red knob pointers, red hot-scale zones, and red rectangular LEDs.
  - **Contrast Bays**: Matte charcoal plastic sub-bezels (`#262930`) and recessed smoked-acrylic windows (`#111317`) housing discrete rectangular LED bargraphs (with visible unlit LED cells).
  - **Controls**: Physical fluted-skirt rotary knobs with red pointers (never glowing sci-fi neon arcs), mechanical cassette-style rectangular push-latch buttons with red status LEDs, and 3-ridge multi-position slide switches.

---

## 3. DeepSeek V4 Pro & Flash Workflow in `dsh`
- **DeepSeek V4 Pro (`deepseek-pro` / `deepseek-reasoner`)**: Use for DSP math (Mid/Side matrix, 670 variable-mu compression, time constants, oversampling, latency-compensated bypass) and custom procedural `LookAndFeel` / `Graphics` rendering.
- **DeepSeek V4 Flash (`deepseek-flash`)**: Use for CMake scaffolding, APVTS parameter definitions, presets, unit/host test harnesses, and build execution.
- **Verification**: Every build must run:
  1. `./scripts/build.sh`
  2. `./build/*Tests_artefacts/Release/*Tests` (offline DSP tests)
  3. `./build/*HostSmokeTest_artefacts/Release/*HostSmokeTest dist/*.vst3` (VST3 host scan & state round-trip)
  4. `./build/*Screenshot_artefacts/Release/*Screenshot --out=dist/midside670-ui.png` followed by **`read_image`** (to verify the 80s silver/red aesthetic and check for clipping) and **`present`** (to attach the UI preview card in the DeepSeek Harness Web UI).
