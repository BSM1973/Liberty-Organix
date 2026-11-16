# ORGANIX — UI acceptance and implementation priorities

## Phase UI-01 (this branch)
1. Match screenshot composition: header, six-material rail, animated center, layer A/B right 3×4 controls, dual fusion/movement/output bottom.
2. Build consistent palette and rounded knob look and feel, robust JUCE editor geometry.
3. Bind every visible control to real processor APVTS IDs, including 12 material shapes for both layers, and connect their normalized values to material DSP.
4. Layer switching rebinds, presets update UI, low-overhead animation reflects audio.
5. Compile macOS ARM64 AU + VST3 and run existing DSP/registry tests via CI.

## Phase UI-02 (next)
- User preset save/import/export with searchable tags and favorites.
- Parameter-specific unit conversion and typed values.
- Tooltips with exact acoustic function and Shift-fine tuning.
- Full movement trajectory editor and 24-slot modulation patch list.

## Phase UI-03 (later)
- Four-way snapshot morph with crossfade for discrete engines.
- Audio-reactive spectral analyzers, visual quality settings.
- Reduced motion persisted to settings, high-contrast mode.
- Host and GUI screenshot automation; pluginval and DAW live tests.

## Sign-off criteria
- CI green is necessary but not enough. Capture screenshots at default/min/max sizes on macOS.
- Check six materials have distinct fields, labels never overlap, all 24 layer values persist after DAW reopen.
- Check click/drag A then B changes the correct engine only.
- Check dry/wet, bypass, output safety, and stereo stability.
- Exercise mouse/keyboard/automation/undo where implemented.
- Avoid announcing unimplemented features as delivered. No third-party builder traces in code, visuals or release docs.
