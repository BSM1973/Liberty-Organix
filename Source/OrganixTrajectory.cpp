#include "OrganixTrajectory.h"

namespace organix
{
namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float tau = 2.0f * pi;
float safe (float x) noexcept { return std::isfinite (x) ? std::clamp (x, 0.0f, 1.0f) : 0.0f; }
TrajectoryPoint line (TrajectoryPoint a, TrajectoryPoint b, float t) noexcept
{
    return { safe (a.x + (b.x - a.x) * t),
             safe (a.y + (b.y - a.y) * t) };
}
template <size_t N>
TrajectoryPoint polygon (const std::array<TrajectoryPoint, N>& points, float phase) noexcept
{
    const float pos = phase * static_cast<float> (N);
    const int i = std::clamp (static_cast<int> (pos), 0, static_cast<int> (N) - 1);
    const int next = (i + 1) % static_cast<int> (N);
    return line (points[static_cast<size_t> (i)],
                 points[static_cast<size_t> (next)],
                 pos - static_cast<float> (i));
}
}
void TrajectoryPath::sanitize() noexcept
{
    count = std::clamp (count, 0, maxPoints);
    for (int i = 0; i < count; ++i)
    {
        auto& p = points[static_cast<size_t> (i)];
        p.x = safe (p.x);
        p.y = safe (p.y);
    }
}
TrajectoryPoint TrajectoryDesigner::sample (int shape, float phase,
                                            const TrajectoryPath& path) noexcept
{
    const float p = std::isfinite (phase) ? phase - std::floor (phase) : 0.0f;
    if (shape == 1)
    {
        // Three graceful rotations; radius closes at the centre without a seam.
        const float angle = -0.5f * pi + tau * 3.0f * p;
        const float radius = 0.45f * std::sin (pi * p);
        return { safe (0.5f + radius * std::cos (angle)),
                 safe (0.5f + radius * std::sin (angle)) };
    }
    if (shape == 2)
    {
        constexpr std::array<TrajectoryPoint, 6> points {{
            {0.12f,0.16f}, {0.88f,0.16f}, {0.12f,0.39f},
            {0.88f,0.61f}, {0.12f,0.84f}, {0.88f,0.84f}
        }};
        return polygon (points, p);
    }
    if (shape == 3 && path.count > 1)
    {
        const int count = std::clamp (path.count, 2, TrajectoryPath::maxPoints);
        const float pos = p * static_cast<float> (count);
        const int index = std::clamp (static_cast<int> (pos), 0, count - 1);
        return line (path.points[static_cast<size_t> (index)],
                     path.points[static_cast<size_t> ((index + 1) % count)],
                     pos - static_cast<float> (index));
    }
    // Circle, and graceful freehand fallback if no user curve is saved.
    const float angle = -0.5f * pi + tau * p;
    return {safe (0.5f + 0.43f * std::cos (angle)),
            safe (0.5f + 0.43f * std::sin (angle))};
}
float TrajectoryDesigner::frequency (bool freeRunning, float speed,
                                    float bpm, int division) noexcept
{
    const float hz = std::isfinite (speed) ? std::clamp (speed, 0.05f, 6.0f) : 0.25f;
    const float safeBpm = std::isfinite (bpm) ? std::clamp (bpm, 20.0f, 300.0f) : 120.0f;
    // In sync mode speed is a musical multiplier of the selected division.
    return freeRunning ? hz : Movement::frequency (false, 0.25f, safeBpm, division) * hz;
}
float TrajectoryDesigner::advance (float phase, float frequency,
                                   double sampleRate) noexcept
{
    if (! std::isfinite (frequency) || sampleRate <= 0.0 ||
        ! std::isfinite (sampleRate)) return 0.0f;
    const float p = phase + frequency / static_cast<float> (sampleRate);
    return p - std::floor (p);
}
TrajectoryPoint TrajectoryDesigner::blend (TrajectoryPoint manual,
                                           TrajectoryPoint animated,
                                           float depth) noexcept
{
    return line (manual, animated, safe (depth));
}
}
