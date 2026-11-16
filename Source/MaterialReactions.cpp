#include "MaterialReactions.h"
#include <algorithm>
#include <cmath>

namespace organix
{
namespace
{
float safe (float value) noexcept
{
    return std::isfinite (value) ? std::clamp (value, 0.0f, 1.0f) : 0.0f;
}
float signedSafe (float value) noexcept
{
    return std::isfinite (value) ? std::clamp (value, -1.0f, 1.0f) : 0.0f;
}
int index (Material m) noexcept
{
    return std::clamp (static_cast<int> (m), 0, 5);
}
struct NamePair { const char* fr; const char* en; };
constexpr NamePair names[6][6] =
{
    {{"Courant liquide", "Liquid current"}, {"Cristaux fluides", "Fluid crystals"},
     {"Plasma ralenti", "Slowed plasma"}, {"Nuage ondulant", "Rolling cloud"},
     {"Métal submergé", "Submerged metal"}, {"Halo aquatique", "Aquatic halo"}},
    {{"Gouttes cristallines", "Crystalline drops"}, {"Résonance jumelle", "Twin resonance"},
     {"Étincelles tonales", "Tonal sparks"}, {"Brume de cristal", "Crystal mist"},
     {"Impact harmonique", "Harmonic impact"}, {"Prisme lumineux", "Light prism"}},
    {{"Ébullition", "Boiling flow"}, {"Éclat électrique", "Electric sparkle"},
     {"Arc en chaîne", "Chain arc"}, {"Vapeur ionisée", "Ionized vapor"},
     {"Forge électrique", "Electric forge"}, {"Éclair diffus", "Diffused lightning"}},
    {{"Gouttes de brume", "Mist drops"}, {"Cristal en suspension", "Suspended crystal"},
     {"Décharge voilée", "Veiled discharge"}, {"Souffle croisé", "Cross-breath"},
     {"Métal vaporeux", "Vapor metal"}, {"Halo diffus", "Diffused halo"}},
    {{"Vagues métalliques", "Metal waves"}, {"Cristal percuté", "Struck crystal"},
     {"Arc percussif", "Percussive arc"}, {"Nuage de débris", "Debris cloud"},
     {"Résonance de forge", "Forge resonance"}, {"Lueurs d'impact", "Impact light"}},
    {{"Flux lumineux", "Luminous flow"}, {"Résonance brillante", "Brilliant resonance"},
     {"Plasma irradié", "Irradiated plasma"}, {"Brume irisée", "Iridescent mist"},
     {"Impact spectral", "Spectral impact"}, {"Halo en cascade", "Cascade halo"}}
};
constexpr float intensity[6][6] =
{
    {0.71f,0.92f,1.00f,0.80f,0.70f,0.91f},
    {0.72f,0.85f,0.84f,0.92f,0.95f,0.86f},
    {0.89f,0.96f,0.80f,0.73f,1.00f,0.85f},
    {0.77f,0.72f,0.78f,0.82f,0.70f,1.00f},
    {0.84f,1.00f,0.93f,0.80f,0.87f,0.90f},
    {0.93f,0.89f,0.85f,0.97f,0.77f,0.74f}
};
float sourceActivity (Material m, float body, float attack, float air) noexcept
{
    switch (m)
    {
        case Material::Liquid:  return safe (body * 2.5f);
        case Material::Crystal: return safe (body * 1.65f + attack * 0.75f);
        case Material::Plasma:  return safe (air * 2.3f + attack * 0.35f);
        case Material::Vapor:   return safe (body * 1.6f + air * 0.55f);
        case Material::Metal:   return safe (attack * 2.2f + air * 0.35f);
        case Material::Light:   return safe (air * 1.7f + body * 0.65f);
    }
    return 0.0f;
}
}
ReactionDescription MaterialReactions::description (Material source,
                                                      Material dest) noexcept
{
    const auto& name = names[index (source)][index (dest)];
    return {name.fr, name.en};
}
ReactionDrive MaterialReactions::drive (Material source, Material dest,
                                       float body, float attack, float air,
                                       float signedEdge, float amount) noexcept
{
    const float active = sourceActivity (source, safe (body), safe (attack), safe (air));
    const float force = safe (amount) * intensity[index (source)][index (dest)];
    const float modulation = force * active;
    const float signedImpulse = signedSafe (signedEdge * 4.0f) * force *
                                (0.28f + 0.72f * safe (attack * 4.0f));
    ReactionDrive result;
    switch (dest)
    {
        case Material::Liquid:
            result.slow = modulation * 0.70f;
            result.colour = modulation * 0.20f;
            break;
        case Material::Crystal:
            result.excite = signedImpulse * (source == Material::Metal ? 0.90f : 0.52f);
            result.colour = modulation * 0.12f;
            break;
        case Material::Plasma:
            result.slow = modulation * (source == Material::Liquid ? 0.86f : 0.46f);
            result.colour = modulation * 0.32f;
            break;
        case Material::Vapor:
            result.disperse = modulation * 0.90f;
            result.colour = modulation * 0.20f;
            break;
        case Material::Metal:
            result.excite = signedImpulse * 0.64f;
            result.colour = modulation * 0.24f;
            break;
        case Material::Light:
            result.disperse = modulation * (source == Material::Vapor ? 0.92f : 0.61f);
            result.colour = modulation * 0.14f;
            break;
    }
    return result;
}
void MaterialReactionLink::Detector::feed (float sample, float alpha) noexcept
{
    const float x = signedSafe (sample * 0.25f) * 4.0f;
    const float absolute = std::abs (x);
    const float delta = x - previous;
    previous = x;
    const float highEnergy = std::abs (delta);
    edge = signedSafe (delta);
    // Air/edge and transient react quickly, body follows more slowly.
    const float fast = std::clamp (alpha * 10.0f, 0.002f, 0.40f);
    const float envelope = std::clamp (alpha, 0.00002f, 0.25f);
    const float onset = std::max (0.0f, absolute - body);
    body += envelope * (absolute - body);
    high += fast * (highEnergy - high);
    attack += fast * (onset - attack);
    body = safe (body);
    high = safe (high);
    attack = safe (attack);
}
void MaterialReactionLink::prepare (double fs) noexcept
{
    rate = std::isfinite (fs) ? std::clamp (static_cast<float> (fs), 8000.0f, 384000.0f)
                              : 48000.0f;
    reset();
}
void MaterialReactionLink::reset() noexcept
{
    state = {};
    meterAB = meterBA = 0.0f;
}
float MaterialReactionLink::process (MaterialDSP& a, MaterialDSP& b,
                                     Material materialA, Material materialB,
                                     int routing, float balance, float fusion,
                                     int channel, float input,
                                     const ReactionControls& controls) noexcept
{
    if (channel < 0 || channel >= 2) return std::isfinite (input) ? input : 0.0f;
    auto& s = state[static_cast<size_t> (channel)];
    const int mode = std::clamp (controls.direction, 0, 2);
    const float depth = controls.enabled ? safe (controls.amount) : 0.0f;
    const float targetAB = mode == 1 ? 0.0f : depth;
    const float targetBA = mode == 0 ? 0.0f : depth;
    const float response = safe (controls.response);
    const float smoothAlpha = 1.0f - std::exp (-1.0f / (rate * 0.016f));
    s.amountAB += smoothAlpha * (targetAB - s.amountAB);
    s.amountBA += smoothAlpha * (targetBA - s.amountBA);

    // Cross-control uses outputs remembered from previous samples, so the
    // bidirectional connection has no zero-delay algebraic feedback loop.
    const auto driveA = MaterialReactions::drive (
        materialB, materialA, s.b.body, s.b.attack, s.b.high,
        s.b.edge, s.amountBA);
    const auto driveB = MaterialReactions::drive (
        materialA, materialB, s.a.body, s.a.attack, s.a.high,
        s.a.edge, s.amountAB);

    const float x = std::isfinite (input) ? input : 0.0f;
    const int route = std::clamp (routing, 0, 3);
    float outA = 0.0f, outB = 0.0f;
    if (route == 2)
    {
        outB = b.process (channel, x, driveB);
        outA = a.process (channel, outB, driveA);
    }
    else if (route == 1)
    {
        outA = a.process (channel, x, driveA);
        outB = b.process (channel, outA, driveB);
    }
    else
    {
        outA = a.process (channel, x, driveA);
        outB = b.process (channel, x, driveB);
    }
    const float detectorAlpha = 1.0f - std::exp (
        -1.0f / (rate * (0.005f + response * 0.090f)));
    s.a.feed (outA, detectorAlpha);
    s.b.feed (outB, detectorAlpha);
    meterAB = std::max (meterAB * 0.9995f, safe (s.amountAB *
        sourceActivity (materialA, s.a.body, s.a.attack, s.a.high)));
    meterBA = std::max (meterBA * 0.9995f, safe (s.amountBA *
        sourceActivity (materialB, s.b.body, s.b.attack, s.b.high)));

    float y;
    if (route == 1) y = outB;
    else if (route == 2) y = outA;
    else
    {
        y = (1.0f - safe (balance)) * outA + safe (balance) * outB;
        if (route == 3)
            y += safe (fusion) * 0.27f * std::tanh (3.0f * outA * outB);
    }
    return std::isfinite (y) ? std::clamp (y, -8.0f, 8.0f) : 0.0f;
}
}
