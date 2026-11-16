# ORGANIX v0.6 — MATTER DNA (ADN SONORE)

## Goal
MATTER DNA is a deterministic, **real input-audio analysis and material modulation** engine. A voice's plosive attack can excite CRYSTAL while high-frequency breath content animates VAPOR. Other material combinations inherit energy-following motion and spectral brightness. No AI model, internet connection, prerecorded analysis or external audio service is used.

## Real-time signal path
1. Read unmodified mono/stereo input block without modifying it.
2. Run a preallocated, in-place radix-2 FFT on a Hann-windowed **1024-sample window** with a **256-sample analysis hop**; track positive spectral flux between consecutive frames.
3. Analyze RMS energy and a fast-versus-slow amplitude follower to track attacks/transients.
4. Extract relative low (<250 Hz), mid (250 Hz to approx 3500 Hz) and air (>3500 Hz) spectral energy; approximate normalized spectral centroid and spectral motion. Adapt the upper cutoff at low sample rates.
5. Gate spectral readings on input amplitude to avoid triggering materials from effectively silent input.
6. Apply bounded normalized modulation to the existing two material processors:
   - CRYSTAL: transients increase the resonator excitation/amount;
   - VAPOR: high-frequency air increases grain cloud density/amount;
   - ALL MATERIALS: energy and positive spectral motion animate each engine's movement; spectral centroid adjusts tone.
7. Existing wet/dry, routing, bypass and output stages remain unchanged. No audio buffering or new declared plugin latency is added. The feature analysis itself is windowed/smoothed and is **not sample-accurate at the feature level**.

## New host-automatable parameters
- `dnaEnable` OFF by default (old sessions/presets retain the v0.5 sound).
- `dnaDepth`, `dnaSensitivity`.
- `dnaCrystalAttack` (transients to Crystal).
- `dnaVaporAir` (high-frequency content to Vapor).
- `dnaEnergyMotion` (energy + spectral flux to motion).
- `dnaSpectralTone` (spectral centroid to tonal motion).
All parameters are serialized by the existing APVTS project/preset system.

## UI
The header has a bilingual **ADN / DNA** button opening a dedicated footer laboratory (without replacing the validated material view). The laboratory has six knobs, a single real on/off button and five genuinely audio-reactive meters: ENERGY, TRANSIENTS, AIR, CENTROID, MOTION. Labels translate to French (default) and English; no glyph-dependent decoration in the language selector.

The central procedural material view reacts to FFT attack/air/motion when DNA is enabled. Existing macro, routing, output, user preset browser and layer selection remain available when switching to their respective page.

## Suggested test with a voice recording
Select CRYSTAL on layer A and VAPOR on layer B; enable layer B; select PARALLEL; open the ADN panel; click **ACTIVER ADN**. Start with depth 50%, sensitivity 50%, Crystal Attack 80%, Vapor Air 80%, Energy/Motion 40%, Spectrum/Tone 25%. Play a vocal track with plosives and breath. Compare DNA ON/OFF at matched output level; Crystal should react to attacks and Vapor to fricatives, without promising literal speech separation.

## Explicit limits / release gates
- The FFT detects high-frequency energy, **not semantic breath/phoneme separation**. Harmonic pitched content can also trigger the Vapor air control.
- The 1024-sample FFT has limited low-frequency resolution; the spectrum is measured, not resynthesized.
- This version does not implement the planned advanced STFT audio processing, four-scene morphing, sidechain sources or machine-learning classification.
- No model inference, model training, subscription or online service is required.
- CPU and release stability must be verified on macOS ARM64 AU/VST3, with CTest, host automation and manual listening A/B before commercial release.
