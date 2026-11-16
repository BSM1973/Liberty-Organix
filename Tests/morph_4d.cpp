#include "../Source/OrganixMorph4D.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool near (float x, float y, float epsilon = 0.0001f)
{
    return std::abs (x - y) <= epsilon;
}
bool bounded (const std::array<float,4>& w)
{
    float sum = 0.0f;
    for (float x : w)
    {
        if (! std::isfinite (x) || x < 0.0f || x > 1.0f) return false;
        sum += x;
    }
    return near (sum, 1.0f, 0.00001f);
}
}
int main()
{
    using organix::Morph4D;
    using organix::MorphScene;
    using organix::MorphRender;
    const auto a = Morph4D::weights (0.0f, 0.0f);
    const auto b = Morph4D::weights (1.0f, 0.0f);
    const auto c = Morph4D::weights (0.0f, 1.0f);
    const auto d = Morph4D::weights (1.0f, 1.0f);
    const auto centre = Morph4D::weights (0.5f, 0.5f);
    if (a != std::array<float,4>{{1,0,0,0}} ||
        b != std::array<float,4>{{0,1,0,0}} ||
        c != std::array<float,4>{{0,0,1,0}} ||
        d != std::array<float,4>{{0,0,0,1}}) return 1;
    for (float w : centre) if (! near (w, 0.25f)) return 2;
    for (int x = 0; x <= 100; ++x)
        for (int y = 0; y <= 100; ++y)
            if (! bounded (Morph4D::weights (x * 0.01f, y * 0.01f))) return 3;

    std::array<MorphScene,4> scenes {};
    for (int i = 0; i < 4; ++i)
    {
        auto& scene = scenes[static_cast<size_t> (i)];
        scene.materialA = i; // four genuinely different engines
        scene.materialB = 5 - i;
        scene.enableB = i % 2 == 0;
        scene.routing = i;
        scene.a.amount = 0.10f + i * 0.2f;
        scene.a.shape[0] = i * 0.25f;
        scene.b.feedback = i * 0.2f;
        scene.balance = i * 0.25f;
        scene.fusion = i * 0.20f;
        scene.wet = i * 0.25f;
        scene.width = i * 0.5f;
        scene.sanitize();
        const auto decoded = MorphScene::unpack (scene.pack());
        if (decoded.pack() != scene.pack()) return 4;
    }
    const auto atA = Morph4D::interpolate (scenes, a);
    const auto atD = Morph4D::interpolate (scenes, d);
    const auto middle = Morph4D::interpolate (scenes, centre);
    if (! near (atA.a.amount, scenes[0].a.amount) ||
        ! near (atD.wet, scenes[3].wet) ||
        ! near (middle.a.amount, 0.4f) ||
        ! near (middle.a.shape[0], 0.375f) ||
        ! near (middle.width, 0.75f) ||
        ! near (middle.balance, 0.375f)) return 5;

    std::array<float, MorphScene::packedCount> invalid = scenes[0].pack();
    invalid[0] = 10000.0f;
    invalid[3] = -100.0f;
    invalid[9] = std::numeric_limits<float>::quiet_NaN();
    const auto repaired = MorphScene::unpack (invalid);
    if (repaired.materialA != 5 || repaired.enableB ||
        ! std::isfinite (repaired.a.energy)) return 6;

    MorphRender render;
    render.prepare (48000.0, 2);
    for (int i = 0; i < 4; ++i) render.setScene (i, scenes[static_cast<size_t> (i)]);
    render.configure (0.5f, 0.5f);
    float maxStep = 0.0f, last = 0.0f;
    constexpr float pi2 = 6.283185307179586f;
    for (int frame = 0; frame < 14000; ++frame)
    {
        if (frame == 6000)
        {
            scenes[2].materialA = 4;
            scenes[2].routing = 1;
            scenes[2].a.amount = 0.85f;
            render.setScene (2, scenes[2]); // real engine replacement while processing
            render.configure (0.5f, 0.5f);
            if (! render.hasPendingTransition (2)) return 7;
        }
        if (frame == 12000)
        {
            render.configure (0.75f, 0.25f);
        }
        float input[2] = {
            0.20f * std::sin (frame * pi2 * 220.0f / 48000.0f),
            0.16f * std::sin (frame * pi2 * 330.0f / 48000.0f)
        };
        float output[2] {};
        const float x = frame < 12000 ? 0.5f : 0.75f;
        const float y = frame < 12000 ? 0.5f : 0.25f;
        render.processFrame (input, output, 2, x, y);
        if (! std::isfinite (output[0]) || ! std::isfinite (output[1]) ||
            std::abs (output[0]) > 10.0f || std::abs (output[1]) > 10.0f) return 8;
        if (frame > 200) maxStep = std::max (maxStep, std::abs (output[0] - last));
        last = output[0];
    }
    if (render.hasPendingTransition (2)) return 9;
    if (maxStep > 1.0f) return 10;
    std::cout << "PASS: 4 scene positions, bilinear interpolation, serialization and smooth DSP bank transitions\n";
    return 0;
}
