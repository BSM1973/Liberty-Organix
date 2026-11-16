#pragma once
#include "MatterDNA.h"
#include "MaterialDSP.h"

namespace organix
{
struct MatterDNAControls
{
    bool enabled = false;
    float depth = 0.5f;
    float sensitivity = 0.5f;
    float crystalAttack = 0.8f;
    float vaporAir = 0.8f;
    float energyMotion = 0.4f;
    float spectralTone = 0.25f;
};

inline void applyMatterDNA (Material material, Parameters& p,
                            MatterDNA::Features features,
                            const MatterDNAControls& controls) noexcept
{
    // Disabled = identical to v0.5 DSP. Clamp all modulation paths before
    // configuring the material engines, which already smooth their own audio.
    if (! controls.enabled || controls.depth <= 0.0f) return;
    const auto safe = [] (float x) noexcept { return std::clamp (x, 0.0f, 1.0f); };
    const float depth = safe (controls.depth) *
                        (0.35f + 1.3f * safe (controls.sensitivity));

    // Motion is the actual input's changing energy + spectral flux.
    // An incoming signal that is quiet and stable does not move on its own.
    const float evolving = safe (features.energy * 0.65f + features.motion * 0.35f);
    p.motion = safe (p.motion +
        depth * safe (controls.energyMotion) * evolving * 0.65f);

    // Spectral brightness maps to material tone; dark/bright recordings yield
    // different tonal behaviours without requiring pretrained AI models.
    const float tilt = std::clamp ((features.centroid - 0.27f) * 1.8f, -0.5f, 0.5f);
    p.tone = safe (p.tone +
        depth * safe (controls.spectralTone) * tilt * 0.5f);

    // A vocal plosive or instrument transient excites the Crystal resonator.
    // MaterialDSP::crystal uses p.energy to scale its excitation.
    if (material == Material::Crystal)
    {
        const float attack = safe (features.transient);
        p.energy = safe (p.energy +
            depth * safe (controls.crystalAttack) * attack * 0.8f);
        p.amount = safe (p.amount +
            depth * safe (controls.crystalAttack) * attack * 0.30f);
    }
    // Breath, sibilants and airy textures make Vapor more granular/diffuse.
    // The FFT high-band ratio is gated by the input level in MatterDNA.
    if (material == Material::Vapor)
    {
        const float air = safe (features.air);
        p.density = safe (p.density +
            depth * safe (controls.vaporAir) * air * 0.75f);
        p.amount = safe (p.amount +
            depth * safe (controls.vaporAir) * air * 0.40f);
    }
}
}
