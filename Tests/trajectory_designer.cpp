#include "../Source/OrganixTrajectory.h"
#include "../Source/OrganixMorph4D.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool near (float a, float b, float e = 1.0e-4f)
{
    return std::abs (a - b) <= e;
}
bool finite (organix::TrajectoryPoint p)
{
    return std::isfinite (p.x) && std::isfinite (p.y) &&
           p.x >= 0.0f && p.x <= 1.0f && p.y >= 0.0f && p.y <= 1.0f;
}
}
int main()
{
    using organix::TrajectoryDesigner;
    using organix::TrajectoryPath;
    TrajectoryPath custom;
    custom.count = 4;
    custom.points[0] = {0.1f, 0.1f};
    custom.points[1] = {0.9f, 0.1f};
    custom.points[2] = {0.9f, 0.9f};
    custom.points[3] = {0.1f, 0.9f};
    custom.sanitize();
    for (int shape = 0; shape < 4; ++shape)
    {
        const auto atStart = TrajectoryDesigner::sample (shape, 0.0f, custom);
        const auto atEnd = TrajectoryDesigner::sample (shape, 1.0f, custom);
        if (! near (atStart.x, atEnd.x) || ! near (atStart.y, atEnd.y)) return 1;
        for (int i = 0; i <= 1000; ++i)
        {
            const auto p = TrajectoryDesigner::sample (shape, i / 1000.0f, custom);
            if (! finite (p)) return 2;
            const auto reversed = TrajectoryDesigner::sample (
                shape, 1.0f - i / 1000.0f, custom);
            if (! finite (reversed)) return 3;
        }
    }
    // Circle is periodic, spiral starts and closes at center.
    auto spiralStart = TrajectoryDesigner::sample (1, 0.0f, custom);
    if (! near (spiralStart.x, 0.5f) || ! near (spiralStart.y, 0.5f)) return 4;

    // The custom path follows mouse data, not a decorative default shape.
    auto start = TrajectoryDesigner::sample (3, 0.0f, custom);
    auto right = TrajectoryDesigner::sample (3, 0.25f, custom);
    if (! near (start.x, 0.1f) || ! near (start.y, 0.1f) ||
        ! near (right.x, 0.9f) || ! near (right.y, 0.1f)) return 5;

    const auto fixed = TrajectoryDesigner::blend ({0.22f, 0.38f}, {0.92f, 0.72f}, 0.0f);
    const auto full = TrajectoryDesigner::blend ({0.22f, 0.38f}, {0.92f, 0.72f}, 1.0f);
    if (! near (fixed.x, 0.22f) || ! near (full.y, 0.72f)) return 6;
    auto half = TrajectoryDesigner::blend ({0.22f, 0.38f}, {0.92f, 0.72f}, 0.5f);
    if (! near (half.x, 0.57f) || ! near (half.y, 0.55f)) return 7;

    if (! near (TrajectoryDesigner::frequency (true, 0.35f, 120.0f, 2), 0.35f)) return 8;
    if (! near (TrajectoryDesigner::frequency (false, 1.0f, 120.0f, 2), 2.0f)) return 9;
    if (! near (TrajectoryDesigner::frequency (false, 2.0f, 120.0f, 2), 4.0f)) return 10;
    if (! near (TrajectoryDesigner::advance (0.95f, 2400.0f, 48000.0), 0.0f, 1.0e-3f)) return 11;

    // The trajectory changes the bilinear weights feeding actual DSP.
    const auto weightsOne = organix::Morph4D::weights (
        TrajectoryDesigner::sample (0, 0.0f, custom).x,
        TrajectoryDesigner::sample (0, 0.0f, custom).y);
    const auto weightsTwo = organix::Morph4D::weights (
        TrajectoryDesigner::sample (0, 0.5f, custom).x,
        TrajectoryDesigner::sample (0, 0.5f, custom).y);
    if (near (weightsOne[0], weightsTwo[0]) &&
        near (weightsOne[2], weightsTwo[2])) return 12;
    custom.points[0].x = std::numeric_limits<float>::quiet_NaN();
    custom.points[1].y = 9.0f;
    custom.sanitize();
    if (! finite (custom.points[0]) || ! finite (custom.points[1])) return 13;
    std::cout << "PASS: circle, spiral, zigzag, stored freehand curve, reverse, sync, depth and DSP weights\n";
    return 0;
}
