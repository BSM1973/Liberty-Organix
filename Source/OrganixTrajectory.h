#pragma once
#include "OrganixMovement.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace organix
{
struct TrajectoryPoint
{
    float x = 0.5f, y = 0.5f;
};
struct TrajectoryPath
{
    static constexpr int maxPoints = 64;
    std::array<TrajectoryPoint, maxPoints> points {};
    int count = 0;
    void sanitize() noexcept;
};
class TrajectoryDesigner
{
public:
    // 0 circle; 1 spiral; 2 zigzag; 3 freehand.
    static TrajectoryPoint sample (int shape, float phase,
                                    const TrajectoryPath& path) noexcept;
    static float frequency (bool freeRunning, float speed, float bpm,
                            int division) noexcept;
    static float advance (float phase, float frequency, double sampleRate) noexcept;
    static TrajectoryPoint blend (TrajectoryPoint manual,
                                  TrajectoryPoint animated,
                                  float depth) noexcept;
};
}
