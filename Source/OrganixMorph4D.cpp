#include "OrganixMorph4D.h"
#include <algorithm>
#include <cmath>

namespace organix
{
namespace
{
float finite (float x, float lo, float hi, float fallback) noexcept
{
    return std::isfinite (x) ? std::clamp (x, lo, hi) : fallback;
}
bool different (const MorphScene& a, const MorphScene& b) noexcept
{
    return a.pack() != b.pack();
}
}

std::array<float, MorphScene::packedCount> MorphScene::pack() const noexcept
{
    std::array<float, packedCount> result {};
    int n = 0;
    result[n++] = static_cast<float> (materialA);
    result[n++] = static_cast<float> (materialB);
    result[n++] = static_cast<float> (routing);
    result[n++] = enableB ? 1.0f : 0.0f;
    const auto write = [&] (const Parameters& p, auto& out, int& index) noexcept
    {
        out[static_cast<size_t> (index++)] = p.amount;
        out[static_cast<size_t> (index++)] = p.motion;
        out[static_cast<size_t> (index++)] = p.tone;
        out[static_cast<size_t> (index++)] = p.feedback;
        out[static_cast<size_t> (index++)] = p.density;
        out[static_cast<size_t> (index++)] = p.energy;
        for (float v : p.shape) out[static_cast<size_t> (index++)] = v;
    };
    write (a, result, n);
    write (b, result, n);
    result[n++] = fusion;
    result[n++] = balance;
    result[n++] = wet;
    result[n++] = width;
    return result;
}

MorphScene MorphScene::unpack (const std::array<float, packedCount>& values) noexcept
{
    MorphScene scene;
    int n = 0;
    scene.materialA = static_cast<int> (finite (values[n++], 0.0f, 5.0f, 0.0f));
    scene.materialB = static_cast<int> (finite (values[n++], 0.0f, 5.0f, 1.0f));
    scene.routing = static_cast<int> (finite (values[n++], 0.0f, 3.0f, 0.0f));
    scene.enableB = finite (values[n++], 0.0f, 1.0f, 1.0f) > 0.5f;
    const auto read = [&] (Parameters& p, const auto& from, int& index) noexcept
    {
        p.amount = from[static_cast<size_t> (index++)];
        p.motion = from[static_cast<size_t> (index++)];
        p.tone = from[static_cast<size_t> (index++)];
        p.feedback = from[static_cast<size_t> (index++)];
        p.density = from[static_cast<size_t> (index++)];
        p.energy = from[static_cast<size_t> (index++)];
        for (float& v : p.shape) v = from[static_cast<size_t> (index++)];
    };
    read (scene.a, values, n);
    read (scene.b, values, n);
    scene.fusion = values[static_cast<size_t> (n++)];
    scene.balance = values[static_cast<size_t> (n++)];
    scene.wet = values[static_cast<size_t> (n++)];
    scene.width = values[static_cast<size_t> (n++)];
    scene.sanitize();
    return scene;
}
void MorphScene::sanitize() noexcept
{
    materialA = std::clamp (materialA, 0, 5);
    materialB = std::clamp (materialB, 0, 5);
    routing = std::clamp (routing, 0, 3);
    const auto fix = [] (Parameters& p) noexcept
    {
        p.amount = finite (p.amount, 0.0f, 1.0f, 0.5f);
        p.motion = finite (p.motion, 0.0f, 1.0f, 0.5f);
        p.tone = finite (p.tone, 0.0f, 1.0f, 0.5f);
        p.feedback = finite (p.feedback, 0.0f, 0.88f, 0.2f);
        p.density = finite (p.density, 0.0f, 1.0f, 0.5f);
        p.energy = finite (p.energy, 0.0f, 1.0f, 0.5f);
        for (auto& v : p.shape) v = finite (v, 0.0f, 1.0f, 0.5f);
    };
    fix (a);
    fix (b);
    fusion = finite (fusion, 0.0f, 1.0f, 0.3f);
    balance = finite (balance, 0.0f, 1.0f, 0.5f);
    wet = finite (wet, 0.0f, 1.0f, 0.65f);
    width = finite (width, 0.0f, 2.0f, 1.0f);
}

std::array<float,4> Morph4D::weights (float x, float y) noexcept
{
    x = finite (x, 0.0f, 1.0f, 0.0f);
    y = finite (y, 0.0f, 1.0f, 0.0f);
    return {{ (1.0f - x) * (1.0f - y), x * (1.0f - y),
              (1.0f - x) * y, x * y }};
}
MorphScene Morph4D::interpolate (const std::array<MorphScene,4>& scenes,
                                 const std::array<float,4>& weights) noexcept
{
    MorphScene result;
    // Discrete engine identities are deliberately NOT interpolated. Each
    // discrete scene has its own engines, and its audio is crossfaded instead.
    auto combine = [&] (auto get) noexcept
    {
        float sum = 0.0f;
        for (int i = 0; i < 4; ++i)
            sum += weights[static_cast<size_t> (i)] * get (scenes[static_cast<size_t> (i)]);
        return sum;
    };
    auto blendParams = [&] (Parameters& out, bool isA) noexcept
    {
        out.amount = combine ([&] (const MorphScene& s) { return (isA ? s.a : s.b).amount; });
        out.motion = combine ([&] (const MorphScene& s) { return (isA ? s.a : s.b).motion; });
        out.tone = combine ([&] (const MorphScene& s) { return (isA ? s.a : s.b).tone; });
        out.feedback = combine ([&] (const MorphScene& s) { return (isA ? s.a : s.b).feedback; });
        out.density = combine ([&] (const MorphScene& s) { return (isA ? s.a : s.b).density; });
        out.energy = combine ([&] (const MorphScene& s) { return (isA ? s.a : s.b).energy; });
        for (int j = 0; j < 12; ++j)
            out.shape[static_cast<size_t> (j)] = combine ([&] (const MorphScene& s)
            {
                return (isA ? s.a : s.b).shape[static_cast<size_t> (j)];
            });
    };
    blendParams (result.a, true);
    blendParams (result.b, false);
    result.fusion = combine ([] (const MorphScene& s) { return s.fusion; });
    result.balance = combine ([] (const MorphScene& s) { return s.balance; });
    result.wet = combine ([] (const MorphScene& s) { return s.wet; });
    result.width = combine ([] (const MorphScene& s) { return s.width; });
    result.sanitize();
    return result;
}

void MorphRender::prepare (double sampleRate, int channels)
{
    fadeSamples = std::max (128, static_cast<int> (std::ceil (sampleRate * 0.085)));
    for (auto& scene : engines)
        for (auto& pair : scene)
        {
            pair.a.prepare (sampleRate, channels);
            pair.b.prepare (sampleRate, channels);
            pair.reaction.prepare (sampleRate);
        }
    reset();
}

void MorphRender::reset() noexcept
{
    currentBank.fill (0);
    transitionSample.fill (0);
    transitioning.fill (false);
    // Existing prepared delay allocations are reused after reset.
    for (auto& scene : engines)
        for (auto& pair : scene)
        {
            pair.a.reset();
            pair.b.reset();
            pair.reaction.reset();
        }
}

void MorphRender::setScene (int scene, MorphScene next) noexcept
{
    if (scene < 0 || scene >= 4) return;
    next.sanitize();
    const size_t index = static_cast<size_t> (scene);
    if (! different (target[index], next)) return;
    if (transitioning[index])
    {
        // Preserve current active bank as source; the spare bank is free
        // to receive a new target, avoiding memory allocations on repeated captures.
        transitioning[index] = false;
    }
    previous[index] = target[index];
    target[index] = next;
    transitionSample[index] = 0;
    transitioning[index] = true;
    // No reset or buffer allocation: the destination bank is already prepared.
}

void MorphRender::configureOne (Pair& pair, const MorphScene& scene,
                                const MorphScene& continuous,
                                MatterDNA::Features features,
                                MatterDNAControls controls,
                                const MatrixOffsets* modulation) noexcept
{
    auto a = continuous.a;
    auto b = continuous.b;
    if (controls.enabled)
    {
        applyMatterDNA (static_cast<Material> (scene.materialA), a, features, controls);
        if (scene.enableB)
            applyMatterDNA (static_cast<Material> (scene.materialB), b, features, controls);
    }
    if (modulation != nullptr)
        modulation->apply (a, b);
    pair.a.configure (static_cast<Material> (scene.materialA), a);
    if (scene.enableB)
        pair.b.configure (static_cast<Material> (scene.materialB), b);
}

void MorphRender::configure (float x, float y,
                             MatterDNA::Features features,
                             MatterDNAControls controls,
                             ReactionControls reactions,
                             const MatrixOffsets* modulation) noexcept
{
    reactionSettings = reactions;
    matrixOffsets = modulation != nullptr ? *modulation : MatrixOffsets {};
    blockWeights = Morph4D::weights (x, y);
    continuousBlend = Morph4D::interpolate (target, blockWeights);
    continuousBlend.fusion = std::clamp (
        continuousBlend.fusion + matrixOffsets.destinations[37],0.0f,1.0f);
    continuousBlend.balance = std::clamp (
        continuousBlend.balance + matrixOffsets.destinations[38],0.0f,1.0f);
    for (int i = 0; i < 4; ++i)
    {
        const auto index = static_cast<size_t> (i);
        const int bank = currentBank[index];
        if (! transitioning[index])
            configureOne (engines[index][static_cast<size_t> (bank)],
                          target[index], continuousBlend, features, controls, modulation);
        else
        {
            auto oldSceneSet = target;
            oldSceneSet[index] = previous[index];
            const auto oldBlend = Morph4D::interpolate (oldSceneSet, blockWeights);
            previousContinuous[index] = oldBlend;
            previousContinuous[index].fusion = std::clamp (
                oldBlend.fusion + matrixOffsets.destinations[37],0.0f,1.0f);
            previousContinuous[index].balance = std::clamp (
                oldBlend.balance + matrixOffsets.destinations[38],0.0f,1.0f);
            configureOne (engines[index][static_cast<size_t> (bank)],
                          previous[index], previousContinuous[index], features, controls, modulation);
            configureOne (engines[index][static_cast<size_t> (1 - bank)],
                          target[index], continuousBlend, features, controls);
        }
    }
}

float MorphRender::renderOne (Pair& pair, const MorphScene& scene,
                              const MorphScene& continuous,
                              int channel, float input,
                              ReactionControls reactions) noexcept
{
    if (! scene.enableB) return pair.a.process (channel, input);
    return pair.reaction.process (pair.a, pair.b,
             static_cast<Material> (scene.materialA),
             static_cast<Material> (scene.materialB),
             scene.routing, continuous.balance, continuous.fusion,
             channel, input, reactions);

}

void MorphRender::processFrame (const float in[2], float out[2], int channels,
                                float x, float y) noexcept
{
    const auto weight = Morph4D::weights (x, y);
    out[0] = out[1] = 0.0f;
    for (int scene = 0; scene < 4; ++scene)
    {
        const size_t k = static_cast<size_t> (scene);
        const int active = currentBank[k];
        const float t = transitioning[k]
            ? std::clamp (static_cast<float> (transitionSample[k]) /
                          static_cast<float> (fadeSamples), 0.0f, 1.0f) : 1.0f;
        // All engine pairs always process audio, even when their XY weight is
        // zero, so tails and feedback continue during movement between corners.
        for (int ch = 0; ch < channels && ch < 2; ++ch)
        {
            const float oldOutput = renderOne (
                engines[k][static_cast<size_t> (active)],
                transitioning[k] ? previous[k] : target[k],
                transitioning[k] ? previousContinuous[k] : continuousBlend,
                ch, in[ch], reactionSettings);
            float sample = oldOutput;
            if (transitioning[k])
            {
                const float fresh = renderOne (
                    engines[k][static_cast<size_t> (1 - active)], target[k],
                    continuousBlend, ch, in[ch], reactionSettings);
                sample = oldOutput * (1.0f - t) + fresh * t;
            }
            out[ch] += sample * weight[k];
        }
        if (transitioning[k] && ++transitionSample[k] >= fadeSamples)
        {
            currentBank[k] = 1 - currentBank[k];
            previous[k] = target[k];
            transitioning[k] = false;
        }
    }
}
float MorphRender::wetAt (const std::array<float,4>& weights) const noexcept
{
    float value = 0.0f;
    for (int i = 0; i < 4; ++i)
        value += weights[static_cast<size_t> (i)] * target[static_cast<size_t> (i)].wet;
    return std::clamp (value + matrixOffsets.destinations[39], 0.0f, 1.0f);
}
float MorphRender::widthAt (const std::array<float,4>& weights) const noexcept
{
    float value = 0.0f;
    for (int i = 0; i < 4; ++i)
        value += weights[static_cast<size_t> (i)] * target[static_cast<size_t> (i)].width;
    return std::clamp (value + matrixOffsets.destinations[40]*2.0f, 0.0f, 2.0f);
}
float MorphRender::reactionMeterAB() const noexcept
{
    float sum = 0.0f;
    for (int i = 0; i < 4; ++i)
        sum += blockWeights[static_cast<size_t> (i)] *
               engines[static_cast<size_t> (i)]
                      [static_cast<size_t> (currentBank[static_cast<size_t> (i)])]
                      .reaction.activityAB();
    return std::clamp (sum, 0.0f, 1.0f);
}
float MorphRender::reactionMeterBA() const noexcept
{
    float sum = 0.0f;
    for (int i = 0; i < 4; ++i)
        sum += blockWeights[static_cast<size_t> (i)] *
               engines[static_cast<size_t> (i)]
                      [static_cast<size_t> (currentBank[static_cast<size_t> (i)])]
                      .reaction.activityBA();
    return std::clamp (sum, 0.0f, 1.0f);
}
bool MorphRender::hasPendingTransition (int scene) const noexcept
{
    return scene >= 0 && scene < 4 && transitioning[static_cast<size_t> (scene)];
}
} // namespace organix
