#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace organix
{
// Pure C++ controls also consumed by the audio callback. All functions are
// allocation-free, bounded and testable independently of JUCE.
struct Movement
{
    static float frequency (bool freeRunning, float hz, double bpm, int division) noexcept
    {
        if (freeRunning) return std::clamp (hz, 0.02f, 12.0f);
        const float beats = std::clamp (static_cast<float> (bpm), 20.0f, 300.0f) / 60.0f;
        constexpr float cyclesPerBeat[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 3.0f, 6.0f };
        return beats * cyclesPerBeat[std::clamp (division, 0, 6)];
    }

    static float wave (int trajectory, float phase, float held) noexcept
    {
        const float p = phase - std::floor (phase);
        switch (trajectory)
        {
            case 1: return std::sin (6.2831853071795864769f * p); // smooth
            case 2: return 1.0f - 4.0f * std::abs (p - 0.5f); // triangle
            case 3: return p < 0.5f ? 1.0f : -1.0f; // pulse
            case 4: return std::clamp (held, -1.0f, 1.0f); // sample and hold
            default: return 0.0f; // manual: no autonomous modulation
        }
    }

    static float advance (float phase, float increment) noexcept
    {
        const float p = phase + increment;
        return p - std::floor (p);
    }

    static float scaleWet (float wet, float movement, float waveValue) noexcept
    {
        return std::clamp (wet * (1.0f + (0.12f + movement * 0.36f) *
                                     waveValue), 0.0f, 1.0f);
    }
};

struct StereoWidth
{
    static void process (float& left, float& right, float width) noexcept
    {
        // Normalised M/S. Width 0 = mono, 1 = unchanged, 2 = twice the side signal.
        const float mid = (left + right) * 0.5f;
        const float side = (left - right) * 0.5f * std::clamp (width, 0.0f, 2.0f);
        left = mid + side;
        right = mid - side;
    }
};
} // namespace organix
