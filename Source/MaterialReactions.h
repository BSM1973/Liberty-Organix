#pragma once

#include "MaterialDSP.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace organix
{
// Changes to the actual material DSP, not a crossfade or postmix.
// All fields are bounded and default-neutral for old Organix projects.
struct ReactionDrive
{
    float slow = 0.0f;       // reduces modulation speed of Liquid/Plasma
    float excite = 0.0f;     // signed excitation of Crystal/Metal oscillators
    float disperse = 0.0f;   // scatters Vapor grains / diffuses Light taps
    float colour = 0.0f;     // small bipolar spectral / speed deviation
};

struct ReactionControls
{
    bool enabled = false;
    float amount = 0.5f;    // 0..1
    int direction = 2;      // 0 A to B, 1 B to A, 2 both
    float response = 0.40f; // 0..1, 5..95 ms envelope time
};

struct ReactionDescription
{
    const char* french;
    const char* english;
};

// A curated and directional 6x6 matrix. The source is the row and the
// destination the column. The destination's actual DSP is modulated.
class MaterialReactions
{
public:
    static ReactionDescription description (Material source,
                                             Material destination) noexcept;
    static ReactionDrive drive (Material source, Material destination,
                                float body, float attack, float air,
                                float signedEdge, float amount) noexcept;
};

class MaterialReactionLink
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    float process (MaterialDSP& a, MaterialDSP& b,
                   Material materialA, Material materialB,
                   int routing, float balance, float fusion,
                   int channel, float input,
                   const ReactionControls& controls) noexcept;

    float activityAB() const noexcept { return meterAB; }
    float activityBA() const noexcept { return meterBA; }
private:
    struct Detector
    {
        float body = 0.0f;
        float high = 0.0f;
        float attack = 0.0f;
        float previous = 0.0f;
        float edge = 0.0f;
        void feed (float sample, float alpha) noexcept;
    };
    struct Channel
    {
        Detector a, b;
        float amountAB = 0.0f, amountBA = 0.0f;
    };
    std::array<Channel,2> state {};
    float meterAB = 0.0f, meterBA = 0.0f;
    float rate = 48000.0f;
};
} // namespace organix
