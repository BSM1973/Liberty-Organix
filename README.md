# Organix — Living Audio Matter

## Premium native GUI v0.2 (feature branch)
- Visual architecture based on six supplied MATTERFLOW reference images, adapted to the **ORGANIX** brand.
- Six distinctive native JUCE material views; 3×4 material property grid.
- Editable layer A/B; 24 persistent, automatable property values and real bounded prototype audio effects.
- Elegant matte knob rendering, dark midnight design tokens, responsive three-panel footer and eight-macro view.
- Six technical UI documents under `docs/UI-*.md`.
- Material-specific DSP is *prototype-grade*: nonlinear/ring/granular/resonator/delay engines exist, but STFT spectral resynthesis, complete trajectory editing, physics and final 100-preset listening QA are still outstanding.
- Build status: verify the CI workflow for the latest commit. CI compilation alone does not validate DAW audio, usability or distribution signing.



**Liberty Music Software**  
*Shape Sound. Transform Matter. Create the Impossible.*

This repository is an original JUCE 8 C++ **early development prototype** of the Organix audio effect. It is **not** a finished commercial plugin.

## What works in source code (v0.1)

- Six **audible, different** pure C++ DSP materials: LIQUID (modulated delay), CRYSTAL (harmonic modal resonators), PLASMA (nonlinear multiband distortion/ring modulation), VAPOR (windowed granular clouds), METAL (inharmonic resonators and transient excitation), LIGHT (harmonic exciter and moving delay).
- Two independent material slots, serial A→B, serial B→A, parallel, and bounded cross-fusion.
- Parameters exposed through JUCE `AudioProcessorValueTreeState`, including input/output, wet/dry, bypass, and eight fixed-assignment performance macros.
- JUCE editor with scalable dark interface, a real-time energy-responsive procedural visualizer, drag-to-control motion/tone, preset browser, meters, and engine controls.
- 100 **distinct parametrically configured prototype presets** retaining the names, categories and material combinations from the specification. These need manual listening, engineering, modulation assignments and leveling before being advertised as finished factory sound designs.
- Host state serialization, mono/stereo support, protected feedback values, denormal prevention, and fixed zero-latency time-domain routing.
- Headless DSP stress test across 44.1/48/96/192 kHz.

## Not yet implemented

- True STFT spectral resynthesis/freeze/blur/gate and proper spectral reporting (LIGHT is currently a harmonic exciter, **not** the full spectral engine described by the specification).
- The 12-parameter panels specific to each material, Matter Physics, interactive brushes and trajectory editor.
- 4 LFO, 4 envelopes, 2 envelope followers, 2 step sequencers, random sources, 24-slot modulation matrix and free macro assignments.
- Four snapshots, XY morph and preset bank/user preset management.
- Global finishing EQ, dynamics, saturation, reverb, delay, true-peak metering, advanced automation mapping and sidechain routing.
- Per-engine transition crossfades, full realtime/audio quality benchmarking, DAW compatibility tests, preset listening/QA, signed/notarized release artifacts.

## Build

Requires CMake 3.22+, a C++17 compiler, and JUCE 8.0.10 sources. CMake fetches JUCE unless `-DJUCE_SOURCE_DIR=/path/to/JUCE` is provided.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

**macOS:** JUCE builds AU, VST3 and Standalone. **Windows:** JUCE builds VST3 and Standalone. This version is not signed or notarized. GitHub Actions has validated the v0.1 AU/VST3 macOS ARM64 build. The new v0.2 UI branch needs a separate green CI run and manual DAW visual/audio validation.

To run only dependency-free DSP tests (on any platform with C++17):

```bash
cmake -S . -B build-dsp -DORGANIX_BUILD_PLUGIN=OFF
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

## Safety and development notes

This prototype has no additional algorithmic FFT latency and currently reports zero samples. It does **not** promise zero latency once spectral processing is introduced. The audio callback does not allocate or acquire locks. Large delay buffers are allocated in `prepareToPlay`; the visualizer runs on the GUI thread. Further profiling, sanitizer tests, audiotesting, click-free mode switching and DAW validation are required. No external processing service is required for sound.

Copyright © Liberty Music Software.
