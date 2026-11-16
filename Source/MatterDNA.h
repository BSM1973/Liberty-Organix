#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>

namespace organix
{
// A fixed-memory, causal input analyzer. FFT is evaluated inside the audio
// callback only after a full 1024-sample frame and then once every 256 samples.
// No heap allocation, locks, file access or UI calls occur in processBlock().
class MatterDNA
{
public:
    static constexpr int fftSize = 1024;
    static constexpr int hopSize = 256;
    static constexpr int bins = fftSize / 2;

    struct Features
    {
        float energy = 0.0f;        // RMS-derived input energy, 0..1
        float transient = 0.0f;     // fast vs slow envelope attack
        float air = 0.0f;           // 3.5kHz+ FFT energy ratio
        float centroid = 0.0f;      // normalized spectral centroid
        float motion = 0.0f;        // normalized positive spectral flux
        float low = 0.0f;           // relative <250Hz energy
        float mid = 0.0f;           // relative 250..3500Hz energy
        float high = 0.0f;          // relative >=3500Hz energy
    };

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    Features analyzeBlock (const float* left, const float* right,
                           int numSamples, int channels) noexcept;
    Features getFeatures() const noexcept { return features; }

private:
    void processSpectrum() noexcept;
    static float clamp01 (float value) noexcept
    {
        return std::clamp (value, 0.0f, 1.0f);
    }

    std::array<float, fftSize> ring {};
    std::array<float, fftSize> window {};
    std::array<std::complex<float>, fftSize> fft {};
    std::array<std::complex<float>, bins> twiddle {};
    std::array<float, bins> previousMagnitudes {};
    Features features;
    double rate = 48000.0;
    int writeIndex = 0;
    int validSamples = 0;
    int hopCounter = 0;
    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;
    float recentSpectralFlux = 0.0f;
    float recentAir = 0.0f;
    float recentCentroid = 0.0f;
    float recentLow = 0.0f;
    float recentMid = 0.0f;
    float recentHigh = 0.0f;
};
}
