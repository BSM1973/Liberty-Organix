# ORGANIX — v0.3 Precision UI acceptance

## Visual target
The exact six user reference screenshots (LIQUID, CRYSTAL, PLASMA, VAPOR, METAL, LIGHT) define the visual target for this iteration. Keep the existing product name ORGANIX. Prioritize a professional, legible native JUCE product, not a screenshot overlay.

## Implemented in this branch
- Six individually illustrated vector material sidebar cards with visible A/B state badges, individual accent and selected/hover/pressed states.
- Twelve custom matte ring knobs per active material displayed as **knob → label → editable numeric value** (percentage or actual global dB); identical per-row baselines.
- Fixed max ~66px knob diameter and responsive row geometry, reducing disproportionate enlarged controls at 1504×1008.
- Shared, JUCE-independent panel geometry in `Source/OrganixUILayout.h`; the editor and `Tests/ui_geometry.cpp` use the exact same layout calculations.
- All main panels, center field and footer layout remain inside their bounds from 1240×820 to 1940×1220.
- Footer signature strip reserved below cards, matching the provided reference composition.
- 30Hz message-thread animator with smoothed peak/RMS, gentle new-material reveal and no callbacks added to the audio thread.
- Material-pair status correctly displays disabled layer when B is turned off; macro view button reflects current page.
- No changes to plugin ID, sound-preset registry, 24 layer-parameter IDs, or audio DSP engine families.

## CI gates
1. macOS arm64 CMake/JUCE build, AU, VST3, Standalone.
2. DSP smoke, preset registry, 72 material controls, and UI geometry CTest targets.
3. Binary validation with `lipo`.
4. Generated AU/VST3 artifact.

## Manual GUI/DAW gates still outstanding
- Open the AU in Logic Pro and VST3 in Ableton Live/Bitwig or other host, at 1240×820 / 1504×1008 / 1940×1220.
- Compare six states against supplied screenshots, including typography and caption clipping.
- Exercise layer A/B, preset change, bypass, macro switch, XY automation, numeric entry and DAW restoration.
- Observe repaint smoothness/CPU on Apple Silicon while real audio plays.
- Verify host accessibility and contrast across monitors with HiDPI scaling.
A CI green build does **not** certify the screenshots or manual DAW tests.

## Follow-up backlog
- Fully customized editable preset browser with search/tags/favorites.
- Per-material physical parameter units and DSP algorithms (currently normalized prototype properties).
- Dedicated high-contrast / reduced-motion settings and still more direct UI screenshot testing.
