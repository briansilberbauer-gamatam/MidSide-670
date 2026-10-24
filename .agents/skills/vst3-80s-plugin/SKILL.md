---
name: vst3-80s-plugin
description: >-
  JUCE 8 VST3 audio plugin architecture, offline Linux CMake build/test workflow,
  GitHub Actions CI/release pipelines (Windows, Linux, macOS Universal), DeepSeek
  Harness (dsh) Flash & Pro orchestration, and 1980s silver-and-red plastic
  electronics (tape/radio combo / boombox) Look & Feel specification.
user-invocable: true
---

# VST3 Plugin Development & 1980s Silver/Red Electronics UI Skill (DeepSeek Harness)

This skill defines the engineering workflow, JUCE 8.0.8 conventions, **DeepSeek Harness (`dsh`)** tool & model orchestration (**DeepSeek V4 Pro** vs **DeepSeek V4 Flash**), cross-platform **GitHub Actions CI & Release workflows (Windows, Linux, macOS)**, and the **1980s Silver & Red Molded Plastic Electronics (Boombox / Tape-Radio Combo)** visual design system for building VST3 plugins in `/home/brians/bls/sound/MidSide-670`.

When this skill is loaded in DeepSeek Harness, also read the companion reference files using the `read` tool as needed:
- `references/look-and-feel-recipes.md` — Copy-pasteable JUCE 8 `Graphics` drawing recipes for 1980s silver/red plastic components.
- `references/github-actions-ci-release.md` — Complete `.github/workflows/build.yml` for Windows x64, Linux x64, macOS Universal (`arm64` + `x86_64`), and automated GitHub Release publishing.

---

## 1. DeepSeek Harness (`dsh`) Execution & Model Routing

### A. DeepSeek V4 Pro vs. Flash Division of Labor
Whether switching models manually in the harness or orchestrating child agents via the `workflow` / `subagent` tools, split tasks by reasoning depth:

| Task Area | Target Model | Responsibilities |
| :--- | :--- | :--- |
| **DSP & Audio Architecture** | **DeepSeek V4 Pro** (`deepseek-pro` / `deepseek-reasoner`) | Mid/Side encoding/decoding (`M = (L+R)*0.5`, `S = (L-R)*0.5`), 670-style variable-mu / program-dependent compression curves, time-constant networks (switches 1–6), transformer saturation/oversampling, bypass latency compensation, and lock-free UI metering atomics. |
| **Custom `LookAndFeel` & Vector Art** | **DeepSeek V4 Pro** (`deepseek-pro` / `deepseek-reasoner`) | Writing procedural JUCE `Graphics` rendering in `LookAndFeel` and `Theme`: 3D molded silver ABS bevels, parting seams, fluted rotary knobs with red pointers, mechanical cassette-style push-latch buttons, ridged slide switches, and smoked-acrylic discrete LED bargraphs. |
| **Scaffolding & Parameter Plumbing** | **DeepSeek V4 Flash** (`deepseek-flash`) | Creating `CMakeLists.txt`, `Source/Parameters.h`, `Source/Presets.h`, APVTS parameter layout, attachment wiring, value formatters (`dB`, `ms`, `Hz`, `%`, ratios), and component `resized()` grid placement. |
| **Tests, Tooling & Build Loop** | **DeepSeek V4 Flash** (`deepseek-flash`) | Writing `Tests/DspTests.cpp`, `Tools/HostSmokeTest.cpp`, `Tools/RenderEditor.cpp`, `scripts/setup-linux.sh`, `scripts/build.sh`, and executing CMake builds and test binaries. |

### B. DeepSeek Harness Tooling Rules
1. **Task Tracking (`todo_write` / `create_goal`)**:
   - At the beginning of a multi-step build, call `todo_write` to maintain a clear checklist (`Setup JUCE & sysroot`, `Implement Parameters & DSP`, `Implement 1980s Silver/Red UI`, `Build & Run DSP/Host Tests`, `Render & Inspect UI Screenshot`).
