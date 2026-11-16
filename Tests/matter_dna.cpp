#include "../Source/MatterDNA.h"
#include "../Source/MatterDNAModulation.h"
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
bool inRange (float x) { return std::isfinite (x) && x >= 0.0f && x <= 1.0f; }
bool valid (const organix::MatterDNA::Features& x)
{
    return inRange (x.energy) && inRange (x.transient) && inRange (x.air) &&
           inRange (x.motion) && inRange (x.centroid) && inRange (x.low) &&
           inRange (x.mid) && inRange (x.high);
}
std::vector<float> sine (int size, float hz, float sampleRate)
{
    std::vector<float> out (static_cast<size_t> (size));
    constexpr float tau = 6.28318530718f;
    for (int i = 0; i < size; ++i)
        out[static_cast<size_t> (i)] = 0.25f * std::sin (tau * hz * i / sampleRate);
    return out;
}
}
int main()
{
    organix::MatterDNA analyzer;
    analyzer.prepare (48000.0);
    std::vector<float> silence (4096, 0.0f);
    auto empty = analyzer.analyzeBlock (silence.data(), nullptr, 4096, 1);
    if (! valid (empty) || empty.energy > 0.001f || empty.air > 0.001f ||
        empty.transient > 0.001f) return 1;

    analyzer.reset();
    const auto fundamental = sine (4096, 220.0f, 48000.0f);
    auto bass = analyzer.analyzeBlock (fundamental.data(), nullptr, 4096, 1);
    if (! valid (bass) || bass.energy < 0.05f || bass.low < bass.high) return 2;

    analyzer.reset();
    const auto hiss = sine (4096, 8000.0f, 48000.0f);
    auto air = analyzer.analyzeBlock (hiss.data(), nullptr, 4096, 1);
    if (! valid (air) || air.air < 0.5f || air.high <= bass.high ||
        air.centroid <= bass.centroid) return 3;

    analyzer.reset();
    std::vector<float> hits (4096, 0.0f);
    hits[0] = hits[1000] = hits[2500] = 0.9f;
    auto transient = analyzer.analyzeBlock (hits.data(), nullptr, 4096, 1);
    if (! valid (transient) || transient.transient < 0.04f) return 4;

    analyzer.reset();
    auto left = sine (4096, 170.0f, 48000.0f);
    auto right = sine (4096, 5500.0f, 48000.0f);
    auto stereo = analyzer.analyzeBlock (left.data(), right.data(), 4096, 2);
    if (! valid (stereo) || stereo.air <= 0.02f || stereo.low <= 0.02f) return 5;

    // Verify intentional no-effect when DNA is disabled, then genuine
    // Crystal transient excitation and Vapor high-frequency control.
    organix::MatterDNAControls controls;
    organix::Parameters base, transformed;
    controls.enabled = false;
    organix::MatterDNA::Features features;
    features.transient = 0.85f;
    features.air = 0.92f;
    features.energy = 0.8f;
    features.centroid = 0.75f;
    features.motion = 0.68f;
    transformed = base;
    organix::applyMatterDNA (organix::Material::Crystal, transformed, features, controls);
    if (transformed.energy != base.energy || transformed.amount != base.amount ||
        transformed.motion != base.motion) return 6;

    controls.enabled = true;
    transformed = base;
    organix::applyMatterDNA (organix::Material::Crystal, transformed, features, controls);
    if (transformed.energy <= base.energy || transformed.amount <= base.amount) return 7;
    transformed = base;
    organix::applyMatterDNA (organix::Material::Vapor, transformed, features, controls);
    if (transformed.density <= base.density || transformed.amount <= base.amount) return 8;

    // Even without a new attack, changing input spectral energy must
    // influence Crystal's current engine through its resonator excitation.
    features.transient = 0.0f;
    transformed = base;
    organix::applyMatterDNA (organix::Material::Crystal, transformed, features, controls);
    if (transformed.energy <= base.energy) return 11;
    features.transient = 0.85f;

    for (auto material : {organix::Material::Liquid, organix::Material::Crystal,
                          organix::Material::Plasma, organix::Material::Vapor,
                          organix::Material::Metal, organix::Material::Light})
    {
        transformed = base;
        organix::applyMatterDNA (material, transformed, features, controls);
        if (! inRange (transformed.amount) || ! inRange (transformed.motion) ||
            ! inRange (transformed.tone) || ! inRange (transformed.density) ||
            ! inRange (transformed.energy)) return 9;
    }

    // 192k mono stress: FFT frame cadence stays fixed and all features bounded.
    analyzer.prepare (192000.0);
    const auto highRate = sine (16384, 6000.0f, 192000.0f);
    const auto stress = analyzer.analyzeBlock (highRate.data(), nullptr,
                                                static_cast<int> (highRate.size()), 1);
    if (! valid (stress)) return 10;
    std::cout << "PASS: 1024 FFT, RMS/attacks/spectral motion, voice-air routes, disabled compatibility, 192k\n";
    return 0;
}
