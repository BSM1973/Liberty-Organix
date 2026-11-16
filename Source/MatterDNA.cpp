#include "MatterDNA.h"
#include <limits>

namespace organix
{
namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float tau = 2.0f * pi;
float finite (float x) noexcept { return std::isfinite (x) ? x : 0.0f; }
}

void MatterDNA::prepare (double sampleRate) noexcept
{
    rate = std::clamp (sampleRate, 8000.0, 384000.0);
    for (int i = 0; i < fftSize; ++i)
        window[static_cast<std::size_t> (i)] =
            0.5f - 0.5f * std::cos (tau * i / static_cast<float> (fftSize - 1));
    for (int k = 0; k < bins; ++k)
    {
        const float theta = -tau * static_cast<float> (k) / fftSize;
        twiddle[static_cast<std::size_t> (k)] = {std::cos (theta), std::sin (theta)};
    }
    reset();
}

void MatterDNA::reset() noexcept
{
    ring.fill (0.0f);
    previousMagnitudes.fill (0.0f);
    fft.fill ({});
    features = {};
    writeIndex = 0;
    validSamples = 0;
    hopCounter = 0;
    fastEnvelope = slowEnvelope = 0.0f;
    recentSpectralFlux = recentAir = recentCentroid = 0.0f;
    recentLow = recentMid = recentHigh = 0.0f;
}

void MatterDNA::processSpectrum() noexcept
{
    // Original 1024-sample mono analysis frame, placed in chronological order.
    for (int i = 0; i < fftSize; ++i)
    {
        const int sample = (writeIndex + i) & (fftSize - 1);
        fft[static_cast<std::size_t> (i)] = {
            ring[static_cast<std::size_t> (sample)] * window[static_cast<std::size_t> (i)], 0.0f
        };
    }

    // In-place radix-2 FFT; precomputed complex twiddles, no allocations.
    int j = 0;
    for (int i = 1; i < fftSize; ++i)
    {
        int bit = fftSize >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (fft[static_cast<std::size_t> (i)], fft[static_cast<std::size_t> (j)]);
    }
    for (int length = 2; length <= fftSize; length <<= 1)
    {
        const int half = length >> 1, step = fftSize / length;
        for (int base = 0; base < fftSize; base += length)
            for (int k = 0; k < half; ++k)
            {
                const auto v = fft[static_cast<std::size_t> (base + k + half)] *
                               twiddle[static_cast<std::size_t> (k * step)];
                const auto u = fft[static_cast<std::size_t> (base + k)];
                fft[static_cast<std::size_t> (base + k)] = u + v;
                fft[static_cast<std::size_t> (base + k + half)] = u - v;
            }
    }

    float lowEnergy = 0.0f, midEnergy = 0.0f, airEnergy = 0.0f;
    float totalEnergy = 0.0f, weightedFrequency = 0.0f;
    float flux = 0.0f, magnitudeSum = 0.0f;
    const float binHz = static_cast<float> (rate / fftSize);
    const float nyquist = static_cast<float> (rate * 0.5);
    const float airCutoff = std::min (3500.0f, nyquist * 0.7f);
    for (int k = 1; k < bins; ++k)
    {
        const auto& bin = fft[static_cast<std::size_t> (k)];
        const float power = std::norm (bin);
        const float magnitude = std::sqrt (std::max (0.0f, power));
        const float frequency = k * binHz;
        totalEnergy += power;
        weightedFrequency += magnitude * frequency;
        magnitudeSum += magnitude;
        if (frequency < 250.0f) lowEnergy += power;
        else if (frequency < airCutoff) midEnergy += power;
        else airEnergy += power;

        flux += std::max (0.0f, magnitude - previousMagnitudes[static_cast<std::size_t> (k)]);
        previousMagnitudes[static_cast<std::size_t> (k)] = magnitude;
    }

    constexpr float epsilon = 1.0e-8f;
    if (totalEnergy <= epsilon)
    {
        recentAir *= 0.6f;
        recentCentroid *= 0.6f;
        recentSpectralFlux *= 0.6f;
        recentLow *= 0.6f;
        recentMid *= 0.6f;
        recentHigh *= 0.6f;
        return;
    }
    const float spectralCentroid = weightedFrequency /
                                  std::max (epsilon, magnitudeSum) / std::max (1.0f, nyquist);
    // Spectral flux is positive magnitude growth, not pitch tracking.
    const float spectralMotion = clamp01 (flux * 2.4f / std::max (epsilon, magnitudeSum));
    const float ratio = 1.0f / totalEnergy;
    const float air = std::sqrt (airEnergy * ratio);
    const float low = std::sqrt (lowEnergy * ratio);
    const float mid = std::sqrt (midEnergy * ratio);
    const float high = std::sqrt (airEnergy * ratio);
    recentAir += (clamp01 (air) - recentAir) * 0.34f;
    recentCentroid += (clamp01 (spectralCentroid) - recentCentroid) * 0.34f;
    recentSpectralFlux += (spectralMotion - recentSpectralFlux) * 0.28f;
    recentLow += (clamp01 (low) - recentLow) * 0.34f;
    recentMid += (clamp01 (mid) - recentMid) * 0.34f;
    recentHigh += (clamp01 (high) - recentHigh) * 0.34f;
}

MatterDNA::Features MatterDNA::analyzeBlock (const float* left, const float* right,
                                              int numSamples, int channels) noexcept
{
    if (left == nullptr || channels <= 0 || numSamples <= 0) return features;

    double sumSquares = 0.0;
    float maxTransient = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float l = finite (left[i]);
        const float r = channels >= 2 && right != nullptr ? finite (right[i]) : l;
        const float mono = std::clamp (0.5f * (l + r), -2.0f, 2.0f);
        sumSquares += 0.5 * (static_cast<double> (l) * l + static_cast<double> (r) * r);

        const float absolute = std::abs (mono);
        fastEnvelope += (absolute - fastEnvelope) * 0.085f;
        slowEnvelope += (absolute - slowEnvelope) * 0.004f;
        const float rise = clamp01 ((fastEnvelope - slowEnvelope) / (0.075f + slowEnvelope * 1.5f));
        maxTransient = std::max (maxTransient, rise);

        ring[static_cast<std::size_t> (writeIndex)] = mono;
        writeIndex = (writeIndex + 1) & (fftSize - 1);
        validSamples = std::min (fftSize, validSamples + 1);
        if (++hopCounter >= hopSize)
        {
            hopCounter = 0;
            if (validSamples >= fftSize) processSpectrum();
        }
    }

    const float rms = std::sqrt (static_cast<float> (sumSquares / numSamples));
    const float energy = clamp01 (rms * 2.4f);
    features.energy += (energy - features.energy) * 0.30f;
    features.transient += (maxTransient - features.transient) * (maxTransient > features.transient ? 0.5f : 0.18f);
    // Gate ratios so an almost-silent input cannot modulate Vapor from FFT noise.
    const float audibility = clamp01 (rms * 45.0f);
    features.air = clamp01 (recentAir * audibility);
    features.centroid = clamp01 (recentCentroid * audibility);
    features.motion = clamp01 (recentSpectralFlux * audibility);
    features.low = clamp01 (recentLow * audibility);
    features.mid = clamp01 (recentMid * audibility);
    features.high = clamp01 (recentHigh * audibility);
    return features;
}
}