2. **Parallel Subagents & Multi-Model `workflow`**:
   - Use `subagent` / `subagent_fork` to parallelize independent C++ modules (for example, one agent implementing `Source/dsp/` + `Tests/DspTests.cpp` while another implements `Source/ui/`) so long C++ files and compiler outputs do not bloat the main session context.
   - When using the `workflow` tool to coordinate models explicitly, pass `model` in `agent(prompt, { model: "deepseek-pro" | "deepseek-flash", label, phase })`.
3. **File Observation Policy (`read` before `edit` or `write`)**:
   - `dsh` enforces a strict read-before-write policy on existing files. Always call `read` on an existing file before calling `edit` or overwriting it with `write`.
4. **Sandbox Permissions (`workspace-write` vs `danger-full-access`)**:
   - By default, `bash` runs in `workspace-write` mode: writing is permitted inside `/home/brians/bls/sound/MidSide-670` and `/tmp`, while reading is permitted across the local filesystem (including `/home/brians/bls/sound/plugin_poc`).
   - Always pass `description` before `command` in `bash` calls, and set `workdir: "/home/brians/bls/sound/MidSide-670"`.
   - Symlinking or copying `/home/brians/bls/sound/plugin_poc/external/JUCE` and `/home/brians/bls/sound/plugin_poc/deps/sysroot` into this workspace works inside default `workspace-write` mode—do **not** download packages from the internet when the sibling `plugin_poc` cache exists.
   - Only request `sandbox_permissions: "danger-full-access"` (with a clear `justification`) if copying the built `.vst3` bundle outside the workspace (e.g., to `~/.vst3`).
5. **Visual Inspection & Presentation (`read_image` + `present`)**:
   - Never declare UI work finished based solely on a zero exit code from CMake.
   - Run the `RenderEditor` screenshot binary (using `xvfb-run -a` if headless) to output a 2x PNG (`dist/midside670-ui.png`).
   - Call **`read_image`** on `dist/midside670-ui.png` to visually inspect the rendered plugin and verify:
     - Silver molded plastic chassis (`#C6CCD3`) with crisp dark/light parting seams and red silkscreen framing (`#D9232A`).
     - Fluted knobs with red pointers, mechanical push buttons with red LED lenses, ridged slide switches, and recessed smoked-acrylic LED meter windows.
     - Zero text truncation, zero overlapping labels, and live LED meter activity.
   - Call **`present`** on the PNG file path and embed `![MidSide-670 UI Preview](<dist/midside670-ui.png>)` in the final response so the user sees the rendered UI card in the DeepSeek Harness Web UI.

---

## 2. Critical JUCE 8.0.8 & Real-Time Audio Guardrails

1. **Strict JUCE 8.0.8 API Compliance**:
   - **Fonts**: Never use the deprecated `juce::Font(typeface, height, style)` constructor. Always construct fonts via `juce::FontOptions`:
     ```cpp
     juce::Font result { juce::FontOptions (typeface).withHeight (height) };
     ```
   - **Plugin Host Test**: When building a host test that scans `.vst3` bundles, set `JUCE_PLUGINHOST_VST3=1` in `target_compile_definitions`.
2. **Real-Time Audio Thread Safety**:
   - Zero heap allocations (`new`, `malloc`, `std::vector::push_back`, `juce::String`), zero mutex locks, and zero file/OS calls inside `processBlock()`.
   - Read parameters once per block via raw atomic pointers (`apvts.getRawParameterValue(id)->load()`) and smooth sensitive gain/compression changes.
   - Expose UI telemetry (Input/Output L/R peaks, Mid/Side levels, Gain Reduction dB) exclusively via `std::atomic<float>` polled by a `30 Hz` `juce::Timer` on the message thread.
3. **Constant Latency Across Bypass**:
   - If any stage uses oversampling or lookahead, report the latency via `setLatencySamples()` in `prepareToPlay()` and route bypassed audio through a matching `juce::dsp::DelayLine` so bypassing never causes phase/timing jumps in the host.
