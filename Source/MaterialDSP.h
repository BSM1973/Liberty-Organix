#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace organix
{
enum class Material : int { Liquid, Crystal, Plasma, Vapor, Metal, Light };
enum class Routing : int { Parallel, SerialAB, SerialBA, CrossFusion };

struct Parameters
{
    float amount = 0.55f;   // 0..1
    float motion = 0.45f;   // 0..1
    float tone = 0.55f;     // 0..1
    float feedback = 0.20f; // 0..0.95
    float density = 0.5f;   // 0..1
    float energy = 0.5f;    // 0..1
    // Twelve normalized, host-automatable, per-layer material properties.
    // Values are interpreted differently for LIQUID/CRYSTAL/PLASMA/VAPOR/METAL/LIGHT.
    std::array<float, 12> shape {{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                                   0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f }};
};

struct ReactionDrive
{
    float slow = 0.0f;       // slows modulating oscillator rates
    float excite = 0.0f;     // signed injection into resonator
    float disperse = 0.0f;   // grain scatter / light diffusing tap
    float colour = 0.0f;     // limited tonal coloration
};

class MaterialDSP
{
public:
    void prepare (double sampleRate, int channels);
    void reset() noexcept;
    void configure (Material newMaterial, Parameters newParams) noexcept;
    float process (int channel, float input, ReactionDrive reaction = {}) noexcept;

private:
    static constexpr int maxChannels = 2;
    static constexpr int modes = 4;
    static constexpr int grainVoices = 8;
    static constexpr float pi = 3.14159265358979323846f;
    static float clamp (float x, float lo, float hi) noexcept { return std::clamp (x, lo, hi); }
    static float sine (float x) noexcept { return std::sin (x); }
    static float wrapPhase (float x) noexcept { return x >= 2.0f * pi ? x - 2.0f * pi : x; }

    struct Grain
    {
        float readPos = 0.0f;
        float speed = 1.0f;
        int age = 0;
        int duration = 1;
        bool active = false;
    };
    struct ChannelState
    {
        std::vector<float> delay;
        int write = 0;
        float lp = 0.0f;
        float hpLP = 0.0f;
        float smooth = 0.0f;
        float lastInput = 0.0f;
        float phase = 0.0f;
        float phase2 = 0.0f;
        float feedbackSample = 0.0f;
        float env = 0.0f;
        float oscX[modes] {};
        float oscY[modes] {};
        float lastModeExcitation = 0.0f;
        // Preallocated per-channel transition envelope: eliminates the
        // abrupt first sample when an existing engine changes material.
        float lastOutput = 0.0f;
        float switchAnchor = 0.0f;
        int switchSamples = 0;
        bool hasRendered = false;
        std::array<Grain, grainVoices> grains {};
        int samplesUntilGrain = 0;
        unsigned rng = 0x1A2B3C4Du;
    };

    float readDelay (const ChannelState& s, float delaySamples) const noexcept;
    float readAt (const ChannelState& s, float index) const noexcept;
    float random01 (ChannelState& s) const noexcept;
    float exciteModes (ChannelState& s, float excitation, bool metallic) noexcept;
    float liquid (ChannelState& s, float x, ReactionDrive reaction) noexcept;
    float crystal (ChannelState& s, float x, ReactionDrive reaction) noexcept;
    float plasma (ChannelState& s, float x, ReactionDrive reaction) noexcept;
    float vapor (ChannelState& s, float x, ReactionDrive reaction) noexcept;
    float metal (ChannelState& s, float x, ReactionDrive reaction) noexcept;
    float light (ChannelState& s, float x, ReactionDrive reaction) noexcept;
    void updateModes() noexcept;

    std::array<ChannelState, maxChannels> state {};
    std::array<float, modes> modeSin {};
    std::array<float, modes> modeCos {};
    std::array<float, modes> modeDecay {};
    Material material = Material::Liquid;
    Parameters p;
    double fs = 44100.0;
    int delayLength = 1;
    int switchFadeLength = 240; // 5 ms at 48 kHz; set during prepare()
};
}
