# ORGANIX — JUCE UI component architecture (v0.2)

## Suggested component tree
```
OrganixAudioProcessorEditor (AudioProcessorEditor)
├── LookAndFeel: OrganixLookAndFeel
├── Header: branding, presets, next/previous, bypass, macro view
├── MaterialSidebar: six MaterialButtons, selected A/B badges
├── MatterView: custom procedural, audio-driven interactive drawing
├── LayerPropertyPanel: tab A/B + twelve APVTS-bound Dial objects
├── BottomPanelDefault: fusion/balance/wet, movement/macros, input/output meters
└── BottomPanelMacros: eight performance macros
```

## Responsibilities
- `OrganixUITheme.h`: immutable palette, 6×12 property name table, material accent and descriptions.
- `OrganixLookAndFeel`: knob geometry, focus, buttons, muted/active colors, professional popup style.
- `MatterView`: only UI paint and XY event forwarding; timer at ~30Hz; no DSP in paint().
- `OrganixAudioProcessorEditor`: owns attachments; resizes; selection state; timer for meter/preset refresh.
- Processor: APVTS owns automatable controls. Switching layer changes binding source; never silently duplicates a control into two destinations.
- DSP: `MaterialDSP` owns actual processing; no graphics code or graphics thread calls.

## C++ implementation constraints
- Prefer JUCE 8 public APIs (AudioProcessorValueTreeState::SliderAttachment, juce::Timer, juce::Path, juce::Graphics).
- Use arrays of dials and a static 6×12 metadata table to prevent per-screen copies.
- Lifetime: attachments must be destroyed before the sliders, editor LookAndFeel detached on destruction, timer stopped in destructor.
- Accessibility: visible parameter text and text boxes, tab selection obvious, host automation IDs stable.
- Draw in the same coordinate system as layout; all corners/paths within bounds.
- Use sound-reactive atomics for RMS/peak, not prerecorded video or mock readings.
- Avoid allocations/locks/disk/GPU access inside processBlock().
- Header/panel ornamentation is painted; actual controls are JUCE components.

## Planned refactors, not yet guaranteed
Isolate generic `MaterialButton`, `ParameterDial`, `MaterialHeader`, `MeterStrip` components after first screenshot comparison; add UI snapshots with test geometry and contrast measurements. True browser tags/favorites require persistent data model and are not visual-only toggles.

## Acceptance
Build with CMake+JUCE 8.0.10, both AU and VST3 ARM64. Run CTest. Manually inspect all six materials for exact placement and contrast; test preset/state restore and DAW mouse interactions.
