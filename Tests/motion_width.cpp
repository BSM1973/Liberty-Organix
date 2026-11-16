#include "../Source/OrganixMovement.h"
#include <cmath>
#include <iostream>

namespace
{
bool close (float a, float b, float eps = 1.0e-5f)
{
    return std::abs (a - b) <= eps;
}
}
int main()
{
    using organix::Movement;
    using organix::StereoWidth;

    if (! close (Movement::frequency (true, 0.25f, 120, 2), 0.25f)) return 1;
    if (! close (Movement::frequency (false, 0.25f, 120, 2), 2.0f)) return 2;
    if (! close (Movement::frequency (false, 1.0f, 120, 0), 0.5f)) return 3;
    if (! close (Movement::frequency (false, 1.0f, 120, 6), 12.0f)) return 4;
    if (! close (Movement::advance (0.9f, 0.3f), 0.2f)) return 5;
    if (! close (Movement::wave (0, 0.24f, 0.78f), 0.0f)) return 6;
    if (! close (Movement::wave (1, 0.25f, 0.0f), 1.0f)) return 7;
    if (! close (Movement::wave (2, 0.5f, 0.0f), 1.0f)) return 8;
    if (! close (Movement::wave (3, 0.75f, 0.0f), -1.0f)) return 9;
    if (! close (Movement::wave (4, 0.28f, 0.7f), 0.7f)) return 10;
    if (! close (Movement::scaleWet (0.5f, 0.8f, 0.0f), 0.5f)) return 11;
    if (close (Movement::scaleWet (0.5f, 0.8f, 1.0f), 0.5f)) return 12;
    if (close (Movement::scaleWet (0.5f, 0.8f, -1.0f), 0.5f)) return 13;
    for (int i = 0; i <= 200; ++i)
    {
        const float sample = static_cast<float> (i) / 200.0f;
        for (int mode = 0; mode <= 4; ++mode)
        {
            const float wave = Movement::wave (mode, sample, 0.77f);
            const float wet = Movement::scaleWet (sample, 0.75f, wave);
            if (! std::isfinite (wave) || wave < -1.0001f || wave > 1.0001f ||
                ! std::isfinite (wet) || wet < 0.0f || wet > 1.0f) return 14;
        }
    }
    // Default width is bitwise/arithmetically neutral for representable samples.
    {
        float l = 0.31f, r = -0.18f;
        const float mid = (l + r) * 0.5f;
        StereoWidth::process (l, r, 0.0f);
        if (! close (l, mid) || ! close (r, mid)) return 15;
    }
    {
        float l = 0.6f, r = -0.2f;
        StereoWidth::process (l, r, 1.0f);
        if (! close (l, 0.6f) || ! close (r, -0.2f)) return 16;
    }
    {
        float l = 0.6f, r = -0.2f;
        StereoWidth::process (l, r, 2.0f);
        if (! close (l, 1.0f) || ! close (r, -0.6f)) return 17;
    }
    // 100% of randomly sampled stereo pairs maintain their mono midpoint.
    for (int i = 0; i < 10000; ++i)
    {
        const float a = std::sin (i * 0.017f);
        const float b = std::cos (i * 0.031f);
        const float m = (a + b) * 0.5f;
        float left = a, right = b;
        StereoWidth::process (left, right, static_cast<float> (i % 201) / 100.0f);
        if (! std::isfinite (left) || ! std::isfinite (right) ||
            ! close ((left + right) * 0.5f, m)) return 18;
    }

    std::cout << "PASS: real motion FREE/SYNC, trajectories, beat divisions and M/S width\n";
    return 0;
}
