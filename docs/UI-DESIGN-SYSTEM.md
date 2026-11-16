# ORGANIX — Premium Living Audio Matter UI
Design specification v0.2 — Liberty Music Software

## Design brief and fidelity
Reference: six supplied 1504x1008 screenshot compositions (MATTERFLOW / LIVING AUDIO MATTER). Preserve their panel hierarchy, restrained depth, readable type, live material identities and the six distinct central representations. The shipped product name is **ORGANIX**, not MATTERFLOW. This document is a design specification, not evidence that every planned DSP function exists.

## Visual identity
- Canvas: #07111C (near-black midnight blue); base panel #0B1725; raised panel #0E1D2D; interactive body #102235.
- Border #243B52, subdued separator #1B3248, active focus #4A7999.
- Primary text #E7EEF8, secondary #9EADC1, subdued but readable #8094AD; no tiny gray text on near-black when it could disappear.
- Status engaged #27C7A0. Material accents: LIQUID #39E0E8; CRYSTAL #8EC7FF; PLASMA #E96CFF; VAPOR #B8A7FF; METAL #D8A15C; LIGHT #F2D989.
- Reserve material color for selected cards, knob arcs, the sound field and values. Surrounding panels stay neutral.
- Subtle 1px borders; 5–8px corner radius for inputs, 10px for panels. Avoid glossy plastic, gratuitous bloom and heavy glass.
- JUCE native font fallback, system sans. Font sizes expressed in logical px: brand microline 10, brand title 30, panel headings 11–12, engine title 24, knob labels 10, knob value 11–12, micro status minimum 10. Use proper Unicode only where glyph coverage verified.
- Preferred reference viewport 1504×1008 logical px. 16px external gutter, 14px inter-panel gap, 12–18px internal padding.
- Focus, hover, pressed, bypassed and disabled states must all be distinguishable without relying solely on color.

## Page geometry
At design size 1504×1008:
- Header ~100 px; left materials 164 px; right properties 395 px; central sound field flexible (~850 px); bottom row 187 px.
- Gap 14–16 px. Body height fills remaining space. All bounds must be computed in resized(); no fixed absolute positions that cause clipping.
- Support 1240×820 minimum; at constrained dimensions compress spacing and font without overlapping. The extreme case should clip optional ornamentation before controls or text.
- Never hide parameter labels under rotary widgets. Maintain a minimum 28×28 click target for small buttons and ~50×50 for knobs.
- Prefer an adaptive grid rather than a global zoom if the editor is resized.

## Top header
Left: line 'LIBERTY MUSIC SOFTWARE', brand 'ORGANIX', tagline 'LIVING AUDIO MATTER'. Middle-right: preset dropdown, previous/next, modified indication. Right: engaged/bypass button and an accessible toggle for macro view. Browser categories/search/favorites are designed for subsequent phases; do not ship inert buttons.

## Left material rail
Six buttons, 01–06: LIQUID, CRYSTAL, PLASMA, VAPOR, METAL, LIGHT. Each has its individual color and selected edge accent. Material changes affect the *currently selected layer* A or B. Show small A/B badges where available.

## Center sound field
Top: 'INTERACTIVE SOUND FIELD', current material + layer, 'FUSED WITH' secondary material. Toolbar: view quality selector / audio-reduced motion (future controls only when implemented). Drawing area: six original procedural compositions, audio-reactive (measured RMS/peak) and DSP-param reactive; XY gesture must automate meaningful real parameters with host gesture begin/end.
- LIQUID: nested fluid wave contours.
- CRYSTAL: translucent facets and harmonic axes.
- PLASMA: jagged electrical rings and sparks.
- VAPOR: soft diffusion rings and small particles.
- METAL: crisp polygonal resonant lattice with vibrating orbital shapes.
- LIGHT: rays, pentagonal prism and warm irradiance.
Meters at foot of center: input RMS, output RMS, peak dBFS. Never use fake meter values.

## Right properties
Tabs for LAYER A / LAYER B. Current material accent and micro-description; 12 data-driven parameter cells in a 3×4 grid. The cells should be *real* APVTS-bound host automatable controls. Per-engine label and exact acoustic function must be documented; if the underlying DSP is incomplete, identify it as simplified in development notes and do not label it as full spectral physics.

## Bottom
Default: three panels DUAL FUSION, MOVEMENT and SIGNAL & OUTPUT, with directly mapped DSP controls and clear dB/percent units. Alternate mode: eight real host-automatable performance macros.

## Interaction rules
- Select A/B, then select material in the rail; update accent, labels, visual geometry, and bound controls without replacing the audio processor.
- Mouse wheel and keyboard/value text entry via JUCE Slider; Shift fine adjustment when supported by host.
- XY gesture beginChangeGesture and endChangeGesture around host changes.
- Preset switching updates material/parameter layout and visual state.
- No UI updates/lock/acquisition in audio callback. GUI reads atomics only.
- No view makes a hidden DSP assertion: spectral reconstruction, four-way snapshot morph, sidechain and spectral physics require later modules.

## Launch gate
Contrast/readability, six distinct views, 12 actual controls per selected material, A/B selection, no overlap at min/default/max editor sizes, AU and VST3 macOS ARM64 compile, 2 DSP tests green, and DAW GUI/manual listening checks pending until performed.
