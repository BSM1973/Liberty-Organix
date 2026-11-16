# ORGANIX v0.9 — MATERIAL REACTIONS

## Purpose

Six audio materials become **cross-coupled DSP engines**, instead of only a sum of two audio streams. Every directed source/destination combination among LIQUID, CRYSTAL, PLASMA, VAPOR, METAL and LIGHT has a named identity (36 in total), its own bounded transfer coefficient and destination-specific audio behaviour.

## Audible flagship reactions

- LIQUID -> PLASMA — "Plasma ralenti": the time-varying envelope from Liquid reduces Plasma's **actual ring-modulation oscillator frequency**, with bounded color movement.
- METAL -> CRYSTAL — "Cristal percuté": Metal's signed impacts inject energy into Crystal's **four existing resonator oscillators**.
- VAPOR -> LIGHT — "Halo diffus": Vapor's continuous airy cloud opens **a second Light diffusion delay tap**, which affects the halo and its bounded feedback.

The other 33 directed pairings also drive the destination's real DSP parameter: Liquid's motion oscillator, Crystal's excitation, Plasma's carrier, Vapor's grain rate and speed, Metal's oscillator excitation, or Light's diffusion tap. Source material signatures are distinct (slow liquid body, crystal resonance, plasma brightness, vapor cloud, metal transients, light high-frequency halo), and 36 pair-specific transfer gains/names are defined in `Source/MaterialReactions.cpp`.

## Interaction architecture

- **One-sample delayed cross-control**: each layer's previous processed output produces a bounded envelope, attack, high-frequency/edge detector. The detector controls the opposite layer's DSP, never an instantaneous A->B->A algebraic feedback path.
- **Three host directions**: A TO B, B TO A or MUTUAL.
- **INTENSITY**: output-linked cross-control depth (0..100%). **RESPONSE**: envelope follower speed from approximately 5 to 95 ms. Depth fades in/out over approximately 16 ms.
- **Four original routing modes remain**: parallel, serial A/B, serial B/A, cross-fusion. All audio paths continue to be processed as before when reactions are OFF.
- **MATTER DNA compatible**: analysis still sets existing normalized material parameters at block boundaries; Material Reactions subsequently modulates the actual DSP at the audio-sample level.
- **MORPH 4D compatible**: each of four scenes and each of its crossfade banks has its own independent reaction follower state. The current globally selected reaction mode/amount applies to all four scene pairs; the reaction settings are not captured separately for A/B/C/D in v0.9.
- No extra fixed latency, allocation, lock, disk access or GUI read in the audio callback. Two output-activity meters use atomics for display.

## UI

The right inspector now has three carefully spaced tabs: COUCHE A, COUCHE B and RÉACTIONS. The third tab displays a dedicated dark-blue panel with enable/disable, direction dropdown, intensity, response, live A > B and B > A links and pair-specific French/English labels. The top header, FR/EN language buttons, preset browser, material selectors, MATTER DNA, XY morph and Trajectory Designer remain unchanged. The new page hides the 12 property knobs only while open. No symbols or glyph-dependent icons in the language selector.

## Preservation of v0.8 sound

The new `reactionEnable` APVTS boolean is **false by default**. Factory presets also force it OFF. Users enable it explicitly; older sessions restore the pre-v0.9 sound. `reactionAmount` (0.60), `reactionDirection` (mutual) and `reactionResponse` (0.40) are saved and host-automatable.

## Tests and release limits

- Ninth C++ test covers all 36 directed combinations, named lookup, neutral transfer for amount 0, extreme inputs, all 4 routing modes, stereo finite/clipped stress.
- Exact legacy-sound comparison on disabled state vs the v0.8 dual-engine routing code.
- Audio difference tests for Liquid->Plasma, Metal->Crystal and Vapor->Light to prevent merely cosmetic implementation.
- 756 editor geometries validate three tabs, two meters and all controls with zero overlap.
- CI must pass macOS ARM64 AU/VST3/Standalone, FFT, Morph 4D, trajectory and reaction regressions.
- Real recording tests still required: high-feedback material pairs, long evolving tails, rapid A/B identity changes, CPU and memory under sustained 4-scene morphing. Different source/target pairings may have very different musical loudness; this design does not imply they have already been auditioned or gain-matched.

## Suggested demo

Set A=LIQUID, B=PLASMA, REACTIONS ON, direction A TO B and INTENSITY 85%; listen to Plasma's changed rhythmic carrier. Then A=METAL, B=CRYSTAL for struck-resonator textures. Finally A=VAPOR, B=LIGHT and increase intensity to hear Light's second diffusion tap follow Vapor's cloud. Compare ON/OFF at matched output level, then try MUTUAL to hear both engines evolve.
