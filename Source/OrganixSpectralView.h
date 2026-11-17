#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstdint>

namespace organix
{
// Real-time safe dual FFT analyzer, completely independent of the DNA
// modulation engine. Input and output are processed separately so the central
// view represents the actual wet/dry/bypass result sent to the host.
// Audio thread writes; UI thread reads only atomics.
class SpectralViewAnalyzer
{
public:
    static constexpr int fftSize = 1024;
    static constexpr int hopSize = 512;
    static constexpr int fftBins = fftSize / 2;
    static constexpr int displayBands = 48;
    enum class Stream { Before = 0, After = 1 };

    SpectralViewAnalyzer() noexcept;
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void analyze (Stream stream, const float* left, const float* right,
                  int sampleCount, int channels) noexcept;

    float band (Stream stream, int index) const noexcept;
    float transient (Stream stream) const noexcept;
    float energy (Stream stream) const noexcept;
    float centroid (Stream stream) const noexcept;
    float brightness (Stream stream) const noexcept;
    float motion (Stream stream) const noexcept;

private:
    struct State
    {
        std::array<float, fftSize> ring {};
        std::array<std::complex<float>, fftSize> fft {};
        std::array<float, fftBins> priorMagnitude {};
        std::array<float, displayBands> filteredBands {};
        int writeIndex = 0, validSamples = 0, hopCounter = 0;
        float fastEnvelope = 0.0f, slowEnvelope = 0.0f;
        float recentTransient = 0.0f, recentEnergy = 0.0f;
        float recentCentroid = 0.0f, recentBrightness = 0.0f;
        float recentMotion = 0.0f;
    };
    struct Readout
    {
        std::array<std::atomic<float>, displayBands> bands {};
        std::atomic<float> transient {0.0f}, energy {0.0f};
        std::atomic<float> centroid {0.0f}, brightness {0.0f};
        std::atomic<float> motion {0.0f};
    };
    double rate = 48000.0;
    float fastCoefficient = 0.0f, slowCoefficient = 0.0f;
    std::array<float, fftSize> hann {};
    std::array<std::complex<float>, fftBins> twiddles {};
    std::array<unsigned char, fftBins> binBand {};
    std::array<State,2> states {};
    std::array<Readout,2> readouts {};

    void transform (State& state, Readout& readout) noexcept;
    static float finite (float value) noexcept
    {
        return std::isfinite (value) ? std::clamp (value, -2.0f, 2.0f) : 0.0f;
    }
    static float clamp01 (float value) noexcept
    {
        return std::isfinite (value) ? std::clamp (value, 0.0f, 1.0f) : 0.0f;
    }
};
}