4. **Offline Linux Sysroot & JUCE Bootstrap**:
   - Reuse `/home/brians/bls/sound/plugin_poc/external/JUCE` (JUCE 8.0.8) and `/home/brians/bls/sound/plugin_poc/deps/sysroot` (Ubuntu 24.04 rootless dev headers/libs + `xvfb-run`).
   - Configure `PKG_CONFIG_PATH`, `PKG_CONFIG_SYSROOT_DIR`, `CMAKE_CXX_FLAGS`, and `CMAKE_EXE_LINKER_FLAGS` / `CMAKE_SHARED_LINKER_FLAGS` identically to `../plugin_poc/scripts/build.sh`, with `CMAKE_SKIP_RPATH TRUE` so built binaries do not embed restricted sysroot RPATHs.

---

## 3. Project Layout & CMake Architecture

Use the proven four-target CMake structure (`CMAKE_CXX_STANDARD 17`, `CMAKE_SKIP_RPATH TRUE`):

```text
.
├── .github/
│   └── workflows/
│       └── build.yml           # Windows x64, Linux x64, macOS Universal CI + GitHub Release job
├── .gitignore                  # Excludes external/, deps/, build*/, dist/, stage*/, *.zip
├── AGENTS.md                   # Always-on DeepSeek Harness project instructions
├── CMakeLists.txt              # Plugin (VST3+Standalone), DspTests, HostSmokeTest, Screenshot
├── Resources/                  # Bundled TTF fonts + OFL license (from ../plugin_poc/Resources)
├── Source/
│   ├── Parameters.h            # Stable parameter IDs & ranges
│   ├── Presets.h               # Deterministic factory presets (reset all to default, then apply)
│   ├── PluginProcessor.h/.cpp  # APVTS, M/S routing, 670 variable-mu DSP chain, atomic meters
│   ├── PluginEditor.h/.cpp     # Fixed 1180x720 design canvas + AffineTransform resizer
│   ├── dsp/                    # Header-only or modular DSP classes
│   └── ui/
│       ├── Theme.h/.cpp        # 80s Silver & Red palette, typography, molded plastic panel helpers
│       ├── RetroLookAndFeel.*  # Custom knobs, mechanical buttons, slide switches, combos
│       ├── Controls.h/.cpp     # ParamKnob, PushLatchButton, SlideSwitch, LedBarMeter, VuMeter
│       └── ModulePanel.h/.cpp  # Molded silver sub-chassis bay with silkscreened red frame
├── Tests/
│   └── DspTests.cpp            # Offline deterministic DSP test runner
├── Tools/
│   ├── HostSmokeTest.cpp       # Instantiates built .vst3 in JUCE VST3 host & round-trips state
│   └── RenderEditor.cpp        # Feeds synthetic signal & exports 2x PNG screenshot of UI
└── scripts/
    ├── setup-linux.sh          # Symlinks/copies ../plugin_poc/external/JUCE & deps/sysroot
    └── build.sh                # Configures, builds Release, and populates dist/
```

---

## 4. Look & Feel Specification: 1980s Silver & Red Plastic Electronics

The UI must evoke **1980s portable stereo tape/radio combos, boomboxes, and Walkman/hi-fi component decks** (e.g., early-80s Sony CFS/WM, Sharp GF, Aiwa, Panasonic RX series).

### Core Aesthetic Principles
- **No Modern Dark-Mode / Sci-Fi Glow**: Do **not** use dark slate backgrounds with glowing neon arcs or glassmorphism. Everything is **physical, opaque, injection-molded plastic, silkscreened ink, smoked acrylic windows, and discrete rectangular LEDs**.
- **Base Material — Metallic Silver Molded ABS Plastic**:
  - The main body is spray-finished metallic silver plastic with directional top-lit gradients, crisp `1px` dark parting seams (`#5A6068`) paired with `1px` bright edge catchlights (`#EDF1F5`), molded corner screw wells, and subtle recessed ventilation slots.
- **Contrast Material — Matte Charcoal Sub-Bezels & Smoked Acrylic**:
  - Just like 80s boomboxes frame the tuner dial, cassette door, and LED meters in dark charcoal plastic, use recessed **matte charcoal ABS bays (`#262930`)** and **smoked acrylic windows (`#111317`)** to create high-contrast focal areas on the silver body.
