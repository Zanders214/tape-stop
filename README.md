# Tape Stop

A **tape stop effect** audio plugin (VST3 / AU) built with [JUCE](https://juce.com).

When you engage the **Stop** toggle, the audio winds down in speed and pitch to
silence — like powering off a tape machine or a turntable. Release it and the
tape spins back up to full speed. It's a classic transition / build effect for
electronic and hip-hop production, and works in Ableton Live or any VST3/AU host.

## Parameters

| Control      | What it does                                                        |
|--------------|---------------------------------------------------------------------|
| **Stop**     | Automatable on/off toggle. On = wind down, Off = spin back up. Draw it as automation in Ableton to trigger the effect. |
| **Stop Time**  | How long the spin-**down** takes (1–5000 ms).                     |
| **Start Time** | How long the spin-**up** takes (1–5000 ms).                      |
| **Curve**    | Slowdown shape: `0` = linear, `1` = heavy exponential (fast drop, long linger). |
| **Return**   | How the tape rejoins the live signal on release: **Snap** (instant, click-free rejoin from silence) or **Spin Up** (turntable-style catch-up through the buffered audio). |

## User interface

The editor is the **"Neon Cassette"** panel: a dark rounded panel with a cassette
graphic whose two hubs spin and slow with the tape, a spectrum tape-speed bar, a
big **STOP / STOPPING** button, a live **slowdown-curve** histogram with a playhead,
and **Stop Time / Start Time / Curve** sliders plus a subtle **Return** (Snap / Spin
Up) toggle. Everything is drawn with native JUCE (`paint()` + a custom
`LookAndFeel`); a 60 Hz timer reads the engine's live `speed` / `phase` to drive the
hubs, bar, speed read-out, and playhead. The two UI fonts (Space Grotesk + JetBrains
Mono, both OFL) are embedded as `BinaryData` — see [`Fonts/`](Fonts/). The window is
360 × 693 and resizes proportionally.

## How it works

A variable-rate resampler. Incoming audio is written to a ring buffer at a
constant rate while a read pointer advances at a varying `speed`. At `speed = 1`
the read keeps pace with the write (transparent); as `speed` falls the read lags
behind, dropping pitch and tempo; at `speed = 0` the tape is stopped. The `Stop`
toggle ramps `speed` between 1 and 0 over the configured times, shaped by the
`Curve` knob. The ramp is continuous, so there are no clicks, and the DSP makes
no allocations on the audio thread. See `Source/TapeStopEngine.h`.

## Building

Requires **CMake ≥ 3.22** and a C++17 compiler. JUCE is pulled automatically via
CMake `FetchContent` (no manual download). The first configure clones JUCE, so it
needs network access to GitHub.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Build products land under `build/TapeStop_artefacts/Release/`:

- **Windows:** `VST3/Tape Stop.vst3`
- **macOS:** `VST3/Tape Stop.vst3` and `AU/Tape Stop.component`
- A **Standalone** app is also built for quick testing without a host.

### Offline / locked-down builds

If GitHub egress is blocked, point CMake at a local JUCE checkout instead of
FetchContent:

```sh
cmake -B build -DJUCE_PATH=/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
```

### macOS universal binary

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

### Linux build dependencies

```sh
sudo apt install libasound2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev
```

(webkit and curl are intentionally disabled in `CMakeLists.txt`, so those
packages are not needed.)

## Installing in Ableton (Windows)

1. Copy `Tape Stop.vst3` into your VST3 folder, e.g.
   `C:\Program Files\Common Files\VST3`.
2. In Ableton: **Options → Preferences → Plug-Ins**, enable **VST3** and rescan.
3. Drop **Tape Stop** onto an audio track and automate the **Stop** parameter.

## CI

`.github/workflows/build.yml` builds Windows VST3, macOS VST3 + AU, and Linux
VST3 on every push, validates each with **pluginval**, and uploads the binaries
as downloadable artifacts — so you can grab a macOS build without owning a Mac.

`.github/workflows/sonarqube.yml` runs **SonarQube Cloud** static analysis over
the C++ in `Source/` and `Tests/` on pushes to `dev` and on pull requests. C/C++
can't use SonarQube's Automatic Analysis, so the job builds with a CMake
compilation database (`CMAKE_EXPORT_COMPILE_COMMANDS`) and feeds that to the
scanner. It also runs the unit tests under coverage instrumentation
(`-DTAPESTOP_ENABLE_COVERAGE=ON`) and converts the result with `gcovr --sonarqube`,
so SonarQube reports real line coverage for the DSP core. The project key and
organization live in `sonar-project.properties`; the
`SONAR_TOKEN` secret authenticates the upload. Following SonarQube's "Clean as
You Code" model, the Quality Gate is enforced on **pull requests**
(`sonar.qualitygate.wait=true`, so a red gate fails the check and can block the
merge), while pushes to `dev` publish analysis without failing CI — the first
branch analysis scores the whole existing codebase as "new code", which would
otherwise red-flag already-merged code.
