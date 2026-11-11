# XEIA-FU — 909-style bass drum synthesizer (VST3 / AU / Standalone)

A synthesized 909-style kick in the spirit of the classic drum machine. Every sound is generated live; there are no samples.

## Sound engine
- Swept oscillator (mostly sine with a little triangle) with a two-stage RC pitch drop
- RC exponential amplitude envelope into a transistor-style saturating VCA
- 31-bit LFSR noise clocked at **300 kHz**, decimated to the host rate, for the attack noise
- Phase-aware click: every hit starts at the oscillator's zero crossing, and the click pulse pushes in the same direction, so the transient adds up instead of cancelling
- Asymmetric diode clipper on the output, followed by a DC blocker
- Monophonic like the hardware: a new hit chokes the previous one with a 3 ms fade (no pops)

## Controls
| Control | What it does |
|---|---|
| TUNE | Resting pitch, 30–70 Hz (**TUNE x2** doubles it to 60–140 Hz) |
| DECAY | Body length, 40–800 ms (**DECAY x2** doubles it to 1.6 s) |
| SWEEP | Depth of the pitch drop at the start of the hit |
| ATTACK | Click + noise amount |
| DRIVE | Diode clipping. The **mod wheel (CC1)** pushes it from the knob value up to full in real time |
| LEVEL | Output, −24 to +6 dB |
| VELOCITY | LIN / EXP / LOG / FIX curves |
| MIDI MODE | **PRESET**: notes C6–B7 (MIDI 84–107) play the 24 factory tunings; any other note plays the knob sound. **CHROMATIC**: the keyboard transposes the kick, with C1 at the knob pitch |
| HIT | Audition pad |

Pitch bend: ±12 semitones. Note-offs are ignored (one-shot, like the hardware).
The 24 factory patches are also exposed as host programs and in the preset menu.

## Building

You need **CMake 3.22+** and **Git**. JUCE 8 downloads automatically on the first configure.

### macOS (VST3 + AU + Standalone)
Install Xcode (or `xcode-select --install`) and CMake (`brew install cmake`), then:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -j
```
The plugins install themselves into `~/Library/Audio/Plug-Ins/VST3` and `.../Components`.
They're also in `build/XeiaFu_artefacts/Release/`.

### Windows (VST3 + Standalone)
Install Visual Studio 2022 with the **Desktop development with C++** workload, plus CMake and Git, then:
```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```
Copy `build\XeiaFu_artefacts\Release\VST3\XEIA-FU.vst3` into `C:\Program Files\Common Files\VST3`, then rescan plugins in your DAW.

### No compiler? Let GitHub build it
Push this folder to a GitHub repository. The included workflow (`.github/workflows/build.yml`) builds the Mac and Windows versions on every push. Download them from the run's **Artifacts** section on the **Actions** tab.

## Testing the DSP without JUCE
```bash
cd test && g++ -O2 -std=c++17 -I../Source render_test.cpp -o render_test && ./render_test
```
This renders all 24 presets to `demo.wav`.

## Files
- `Source/KickEngine.h` — the sound engine (plain C++, no JUCE)
- `Source/Presets.h` — the 24 factory patches
- `Source/PluginProcessor.*` — parameters, MIDI handling, state
- `Source/PluginEditor.*` — the interface

Note: JUCE is free for personal use and for commercial use under its revenue threshold. Check the JUCE licence before selling the plugin.