- **Highlight Color — 1980s Signal / Record Red**:
  - Use **Signal Red (`#D9232A`)** for silkscreened section framing lines, solid red category badges (`MID`, `SIDE`, `PEAK`, `LIMIT`), molded red pointer wedges on knobs, tactile red push-buttons (like a cassette `REC` key), and red overload scale zones.

### Color Palette Constants (`Source/ui/Theme.h`)

Use this exact palette in `Theme.h`:

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace retro80s::ui
{
    // --- Molded Silver ABS Plastic Chassis ---
    inline const juce::Colour silverLight      { 0xffdce1e6 }; // Top surface gradient start
    inline const juce::Colour silverBase       { 0xffc6ccd3 }; // Primary molded silver faceplate
    inline const juce::Colour silverMid        { 0xffb5bcc4 }; // Lower panel gradient / recessed bay
    inline const juce::Colour silverRecess     { 0xff9ea6af }; // Recessed control well background
    inline const juce::Colour silverShadow     { 0xff7c848d }; // Bottom/right outer bevel shadow
    inline const juce::Colour seamDark         { 0xff5a6068 }; // 1px molded plastic parting groove
    inline const juce::Colour seamHighlight    { 0xffedf1f5 }; // 1px specular catchlight next to seam

    // --- Charcoal ABS Sub-Bezels & Smoked Acrylic Windows ---
    inline const juce::Colour charcoalBezel    { 0xff262930 }; // Raised dark plastic sub-panel (meter/tuner surround)
    inline const juce::Colour charcoalDeep     { 0xff191b20 }; // Switch slots, knob wells, vent louvers
    inline const juce::Colour smokedWindow     { 0xff111317 }; // Recessed LED / cassette meter window

    // --- 1980s Signal Red Highlights & Molded Red Plastic ---
    inline const juce::Colour signalRed        { 0xffd9232a }; // Primary silkscreen red lines, badges & pointers
    inline const juce::Colour signalRedBright  { 0xffff3b42 }; // Specular ridge on molded red plastic caps
    inline const juce::Colour signalRedDark    { 0xff8f1318 }; // Shadow edge on molded red buttons/switches
    inline const juce::Colour redScaleZone     { 0xffc81e24 }; // Hot/overload tick marks on silver faceplate

    // --- Discrete LED Segments (Inside Smoked Acrylic Window) ---
    inline const juce::Colour ledRedLit        { 0xffff2a2a }; // Active red LED segment / PEAK indicator
    inline const juce::Colour ledRedUnlit      { 0xff381417 }; // Unlit red LED cell (visible in window)
    inline const juce::Colour ledAmberLit      { 0xffffa826 }; // Warm warning LED segment
    inline const juce::Colour ledAmberUnlit    { 0xff332412 }; // Unlit amber LED cell
    inline const juce::Colour ledGreenLit      { 0xff36d65b }; // Signal present LED segment
    inline const juce::Colour ledGreenUnlit    { 0xff142e1b }; // Unlit green LED cell

    // --- Silkscreened Ink Typography ---
    inline const juce::Colour inkBlack         { 0xff181a1e }; // Primary silkscreened text on silver plastic
    inline const juce::Colour inkCharcoal      { 0xff3a3f47 }; // Secondary labels, scale numbers on silver
    inline const juce::Colour inkMuted         { 0xff5c636e }; // Fine technical annotations on silver
    inline const juce::Colour inkLightOnDark   { 0xffe8edf2 }; // Crisp white/silver print on charcoal or red badges
    inline const juce::Colour inkMutedOnDark   { 0xff98a1ab }; // Sub-labels inside dark charcoal bezels
}
```

### Component-by-Component Visual Recipes

#### 1. Main Chassis & Molded Sub-Panels (`drawMoldedPanel`)
- **Outer Chassis**: Fill with a vertical `ColourGradient` from `silverLight` at the top to `silverMid` at the bottom. Draw a `2px` outer bevel (top/left `seamHighlight`, bottom/right `silverShadow`).
- **Molded Plastic Seams**: Divide functional bays (Header, Mid Channel, Side Channel, Master/Matrix, Footer) using **double-line injection seams**:
  - Draw a `1.0f` line in `seamDark`, immediately followed `+1.0f` below/right by a `1.0f` line in `seamHighlight`.
- **Silkscreened Red Framing**: Inside each module panel, draw a crisp `1.5f` rounded rectangle (corner radius `4.0f`–`6.0f`, **tighter 80s corners**, not large `14px` web cards!) in `signalRed`.
- **Module Header Strip**: At the top-left of the red frame, render a solid `signalRed` rectangular tab/badge containing the module number and title in bold uppercase `inkLightOnDark` (`[ 01 // MID COMPRESSOR ]`), accompanied by technical silkscreen sub-labels (`VARIABLE-MU // TIME CONSTANT`) in `inkCharcoal`.
- **Hardware Details**: Paint 4 small recessed screw heads or molded plastic rivet dimples at the outer chassis corners and a row of narrow horizontal recessed vent slots (`charcoalDeep` with bottom `seamHighlight`) along the top header bar.

#### 2. 1980s Rotary Knobs (`drawRotarySlider` — Fluted Skirt + Red Pointer)
Never draw glowing neon arcs around knobs. Instead, render a physical 3-layer 1980s component knob:
1. **Silkscreened Faceplate Scale (Outer Ring)**:
   - Draw 11 major ticks and 10 minor sub-ticks directly on the silver panel around the knob circumference.
   - Ticks below 75% travel use `inkBlack` (`1.4f` thick); ticks above 75% (the "hot/red zone") use `signalRed` (`1.8f` thick), optionally joined by a silkscreened red arc band.
2. **Recessed Mounting Well**:
   - Fill a circle at `radius * 0.76f` with `charcoalDeep` and a subtle top-left inner shadow / bottom-right `seamHighlight` rim.
3. **Fluted Plastic Skirt & Brushed Cap**:
   - Draw 20–24 radial serration notches around `radius * 0.70f` in `charcoalBezel` / `inkBlack` to simulate a grippy molded plastic skirt.
   - Fill the inner cap (`radius * 0.54f`) with a metallic silver or dark anthracite linear gradient (`silverLight` to `silverShadow`, or `charcoalBezel` to `charcoalDeep` for primary stepped switches) with a `1px` specular ring.
4. **Molded Red Pointer Wedge**:
   - Draw a bold, physical `signalRed` pointer bar (`2.5f`–`3.0f` wide) extending from the inner cap edge to the outer skirt, with a `1px` dark drop shadow and a bright `signalRedBright` center line.
5. **Value Callout**:
   - Display the parameter name above or below in uppercase tracked `inkBlack` (`9pt` Black/Semibold), and the live numeric readout inside a small recessed dark pill (`charcoalDeep` background, `inkLightOnDark` or `signalRedBright` text) resembling a mini digital/mechanical counter window.

#### 3. Mechanical Tape-Deck Push Buttons (`PowerSwitch` / Latching Toggles)
- Model bypass and link toggles after **1980s cassette deck rectangular push buttons**:
  - Draw a dark recessed rectangular well (`charcoalDeep`, corner radius `2.5f`).
  - Inside the well, draw a beveled rectangular plastic button cap (`silverBase` when OFF, or molded `signalRed` for primary/record-style toggles).
  - **Engaged (ON) State**: Shift the button cap `1.5px` down inside the well, compress its bottom shadow, and illuminate a **rectangular red LED indicator (`ledRedLit` with subtle ruby halo)** in a small bezel directly above the key, labeled `IN` / `ACTIVE` in silkscreened `inkBlack`.
  - **Bypassed (OFF) State**: Button sits raised (`0px` offset) with a deeper bottom shadow (`3px` `charcoalDeep`), and the LED lens shows `ledRedUnlit`. Do **not** fade out the whole module panel with low alpha (`0.38f`)—physical hardware doesn't turn transparent when bypassed! Instead, dim only the active LEDs and switch the status badge to `BYPASS`.

#### 4. Ridged Multi-Position Slide Switches (`ModeBar` / Time Constant Selector)
- Replace flat web-style segmented pills with a **physical 80s slide switch**:
  - Draw a horizontal recessed dark track (`charcoalDeep`, height `18px–22px`, corner radius `3px`) with silkscreened position labels (`STEREO`, `MID/SIDE`, `DUAL` or `TC 1..6`) printed above/below each detent in `inkBlack` (and `signalRed` for the active index).
  - At the selected detent position, draw a raised rectangular slider thumb (`silverLight` to `silverMid` gradient, or `signalRed` gradient) with a crisp `1px` dark border and **three vertical molded grip ridges (`|||`)** in the center of the thumb.

#### 5. Smoked Acrylic Meter Window (`LedBarMeter` / `GainReductionMeter` / `StereoMeter`)
- Group all meters inside a **recessed smoked-acrylic window bay** (`smokedWindow` `#111317`) framed by a `charcoalBezel` outer surround and a `1px` inner `signalRed` pinstripe:
  - **Glass Reflection**: Paint a subtle diagonal polygon across `25%` of the window in `juce::Colours::white.withAlpha(0.045f)` so it reads as a clear plastic cassette/tuner window.
  - **Discrete LED Segments**: Draw meters as an array of **16 to 24 discrete rectangular LED cells** (`gap = 2px`, corner radius `1px`).
  - Always draw **unlit LED segments** (`ledRedUnlit` / `ledGreenUnlit`) so the physical LED grid is visible even during silence.
  - For **Gain Reduction**: Right-to-left or left-to-right **all-red (`ledRedLit`)** or amber-to-red discrete LEDs with silkscreened scale numbers (`0`, `-3`, `-6`, `-10`, `-15`, `-20 dB`) in `inkLightOnDark`.
  - For **Stereo / M-S Output**: Dual horizontal LED bars (`L / R` or `M / S`) with green segments up to `-12 dB`, amber to `-3 dB`, and `ledRedLit` above `-3 dB`, plus a circular **red `PEAK` LED jewel** that latches on clip.

---

## 5. Fixed Design Canvas & Resizable Scaling Pattern

Always lay out `PluginEditor` in a fixed design coordinate system (`1180 x 720`) inside a child content component, and scale it smoothly in `AudioProcessorEditor::resized()`:

```cpp
constexpr int designWidth = 1180;
constexpr int designHeight = 720;

// In Editor constructor:
setResizable (true, true);
setResizeLimits (885, 540, 1770, 1080);
if (auto* c = getConstrainer())
    c->setFixedAspectRatio (static_cast<double> (designWidth) / static_cast<double> (designHeight));
setSize (designWidth, designHeight);

// In Editor::resized():
const float scale = juce::jmin (static_cast<float> (getWidth()) / static_cast<float> (designWidth),
                                static_cast<float> (getHeight()) / static_cast<float> (designHeight));
const float offsetX = (static_cast<float> (getWidth()) - static_cast<float> (designWidth) * scale) * 0.5f;
const float offsetY = (static_cast<float> (getHeight()) - static_cast<float> (designHeight) * scale) * 0.5f;
content->setBounds (0, 0, designWidth, designHeight);
content->setTransform (juce::AffineTransform::scale (scale).translated (offsetX, offsetY));
```

---

## 6. Cross-Platform GitHub Actions CI & Release Publishing (Windows, Linux, macOS)

When creating or updating `.github/workflows/build.yml` and `CMakeLists.txt`, read `references/github-actions-ci-release.md` and follow these mandatory rules:

1. **Hybrid Local (`external/JUCE`) vs. CI (`FetchContent`) Resolution in `CMakeLists.txt`**:
   - Check `if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")` first so local offline builds in `dsh` use the symlinked/copied JUCE 8.0.8 tree, and fall back to `FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.8 GIT_SHALLOW TRUE)` on GitHub Actions runners.
   - Ensure `.gitignore` excludes `external/`, `deps/`, `build*/`, `dist/`, `stage*/`, `*.zip`, and `*.sha256`.
   - Do **not** define `JUCE_DISPLAY_SPLASH_SCREEN=0` in `CMakeLists.txt` (removed in JUCE 8.0.8 and triggers a `#error`).
2. **Three OS Build & Test Jobs**:
   - **Windows x64 (`runs-on: windows-2022`)**:
     - Always pin `windows-2022` (not `windows-latest`, which uses the VS 2026 image lacking the VS 2022 generator expected by JUCE 8.0.8).
     - Configure with `-A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DMIDSIDE670_BUILD_SCREENSHOT=OFF -DMIDSIDE670_BUILD_HOST_TEST=ON`.
     - Run both `MidSide670Tests.exe` and `MidSide670HostSmokeTest.exe`, then package `.vst3` + Standalone `.exe` into `MidSide670-Windows-x64-v<VERSION>.zip` + `.sha256`.
   - **Linux x64 (`runs-on: ubuntu-latest`)**:
     - Install ALSA, X11, FreeType, and Fontconfig `-dev` packages, build with `-G Ninja -DCMAKE_BUILD_TYPE=Release`, run DSP & Host tests, strip debug symbols (`strip --strip-unneeded`), and package `MidSide670-Linux-x64-v<VERSION>.zip` + `.sha256`.
   - **macOS Universal (`runs-on: macos-14`)**:
     - Configure a Universal Binary (`-G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"`).
     - Run ad-hoc code signing (`codesign --force --deep --sign -`) on both `*.vst3` and `*.app` **before** running `MidSide670HostSmokeTest` and packaging (required on Apple Silicon).
     - Package bundles using `ditto -c -k --sequesterRsrc` into `MidSide670-macOS-Universal-v<VERSION>.zip` + `.sha256`.
3. **Automated GitHub Release Job (`publish-release`)**:
   - Depends on all three OS jobs: `needs: [ windows-x64, linux-x64, macos-universal ]`.
   - Runs when pushing a version tag (`v*`, e.g., `v1.0.0`) or manually via `workflow_dispatch` (`inputs.release_tag != ''`):
     ```yaml
     if: ${{ github.ref_type == 'tag' || (github.event_name == 'workflow_dispatch' && inputs.release_tag != '') }}
     ```
   - Grants `permissions: contents: write` only on the `publish-release` job, downloads all uploaded `MidSide670-*` artifacts via `actions/download-artifact@v4` (`merge-multiple: true`), and publishes the GitHub Release with `softprops/action-gh-release@v2` (`generate_release_notes: true`, `overwrite_files: true`).

---

## 7. Local Verification Checklist (Mandatory Before Completion)

Every feature or UI iteration in DeepSeek Harness must pass all four automated checks:

1. **Build Release Binaries**:
   ```bash
   ./scripts/build.sh
   ```
2. **Run Offline DSP Unit Tests**:
   - Verify finite output, parameter response, Mid/Side matrix round-trip accuracy, gain reduction curves, and bypass latency alignment:
   ```bash
   ./build/*Tests_artefacts/Release/*Tests
   ```
3. **Run VST3 Host Smoke Test**:
   - Verify the `.vst3` bundle scans, instantiates, exposes all parameters/presets, processes audio, and serializes/restores state:
   ```bash
   ./build/*HostSmokeTest_artefacts/Release/*HostSmokeTest dist/*.vst3
   ```
4. **Render, Inspect (`read_image`), and Present (`present`) 2x UI Screenshot**:
   - Run the screenshot target (under `xvfb-run -a` using the sysroot wrapper if headless) while feeding synthetic audio so LED meters show live activity:
   ```bash
   ./build/*Screenshot_artefacts/Release/*Screenshot --out=dist/midside670-ui.png
   ```
   - Call `read_image` on `dist/midside670-ui.png` to verify the 1980s silver plastic body, red highlights, silkscreened labels, and zero text clipping/overlap.
   - Call `present` with `/home/brians/bls/sound/MidSide-670/dist/midside670-ui.png` and include `![MidSide-670 UI Preview](<dist/midside670-ui.png>)` in the final reply.
