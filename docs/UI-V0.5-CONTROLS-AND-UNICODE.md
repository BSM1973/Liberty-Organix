# ORGANIX v0.5 — Reference controls and text rendering standard

## Implemented control corrections
The v0.4 UI incorrectly omitted reference-screen controls. v0.5 restores these **as native, host-automatable parameters with real audio processing**, not decorative widgets:

- Stereo WIDTH dial in Signal & Output, displayed as 0–200%, default 100%; implemented in M/S on the **effect signal only**, bypass and true dry feed unchanged.
- Movement FREE/SYNC switch; FREE runs at the RATE knob's Hz value, SYNC uses host BPM.
- TRAJECTORY selector: MANUAL (no modulation), SINE, TRIANGLE, PULSE, RANDOM HOLD.
- BEAT DIVISION selector: 1/1, 1/2, 1/4, 1/8, 1/16, 1/8T, 1/16T. The division is disabled visually while FREE is active, but its choice remains stored.
- RATE is an actual 0.05–12 Hz host-automatable knob, shown in Hz rather than dB or an invented percentage.
- All new parameter IDs are saved in APVTS and factory presets default to a neutral sound (FREE, MANUAL, WIDTH 100%) to avoid unexpectedly altering v0.4 projects.

## Language control: font-safe by design
The prior language ComboBox showed a decorated label `FR · Français`. On some plugin-host/system font combinations the middle dot or nearby glyph appeared incorrectly. v0.5 replaces it with **two native plain-Latin buttons: FR and EN**. No dot, emoji, flag, or third-party icon font is required. French remains the default; the user's preference persists.

French accents in actual labels are preserved using UTF-8 and JUCE String conversion. Do **not** strip French accents to hide text bugs.

## Reusable permanent text-rendering prevention
`tools/check_ui_text.py` is an enforced GitHub Actions build gate on macOS and in the fast preflight. It rejects:
- invalid UTF-8, replacement characters and suspicious mojibake sequences;
- improperly normalized Unicode (must be NFC);
- user-interface string literals containing font-dependent stars, arrows, multiplication glyphs, middle dots, typographic apostrophes, long dashes, ellipsis, invisible formatting controls or non-breaking spaces.

If a UI needs a star or arrow icon, draw it as an actual JUCE vector shape rather than using an unstable character from a font. Use simple ASCII for routing schematic labels and compact switches.

This source checker is reusable in future Liberty audio-plugin repositories, but **only this Organix repository has the build gate installed now**. There is no account-wide automatic enforcement in other repositories unless the script and workflow are added there too.

## QA boundaries
Native AU/VST3 compilation, CTest geometry, movement/width tests and glyph check must succeed before merge. A live DAW test is still required to confirm actual menu appearance, host-tempo tracking, macro automation, preset recall and the stereo sound on an audio interface.
