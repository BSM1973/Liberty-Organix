# ORGANIX — 6×12 material control nomenclature

All six profiles use the same *positions* in a data-driven 3×4 visual grid, but distinct labels and material accent. New controls can initially modulate a bounded approximation in the v0.2 DSP; do not advertise full future spectral algorithms.

| Engine | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| LIQUID | Fluidity | Viscosity | Current | Ripple | Depth | Turbulence | Wave Size | Surface | Pressure | Flow Speed | Feedback | Stereo Flow |
| CRYSTAL | Crystallization | Facets | Brilliance | Resonance | Harmonic Order | Shimmer | Purity | Growth | Fracture | Detune | Sparkle | Decay |
| PLASMA | Energy | Voltage | Discharge | Arc | Instability | Heat | Ionization | Sparks | Frequency Shift | Plasma Density | Pulse | Containment |
| VAPOR | Vaporization | Density | Grain Size | Drift | Cloud Size | Diffusion | Air | Fog | Motion | Distance | Scatter | Dissolve |
| METAL | Material | Hardness | Thickness | Tension | Resonance | Ring | Impact | Vibration | Fracture | Damping | Body | Shimmer |
| LIGHT | Luminosity | Radiance | Spectrum | Prism | Refraction | Beam | Halo | Glow | Shimmer | Pulse | Speed | Expansion |

## Parameter architecture
- 24 new stable APVTS IDs `aShape01..aShape12` / `bShape01..bShape12`, individually host-automatable, state-stored, normalized 0..1. These values feed the real material engines, not a graphical-only placeholder.
- Material slot selector remains `materialA` / `materialB`. An A UI knob only edits an A parameter; B only edits B. The twelve values persist when material changes, with no unseen cross-layer edit.
- In v0.2 each parameter influences a bounded prototype transformation; exact physical units (cents/hertz/seconds) are not presented unless calculated by DSP model. Percent values are truthful normalized controls.
- Later release must implement algorithm-specific typed units, per-material/slot state banks, freeze/resynthesis, and genuinely independent advanced algorithms where specified.

## Mapping workflow
Each index maps to the same normalized index of `organix::Parameters::shape`. The sound engine maps index effects to the indicated material characteristic. No index is merely decorative.
