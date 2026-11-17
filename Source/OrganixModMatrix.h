#pragma once
#include "MaterialDSP.h"
#include "MatterDNA.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace organix
{
// 32 actual audio modulation assignments; 20 distinct source types and 40
// normalized destination parameters. No allocation in processSegment().
struct MatrixSlot
{
    int source = 0;        // 0 OFF, 1..20
    int destination = 0;   // 0 OFF, 1..40
    float depth = 0.0f;    // -1..1: signed modulation amount
    bool bipolar = false; // false: 0..1, true: -1..1
    bool enabled = false;
};
struct MatrixSettings
{
    static constexpr int slotCount = 32;
    static constexpr int sourceCount = 21;
    static constexpr int destinationCount = 41;
    bool enabled = false;
    bool tempoSync = false;
    int division = 2;
    float masterDepth = 0.8f;
    float rates[4] {0.17f, 0.31f, 0.5f, 0.9f};
    float envAttack[2] {0.015f, 0.060f}; // seconds
    float envDecay[2] {0.32f, 0.8f};    // seconds
    float randomRate = 1.0f;
    std::array<MatrixSlot, slotCount> slots {};
};
struct MatrixOffsets
{
    static constexpr int count = MatrixSettings::destinationCount;
    // Index 0 is unused. Main and scene parameters share identical addressing:
    // 1..18: A (amount,motion,tone,feedback,density,energy,shape01..12)
    // 19..36: B (ditto), 37 fusion, 38 balance, 39 drywet, 40 width.
    std::array<float, count> destinations {};
    std::array<float, MatrixSettings::slotCount> activity {};
    void apply (Parameters& a, Parameters& b, float& fusion, float& balance,
                float& wet, float& width) const noexcept;
    void apply (Parameters& a, Parameters& b) const noexcept;
};
class ModulationMatrix
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    MatrixOffsets processSegment (
        const MatrixSettings& settings,
        const float* left, const float* right,
        int firstSample, int frames, int channels, float bpm,
        MatterDNA::Features spectral,
        const std::array<float,8>& macros) noexcept;

    const MatrixOffsets& getLatest() const noexcept { return latest; }
    static const char* sourceName (int source, bool french) noexcept;
    static const char* destinationName (int destination, bool french) noexcept;

private:
    struct Envelope
    {
        float level = 0.0f;
        bool rising = false;
        int refractory = 0;
    };
    float rate = 48000.0f;
    std::array<float,4> phase {};
    std::array<Envelope,2> envelope {};
    float fast = 0.0f, slow = 0.0f;
    float rmsSmoothed = 0.0f;
    float randomPhase = 0.0f, held = 0.0f, previousHeld = 0.0f;
    std::uint32_t rng = 0x9E3779B9u;
    std::array<float,MatrixSettings::sourceCount> sources {};
    std::array<float,MatrixSettings::slotCount> smoothedSlots {};
    MatrixOffsets latest;
    float nextRandom() noexcept;
};
} // namespace organix
