#pragma once
#include "FactoryPresets.h"
#include <array>
#include <cstdint>

namespace organix
{
// Organix v1.3f: individually authored, fixed factory design data.
//
// 100 entries in exactly the same order as factoryPresets. No index modulo,
// category formula or random number generator is used to invent the sounds.
// All values are deliberate starting points for subsequent human auditions.
// Historical user presets remain unchanged; only factory program recall uses
// this new bank.
enum FactoryFeature : std::uint8_t
{
    D=1u<<0, // FFT-driven MATTER DNA
    M=1u<<1, // independently voiced 4-scene Morph 4D
    T=1u<<2, // moving Trajectory Designer
    R=1u<<3, // genuine cross-material DSP reactions
    X=1u<<4  // genuine DSP modulation matrix assignments
};
struct FactorySoundDesign
{
    float aAmount,aMotion,aTone,aFeedback;
    float bAmount,bMotion,bTone,bFeedback;
    float wet,width,outputDb,energy,density;
    std::uint8_t technologies;
    std::uint8_t trajectoryShape;
    std::uint8_t modulationSource,modulationTarget;
    float dnaDepth,reactionDepth,transient,speed,modulationDepth;
    std::uint8_t scenePalette;
};
inline constexpr std::array<FactorySoundDesign,100> factorySoundDesigns {{

// 01–10 CINEMATIC ATMOSPHERES: long, moving, mostly soft-spatial material relationships
{.62f,.16f,.43f,.36f,.53f,.23f,.78f,.25f, .84f,1.28f,-3.8f,.56f,.79f, D|M|T,1,9,22,.54f,.00f,.04f,.15f,.23f,1}, // Infinite Nebula — airy spectral diffusion
{.57f,.13f,.69f,.29f,.41f,.17f,.46f,.19f, .79f,1.23f,-3.2f,.53f,.65f, D|M|R,0,8,21,.60f,.39f,.03f,.11f,.20f,2}, // Crystal Universe — resonant shadow clouds
{.68f,.26f,.36f,.30f,.48f,.20f,.73f,.24f, .81f,1.25f,-3.7f,.59f,.68f, D|M|T,1,1,2,.49f,.00f,.06f,.19f,.28f,1}, // Liquid Galaxy — graceful circulating halo
{.67f,.37f,.31f,.34f,.51f,.21f,.27f,.30f, .72f,1.11f,-5.1f,.75f,.62f, D|M|R|X,2,10,3,.42f,.35f,.12f,.13f,.22f,4}, // Dark Matter — brooding spectral energy
{.46f,.07f,.64f,.43f,.38f,.12f,.48f,.22f, .83f,1.18f,-3.4f,.37f,.71f, D|M|T,0,6,3,.33f,.00f,-.07f,.08f,.18f,3}, // Frozen Horizon — almost motionless resonators
{.36f,.27f,.82f,.16f,.47f,.17f,.78f,.14f, .74f,1.25f,-4.1f,.33f,.76f, D|M|T|X,1,9,21,.37f,.00f,.01f,.23f,.14f,2}, // Celestial Dust — dancing high particles
{.64f,.24f,.28f,.35f,.69f,.14f,.43f,.25f, .78f,1.21f,-5.3f,.65f,.55f, D|M|R,0,8,21,.48f,.44f,.15f,.15f,.20f,4}, // Beyond Gravity — elastic metal strikes
{.69f,.31f,.55f,.26f,.59f,.25f,.79f,.24f, .77f,1.19f,-5.0f,.72f,.62f, D|M|T|X,2,10,38,.46f,.00f,.13f,.24f,.19f,1}, // Quantum Atmosphere — travelling plasma bands
{.54f,.14f,.51f,.24f,.57f,.28f,.44f,.31f, .79f,1.23f,-3.5f,.54f,.84f, D|R|T,1,7,4,.47f,.37f,.04f,.15f,.16f,2}, // Solar Winds — vapor drags plasma
{.49f,.18f,.81f,.22f,.46f,.13f,.70f,.30f, .88f,1.32f,-3.7f,.47f,.64f, D|M|T,0,9,3,.57f,.00f,.00f,.09f,.20f,3}, // Eternal Expansion — crystalline light tail
// 11–20 VOCAL ALCHEMY: intelligibility, gentle envelope following, no extreme gain
{.38f,.29f,.38f,.20f,.43f,.17f,.49f,.17f, .47f,1.04f,-2.4f,.36f,.42f, D|X,0,7,2,.30f,.00f,-.06f,.33f,.15f,0}, // Liquid Voice
{.37f,.13f,.70f,.18f,.31f,.12f,.47f,.16f, .52f,1.03f,-2.8f,.40f,.31f, D|X,0,8,3,.36f,.00f,.06f,.19f,.17f,0}, // Crystal Choir
{.46f,.12f,.47f,.25f,.39f,.18f,.57f,.15f, .56f,1.11f,-3.4f,.30f,.66f, D|T,1,7,1,.43f,.00f,-.13f,.12f,.15f,0}, // Ghost Whisper
{.42f,.48f,.64f,.14f,.42f,.21f,.50f,.14f, .49f,.98f,-4.2f,.62f,.41f, D|X,2,8,3,.38f,.00f,.13f,.41f,.20f,0}, // Electric Speech
{.34f,.12f,.78f,.17f,.29f,.15f,.64f,.09f, .42f,1.07f,-2.9f,.35f,.31f, D|X,0,9,3,.26f,.00f,.08f,.13f,.12f,0}, // Golden Vocals
{.45f,.31f,.38f,.20f,.44f,.10f,.69f,.23f, .50f,.96f,-4.0f,.54f,.35f, D|X,2,8,2,.32f,.00f,.12f,.27f,.17f,0}, // Metallic Voice
{.57f,.14f,.48f,.32f,.51f,.16f,.72f,.27f, .65f,1.18f,-4.5f,.46f,.72f, D|M|R,1,7,21,.46f,.36f,-.08f,.13f,.13f,1}, // Vocal Nebula
{.39f,.09f,.68f,.31f,.46f,.12f,.47f,.22f, .62f,1.14f,-3.9f,.42f,.58f, D|M|T,0,6,21,.39f,.00f,-.05f,.08f,.14f,3}, // Frozen Words
{.37f,.17f,.74f,.24f,.38f,.15f,.82f,.17f, .57f,1.18f,-3.8f,.39f,.54f, D|M|X,0,9,21,.40f,.00f,.02f,.18f,.13f,2}, // Spectral Angel
{.56f,.44f,.60f,.18f,.62f,.18f,.31f,.23f, .58f,1.01f,-5.4f,.71f,.39f, D|R|X,2,8,22,.43f,.47f,.10f,.33f,.21f,4}, // Alien Communication
// 21–30 GUITAR: attack control and harmonic tails without washing away picks
{.55f,.18f,.42f,.26f,.37f,.17f,.59f,.20f, .63f,1.14f,-3.5f,.51f,.51f, D|T|X,0,8,2,.45f,.00f,.00f,.21f,.19f,1}, // Liquid Guitar Dream
{.43f,.13f,.71f,.25f,.41f,.19f,.44f,.13f, .48f,1.03f,-3.1f,.49f,.35f, D|X,0,8,3,.47f,.00f,.16f,.28f,.12f,0}, // Crystal Strings
{.73f,.44f,.65f,.20f,.45f,.23f,.41f,.18f, .61f,1.00f,-5.9f,.83f,.40f, D|X,2,8,3,.38f,.00f,.23f,.39f,.17f,0}, // Electric Guitar Storm
{.43f,.12f,.48f,.27f,.47f,.18f,.61f,.26f, .67f,1.19f,-4.7f,.38f,.76f, D|M|T,1,7,21,.34f,.00f,-.04f,.17f,.15f,1}, // Ambient Guitar Clouds
{.47f,.21f,.57f,.22f,.34f,.13f,.42f,.17f, .48f,.99f,-3.9f,.62f,.38f, D|X,0,8,6,.42f,.00f,.20f,.25f,.17f,0}, // Metallic Strings
{.42f,.19f,.81f,.18f,.37f,.14f,.69f,.15f, .51f,1.10f,-3.4f,.49f,.47f, D|T,0,9,3,.38f,.00f,.10f,.17f,.12f,0}, // Luminous Guitar
{.52f,.13f,.41f,.26f,.47f,.11f,.60f,.22f, .71f,1.24f,-4.8f,.43f,.81f, D|M|T,1,9,21,.40f,.00f,-.11f,.14f,.13f,3}, // Infinite Guitar Pad
{.56f,.14f,.45f,.24f,.55f,.15f,.58f,.19f, .64f,1.20f,-4.0f,.48f,.70f, D|M|R|T,0,7,21,.37f,.27f,-.05f,.09f,.15f,1}, // Oceanic Guitar
{.44f,.32f,.80f,.20f,.54f,.27f,.49f,.19f, .65f,1.19f,-4.8f,.63f,.47f, D|M|T|X,2,10,21,.51f,.00f,.10f,.23f,.17f,4}, // Cosmic Guitar
{.41f,.09f,.75f,.38f,.38f,.14f,.82f,.16f, .65f,1.14f,-4.1f,.35f,.56f, D|M,0,8,21,.43f,.00f,-.03f,.10f,.12f,2}, // Frozen Harmonics
// 31–40 DRUMS: attack, punch and rhythmic transient modulation, controlled tails
{.48f,.63f,.34f,.15f,.33f,.20f,.41f,.10f, .56f,.98f,-4.3f,.62f,.37f, D|X,2,8,2,.37f,.00f,.18f,.63f,.22f,0}, // Liquid Drums
{.44f,.35f,.73f,.16f,.28f,.11f,.52f,.11f, .49f,.99f,-4.7f,.72f,.29f, D|X,2,8,3,.49f,.00f,.22f,.34f,.18f,0}, // Crystal Percussion
{.68f,.67f,.65f,.11f,.44f,.20f,.39f,.13f, .59f,.99f,-6.1f,.82f,.44f, D|X,2,8,1,.45f,.00f,.29f,.57f,.23f,0}, // Plasma Beats
{.51f,.43f,.47f,.19f,.41f,.15f,.62f,.17f, .56f,1.04f,-4.7f,.68f,.67f, D|T|X,2,8,2,.32f,.00f,.11f,.40f,.18f,0}, // Vapor Rhythm
{.62f,.40f,.49f,.17f,.49f,.24f,.68f,.12f, .66f,.98f,-6.0f,.83f,.49f, D|R|X,2,8,21,.44f,.54f,.26f,.34f,.21f,2}, // Metal Machine
{.46f,.33f,.88f,.12f,.35f,.13f,.57f,.11f, .45f,1.01f,-4.2f,.62f,.30f, D|X,2,8,3,.39f,.00f,.21f,.35f,.22f,0}, // Light Impulses
{.67f,.52f,.58f,.19f,.55f,.41f,.44f,.16f, .65f,1.04f,-6.4f,.85f,.44f, D|M|R|X,2,8,22,.44f,.49f,.28f,.46f,.17f,4}, // Quantum Drums
{.57f,.56f,.37f,.21f,.46f,.31f,.62f,.16f, .58f,1.08f,-5.2f,.69f,.61f, D|M|T,2,8,21,.39f,.00f,.14f,.51f,.17f,1}, // Fluid Breaks
{.58f,.39f,.56f,.23f,.47f,.16f,.73f,.22f, .62f,.99f,-6.2f,.84f,.43f, D|M|R,2,8,3,.44f,.58f,.25f,.35f,.19f,2}, // Resonant Impacts
{.69f,.75f,.64f,.14f,.36f,.30f,.43f,.11f, .66f,.97f,-6.6f,.84f,.36f, D|X|T,2,8,2,.45f,.00f,.31f,.68f,.27f,0}, // Electric Pulse Factory
// 41–50 SYNTH: rich modulation and evolving coupled oscillations
{.59f,.29f,.47f,.27f,.49f,.23f,.71f,.19f, .72f,1.17f,-4.0f,.59f,.68f, D|M|T|X,1,1,2,.45f,.00f,.04f,.25f,.21f,1}, // Liquid Synth Evolution
{.51f,.44f,.72f,.18f,.38f,.31f,.47f,.12f, .62f,1.08f,-3.9f,.67f,.46f, D|X|T,2,1,3,.44f,.00f,.13f,.31f,.23f,0}, // Crystal Arpeggios
{.77f,.58f,.71f,.13f,.46f,.21f,.50f,.17f, .65f,1.03f,-6.1f,.88f,.41f, D|X|R,2,8,3,.43f,.52f,.24f,.55f,.26f,4}, // Plasma Synth Reactor
{.51f,.16f,.41f,.33f,.49f,.16f,.65f,.22f, .77f,1.31f,-4.4f,.53f,.82f, D|M|T|X,1,9,5,.41f,.00f,-.04f,.14f,.21f,1}, // Vapor Synth Clouds
{.58f,.40f,.59f,.22f,.48f,.23f,.42f,.18f, .65f,1.10f,-5.6f,.77f,.46f, D|X,2,1,6,.38f,.00f,.15f,.41f,.23f,2}, // Metallic Oscillations
{.44f,.28f,.85f,.19f,.41f,.21f,.68f,.17f, .63f,1.21f,-4.3f,.59f,.55f, D|X|M,0,9,21,.43f,.00f,.08f,.26f,.18f,2}, // Spectral Illumination
{.63f,.35f,.55f,.21f,.59f,.26f,.81f,.17f, .73f,1.22f,-5.7f,.77f,.50f, D|M|R|T|X,2,12,37,.48f,.37f,.18f,.23f,.23f,4}, // Future Dimension
{.49f,.17f,.78f,.23f,.55f,.29f,.41f,.19f, .61f,1.09f,-4.5f,.57f,.40f, D|R|M,0,1,3,.46f,.28f,.07f,.19f,.14f,3}, // Glasswave Synth
{.50f,.19f,.58f,.28f,.53f,.15f,.83f,.21f, .75f,1.28f,-4.6f,.57f,.80f, D|M|T|X,1,9,21,.47f,.00f,.02f,.17f,.20f,1}, // Nebula Synth Engine
{.64f,.42f,.36f,.26f,.58f,.31f,.61f,.16f, .68f,1.17f,-4.9f,.70f,.57f, D|M|R|X,1,7,21,.40f,.41f,.13f,.32f,.19f,4}, // Synthetic Organism
// 51–60 BASS: protected low band, narrow image and restrained feedback
{.44f,.52f,.25f,.12f,.32f,.13f,.46f,.08f, .47f,.92f,-4.8f,.58f,.36f, D|X,2,1,2,.28f,.00f,.12f,.42f,.16f,0}, // Liquid Sub Motion
{.43f,.20f,.49f,.13f,.30f,.11f,.53f,.10f, .42f,.91f,-4.9f,.52f,.32f, D|X,0,8,3,.35f,.00f,.09f,.19f,.10f,0}, // Crystal Sub Resonance
{.66f,.49f,.38f,.12f,.43f,.24f,.44f,.10f, .47f,.88f,-7.1f,.74f,.40f, D|X,2,8,3,.27f,.00f,.19f,.46f,.17f,0}, // Plasma Bass Reactor
{.38f,.15f,.24f,.19f,.44f,.13f,.39f,.16f, .39f,.93f,-5.3f,.37f,.56f, D|X,1,7,3,.26f,.00f,-.04f,.14f,.11f,0}, // Vapor Low Atmosphere
{.56f,.36f,.38f,.16f,.35f,.18f,.63f,.10f, .48f,.90f,-6.0f,.67f,.44f, D|X,2,8,3,.34f,.00f,.16f,.35f,.14f,0}, // Metallic Bass Engine
{.39f,.22f,.65f,.11f,.29f,.16f,.73f,.12f, .40f,.94f,-5.2f,.56f,.29f, D|X,0,9,3,.33f,.00f,.10f,.24f,.13f,0}, // Luminous Bass Harmonics
{.68f,.45f,.33f,.17f,.55f,.28f,.48f,.12f, .54f,.91f,-7.7f,.85f,.36f, D|R|X,2,8,3,.36f,.42f,.23f,.35f,.18f,4}, // Quantum Sub Pressure
{.53f,.34f,.31f,.14f,.40f,.20f,.74f,.12f, .49f,.95f,-5.7f,.63f,.41f, D|M|T,0,1,2,.30f,.00f,.10f,.20f,.13f,3}, // Fluid Bass Movement
{.69f,.38f,.27f,.17f,.39f,.27f,.42f,.12f, .52f,.88f,-7.4f,.81f,.39f, D|X,2,8,3,.39f,.00f,.20f,.41f,.18f,0}, // Dark Energy Bass
{.47f,.24f,.29f,.16f,.46f,.15f,.48f,.12f, .43f,.96f,-5.9f,.56f,.44f, D|M|T,0,7,21,.27f,.00f,-.03f,.15f,.12f,3}, // Evolving Bass Matter
// 61–70 AMBIENT: expressive slow shifts with deliberately soft output
{.28f,.13f,.27f,.27f,.26f,.11f,.45f,.15f, .53f,1.12f,-2.0f,.27f,.51f, D|M|T,0,7,2,.25f,.00f,-.12f,.08f,.12f,3}, // Peaceful Water
{.29f,.09f,.62f,.23f,.26f,.10f,.47f,.16f, .57f,1.17f,-2.7f,.27f,.48f, D|M,0,6,3,.28f,.00f,-.10f,.09f,.11f,3}, // Crystal Meditation
{.35f,.12f,.37f,.26f,.34f,.09f,.66f,.19f, .61f,1.23f,-3.2f,.30f,.75f, D|M|T,1,9,21,.30f,.00f,-.12f,.11f,.10f,1}, // Vapor Dreams
{.27f,.14f,.78f,.11f,.22f,.11f,.59f,.11f, .46f,1.10f,-2.5f,.25f,.45f, D|X,0,9,3,.23f,.00f,-.06f,.14f,.09f,0}, // Soft Light
{.36f,.12f,.36f,.24f,.30f,.13f,.58f,.15f, .60f,1.20f,-2.9f,.33f,.58f, D|M|T,1,7,2,.30f,.00f,-.11f,.12f,.13f,2}, // Floating Memories
{.30f,.07f,.43f,.32f,.29f,.09f,.62f,.21f, .61f,1.24f,-3.0f,.27f,.73f, D|M|T,0,6,21,.30f,.00f,-.15f,.06f,.11f,3}, // Ocean of Silence
{.34f,.16f,.76f,.17f,.29f,.12f,.49f,.13f, .57f,1.23f,-3.3f,.35f,.56f, D|M|T,0,9,21,.35f,.00f,-.04f,.13f,.12f,2}, // Aurora Atmosphere
{.33f,.21f,.46f,.25f,.36f,.13f,.77f,.13f, .57f,1.27f,-3.1f,.32f,.73f, D|M|X,1,11,21,.31f,.00f,-.04f,.17f,.09f,1}, // Dreaming Particles
{.37f,.19f,.31f,.22f,.32f,.12f,.52f,.13f, .55f,1.16f,-3.0f,.34f,.53f, D|M|R,0,7,21,.28f,.19f,-.09f,.11f,.10f,3}, // Ethereal Flow
{.27f,.08f,.39f,.34f,.25f,.09f,.57f,.18f, .64f,1.28f,-2.7f,.23f,.81f, D|M|T,1,7,21,.32f,.00f,-.10f,.07f,.08f,3}, // Endless Calm
// 71–80 EXPERIMENTAL: deliberately vivid but feedback/output bounded
{.75f,.61f,.38f,.22f,.65f,.41f,.55f,.17f, .69f,1.13f,-7.2f,.88f,.46f, D|M|R|X,2,11,3,.43f,.51f,.26f,.44f,.28f,4}, // Matter Collapse
{.66f,.51f,.81f,.25f,.57f,.40f,.42f,.18f, .62f,1.08f,-6.7f,.84f,.43f, D|M|R|X,2,8,21,.42f,.57f,.28f,.50f,.22f,4}, // Crystal Destruction
{.70f,.70f,.66f,.18f,.46f,.29f,.58f,.11f, .71f,1.05f,-6.9f,.78f,.40f, D|X|T,2,12,3,.39f,.00f,.23f,.64f,.30f,0}, // Quantum Glitch
{.73f,.50f,.47f,.21f,.60f,.34f,.73f,.15f, .65f,1.06f,-7.5f,.87f,.46f, D|M|R|X,2,11,22,.40f,.49f,.24f,.40f,.26f,4}, // Metallic Chaos
{.58f,.62f,.35f,.24f,.43f,.36f,.61f,.15f, .65f,1.13f,-5.8f,.75f,.54f, D|M|T|X,2,12,2,.45f,.00f,.16f,.54f,.22f,1}, // Liquid Mutation
{.58f,.40f,.52f,.27f,.47f,.32f,.64f,.18f, .70f,1.25f,-5.5f,.69f,.78f, D|M|T|X,2,11,21,.42f,.00f,.14f,.38f,.24f,2}, // Vapor Fragments
{.76f,.55f,.83f,.21f,.56f,.23f,.65f,.13f, .68f,1.15f,-7.0f,.88f,.38f, D|M|R|X,2,8,3,.44f,.45f,.30f,.51f,.24f,4}, // Spectral Corruption
{.72f,.43f,.55f,.27f,.64f,.27f,.39f,.19f, .70f,1.16f,-6.3f,.81f,.57f, D|M|R|T,2,11,22,.43f,.50f,.22f,.37f,.20f,2}, // Broken Reality
{.74f,.68f,.73f,.16f,.59f,.32f,.52f,.12f, .62f,1.06f,-7.4f,.89f,.36f, D|R|X,2,8,3,.43f,.51f,.31f,.67f,.28f,4}, // Particle Explosion
{.70f,.52f,.40f,.22f,.61f,.37f,.64f,.17f, .67f,1.15f,-6.6f,.77f,.55f, D|M|R|X,2,10,3,.40f,.47f,.16f,.44f,.25f,4}, // Digital Organism
// 81–90 RHYTHMIC: tempo-linked movements and precise transient response
{.50f,.55f,.39f,.15f,.39f,.21f,.62f,.12f, .61f,1.04f,-4.7f,.63f,.43f, D|M|T|X,2,1,2,.36f,.00f,.13f,.45f,.20f,1}, // Liquid Groove
{.46f,.43f,.73f,.18f,.39f,.17f,.54f,.13f, .57f,1.03f,-4.4f,.65f,.36f, D|X|T,2,8,3,.42f,.00f,.15f,.37f,.20f,0}, // Crystal Sequence
{.71f,.66f,.62f,.13f,.44f,.33f,.42f,.10f, .64f,.99f,-6.3f,.79f,.45f, D|X|T,2,1,3,.41f,.00f,.24f,.61f,.24f,0}, // Plasma Pulsation
{.49f,.41f,.51f,.20f,.41f,.29f,.63f,.14f, .62f,1.12f,-4.9f,.60f,.67f, D|M|T,2,1,21,.36f,.00f,.08f,.31f,.19f,2}, // Vapor Motion
{.63f,.49f,.52f,.18f,.45f,.27f,.70f,.13f, .62f,1.05f,-6.0f,.83f,.42f, D|R|X,2,8,3,.41f,.53f,.22f,.44f,.21f,4}, // Metallic Rhythm
{.55f,.58f,.84f,.14f,.39f,.24f,.55f,.11f, .56f,1.10f,-4.5f,.67f,.46f, D|X|T,2,1,3,.37f,.00f,.16f,.53f,.22f,0}, // Lightwave Sequencer
{.62f,.55f,.31f,.21f,.57f,.27f,.49f,.13f, .65f,1.12f,-5.5f,.75f,.40f, D|M|R|T,2,8,21,.42f,.43f,.14f,.49f,.20f,4}, // Gravity Groove
{.73f,.62f,.61f,.13f,.51f,.34f,.78f,.11f, .65f,1.09f,-6.6f,.81f,.45f, D|M|T|X,2,12,3,.42f,.00f,.19f,.59f,.25f,4}, // Quantum Motion
{.56f,.40f,.43f,.17f,.48f,.36f,.66f,.15f, .58f,1.20f,-5.0f,.66f,.54f, D|M|T|X,0,1,40,.39f,.00f,.10f,.38f,.19f,1}, // Stereo Matter Dance
{.52f,.37f,.78f,.21f,.41f,.22f,.69f,.15f, .62f,1.12f,-5.0f,.69f,.45f, D|M|T|X,2,6,21,.39f,.00f,.12f,.32f,.16f,2}, // Harmonic Motion Engine
// 91–100 MIX ENHANCEMENT: subtle credible wet/dry, restrained stereo image
{.28f,.15f,.32f,.11f,.25f,.11f,.52f,.08f, .25f,1.03f,-1.1f,.37f,.33f, D|X,0,7,2,.18f,.00f,-.08f,.17f,.08f,0}, // Silky Motion
{.32f,.11f,.64f,.12f,.27f,.09f,.44f,.08f, .24f,1.00f,-1.1f,.41f,.29f, D|X,0,8,3,.20f,.00f,.04f,.13f,.08f,0}, // Crystal Presence
{.31f,.17f,.37f,.10f,.26f,.13f,.52f,.08f, .23f,1.00f,-1.5f,.37f,.29f, D|X,0,7,3,.18f,.00f,-.04f,.17f,.06f,0}, // Plasma Warmth
{.26f,.10f,.48f,.16f,.24f,.08f,.56f,.09f, .29f,1.07f,-1.1f,.29f,.45f, D|X,0,9,21,.22f,.00f,-.09f,.12f,.08f,0}, // Atmospheric Depth
{.35f,.17f,.51f,.10f,.25f,.12f,.65f,.08f, .25f,1.00f,-1.7f,.50f,.32f, D|X,0,8,5,.25f,.00f,.12f,.20f,.08f,0}, // Metallic Definition
{.28f,.16f,.78f,.11f,.26f,.12f,.57f,.07f, .21f,1.04f,-1.3f,.38f,.24f, D|X,0,9,3,.24f,.00f,.05f,.16f,.06f,0}, // Luminous Air
{.32f,.17f,.42f,.13f,.26f,.13f,.73f,.09f, .28f,1.11f,-1.5f,.40f,.39f, D|M|T,0,7,40,.24f,.00f,-.03f,.16f,.08f,3}, // Stereo Fluidity
{.31f,.11f,.70f,.13f,.24f,.12f,.79f,.10f, .24f,1.04f,-1.6f,.42f,.29f, D|M,0,8,3,.24f,.00f,.05f,.12f,.07f,3}, // Harmonic Polish
{.33f,.15f,.37f,.14f,.31f,.12f,.58f,.10f, .28f,1.10f,-1.4f,.43f,.41f, D|M|T,0,7,21,.24f,.00f,-.03f,.14f,.08f,3}, // Organic Dimension
{.44f,.23f,.53f,.18f,.39f,.17f,.73f,.13f, .38f,1.15f,-3.0f,.51f,.45f, D|M|T|R|X,1,9,21,.33f,.27f,.08f,.20f,.12f,4}  // Ultimate Organix — tasteful showcase all engines

}};
static_assert(factorySoundDesigns.size()==factoryPresets.size(),
              "Every historical factory name must have exactly one design");
} // namespace organix
