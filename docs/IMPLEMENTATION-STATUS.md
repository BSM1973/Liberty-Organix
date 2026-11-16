# Organix implementation status — initial prototype

Status is based on source created in this package, not on completed AU/VST3 builds.

| Requirement | Status | Remarks |
|---|---|---|
| Six DSP identities | **Implemented (prototype)** | Time-domain processing with independent engine state |
| Material A / B | **Implemented (prototype)** | Choice, enable, routing modes, cross-fusion |
| Host automation | **Partly implemented** | 8 fixed-assignment macros, core parameters exposed through APVTS; future per-material parameter sets absent |
| 100 factory presets | **Prototype** | All names/categories/material combinations present; generated unique starter settings; not yet professionally auditioned |
| Real-time responsive visual | **Implemented (basic)** | Uses output peak and RMS plus material and parameters; no FFT or advanced particle engine yet |
| 44.1–192 kHz DSP sanity tests | **Passed** | g++ C++17, ASan+UBSan, deterministic smoke; no host-specific coverage |
| Advanced spectral DSP | **Not implemented** | No WOLA/STFT, spectral freeze or resynthesis |
| Morph/snapshots | **Not implemented** | Need true 4-point audio-safe morph design |
| Matter Physics / brushes | **Not implemented** | Need defined parameter mappings and DSP layers |
| 24-slot modulation matrix | **Not implemented** | Fixed macros currently, no free assignments |
| User preset library | **Not implemented** | Factory presets and host-state recall only |
| Global dynamics, reverb and EQ | **Not implemented** | Safety saturator exists, not a master processing suite |
| AU/VST3 built and tested | **Not verified** | JUCE source unavailable in current offline development environment; CI workflow added |
| Apple signing/notarization | **Not implemented** | Requires codesigning assets and validated releases |

## Technical validation before any commercial release

1. Compile on Apple Silicon and Windows with JUCE 8.0.10; inspect all compiler errors and warnings.
2. Run pluginval and host tests in Logic, Live, Reaper, Cubase, Studio One, Bitwig and Liberty DAW.
3. Measure CPU at 32, 64, 128, 256, 512 samples at 44.1, 48, 96, 192 kHz. No allocation/lock in real-time callback.
4. Implement click-free material switching and route switching, and verify phase/latency behavior. 
5. Implement full spectral processing before labeling LIGHT as spectral resynthesis; add correct latency and aligned dry path.
6. Add every material-specific parameter, validate sonic behavior, map every visual gesture to a parameter.
7. Implement LFOs, envelopes, envelope followers, step sequencers, deterministic random sources and 24 modulation routes.
8. Implement snapshots, XY morph, morph trajectories and parameter-safe transition policies.
9. Audit and audition all 100 factory presets, improve EQ/level matching, add user preset save/import/export.
10. Complete high-DPI/resizing/accessibility tests, host project recall, crash recovery, signing and distribution.
