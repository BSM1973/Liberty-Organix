#pragma once
#include "MaterialDSP.h"
#include "OrganixMovement.h"
#include "MatterDNAModulation.h"
#include "MaterialReactions.h"
#include <array>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace organix
{
// Top left A, top right B, bottom left C, bottom right D.
struct MorphScene
{
    int materialA = 0, materialB = 1, routing = 0;
    bool enableB = true;
    Parameters a, b;
    float fusion = 0.30f, balance = 0.50f, wet = 0.65f, width = 1.0f;
    static constexpr int packedCount = 44;
    std::array<float, packedCount> pack() const noexcept;
    static MorphScene unpack (const std::array<float, packedCount>& values) noexcept;
    void sanitize() noexcept;
};

struct Morph4D
{
    static std::array<float,4> weights (float x, float y) noexcept;
    static MorphScene interpolate (const std::array<MorphScene,4>& scenes,
                                   const std::array<float,4>& weights) noexcept;
};

// Each scene is an independent pair of audio engines. There are two prepared
// engine banks per scene so capturing a new material/routing fades the old
// and new engines instead of resetting active DSP state or allocating.
class MorphRender
{
public:
    void prepare (double sampleRate, int channels);
    void reset() noexcept;
    void setScene (int scene, MorphScene next) noexcept;
    MorphScene getScene (int scene) const noexcept { return target[static_cast<size_t> (scene)]; }
    void configure (float x, float y,
                    MatterDNA::Features features = {},
                    MatterDNAControls controls = {},
                    ReactionControls reactions = {}) noexcept;
    void processFrame (const float in[2], float out[2], int channels,
                       float x, float y) noexcept;
    bool hasPendingTransition (int scene) const noexcept;
    float wetAt (const std::array<float,4>& weights) const noexcept;
    float widthAt (const std::array<float,4>& weights) const noexcept;
    float reactionMeterAB() const noexcept;
    float reactionMeterBA() const noexcept;

private:
    struct Pair
    {
        MaterialDSP a, b;
        MaterialReactionLink reaction;
    };
    std::array<std::array<Pair,2>,4> engines;
    std::array<MorphScene,4> target {};
    std::array<MorphScene,4> previous {};
    std::array<int,4> currentBank {};
    std::array<int,4> transitionSample {};
    std::array<bool,4> transitioning {};
    int fadeSamples = 1;
    std::array<float,4> blockWeights {{ 1.0f, 0.0f, 0.0f, 0.0f }};
    MorphScene continuousBlend {};
    ReactionControls reactionSettings {};
    std::array<MorphScene,4> previousContinuous {};

    static float renderOne (Pair& pair, const MorphScene& scene,
                            const MorphScene& continuous,
                            int channel, float input,
                            ReactionControls reactions) noexcept;
    static void configureOne (Pair& pair, const MorphScene& scene,
                              const MorphScene& parameters,
                              MatterDNA::Features features,
                              MatterDNAControls controls) noexcept;
};
} // namespace organix
