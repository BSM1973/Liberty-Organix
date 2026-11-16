#include "../Source/MaterialReactions.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace
{
bool finite (float x) { return std::isfinite (x); }
bool isNeutral (organix::ReactionDrive x)
{
    return x.slow == 0.0f && x.excite == 0.0f &&
           x.disperse == 0.0f && x.colour == 0.0f;
}
bool allFinite (organix::ReactionDrive d)
{
    return finite (d.slow) && finite (d.excite) &&
           finite (d.disperse) && finite (d.colour) &&
           d.slow >= 0.0f && d.slow <= 1.0f &&
           d.disperse >= 0.0f && d.disperse <= 1.0f &&
           d.excite >= -1.0f && d.excite <= 1.0f;
}
float processReference (organix::MaterialDSP& a, organix::MaterialDSP& b,
                        int route, float balance, float fusion,
                        int channel, float x)
{
    if (route == 1) return b.process (channel, a.process (channel, x));
    if (route == 2) return a.process (channel, b.process (channel, x));
    const float xa = a.process (channel, x);
    const float xb = b.process (channel, x);
    float y = (1.0f - balance) * xa + balance * xb;
    if (route == 3) y += fusion * 0.27f * std::tanh (3.0f * xa * xb);
    return y;
}
float sample (int n, float fs)
{
    const float t = n / fs;
    const float pulse = n % 1400 < 50 ? 0.34f : 0.0f;
    return 0.21f * std::sin (6.283185307f * 137.0f * t) +
           0.15f * std::sin (6.283185307f * 831.0f * t) + pulse;
}
}
int main()
{
    using organix::Material;
    using organix::MaterialDSP;
    using organix::MaterialReactions;
    using organix::MaterialReactionLink;
    using organix::ReactionControls;
    const auto liquid = Material::Liquid, plasma = Material::Plasma;
    const auto metal = Material::Metal, crystal = Material::Crystal;
    const auto vapor = Material::Vapor, light = Material::Light;

    const auto slowed = MaterialReactions::drive (liquid, plasma,
                                                  0.65f, 0.4f, 0.1f, 0.1f, 1.0f);
    const auto struck = MaterialReactions::drive (metal, crystal,
                                                  0.6f, 0.8f, 0.3f, 0.8f, 1.0f);
    const auto diffused = MaterialReactions::drive (vapor, light,
                                                   0.8f, 0.3f, 0.7f, 0.4f, 1.0f);
    if (slowed.slow <= 0.10f || struck.excite <= 0.10f ||
        diffused.disperse <= 0.10f) return 1;
    if (! isNeutral (MaterialReactions::drive (metal, crystal,
                                               0.8f,0.8f,0.8f,0.7f,0.0f))) return 2;

    // 36 directed combinations have a real DSP effect parameter and an
    // explicit human-readable source-to-destination reaction identity.
    for (int src = 0; src < 6; ++src)
        for (int dst = 0; dst < 6; ++dst)
        {
            const auto source = static_cast<Material> (src);
            const auto destination = static_cast<Material> (dst);
            const auto text = MaterialReactions::description (source, destination);
            if (! text.french || ! text.english ||
                ! text.french[0] || ! text.english[0]) return 3;
            auto drive = MaterialReactions::drive (source, destination,
                                                  0.65f,0.6f,0.5f,0.8f,1.0f);
            if (! allFinite (drive) || isNeutral (drive)) return 4;
            for (auto variant : { -10.0f, 0.0f, 0.5f, 3.0f })
            {
                drive = MaterialReactions::drive (source, destination,
                                                 variant, variant, variant,
                                                 variant, variant);
                if (! allFinite (drive)) return 5;
            }
        }

    // Existing scenes must preserve their sound when the reaction is OFF.
    // Compare sample by sample against the pre-v0.9 routing implementation.
    for (int routing = 0; routing < 4; ++routing)
    {
        MaterialDSP a, b, referenceA, referenceB;
        a.prepare (48000.0, 2); b.prepare (48000.0, 2);
        referenceA.prepare (48000.0, 2); referenceB.prepare (48000.0, 2);
        a.configure (liquid, {}); b.configure (plasma, {});
        referenceA.configure (liquid, {}); referenceB.configure (plasma, {});
        MaterialReactionLink link;
        link.prepare (48000.0);
        ReactionControls disabled;
        for (int i = 0; i < 5000; ++i)
        {
            const float x = sample (i, 48000.0f);
            const float test = link.process (a,b,liquid,plasma,
                                             routing,0.4f,0.6f,0,x,disabled);
            const float reference = processReference (referenceA,referenceB,
                                                      routing,0.4f,0.6f,0,x);
            if (! finite (test) || std::abs (test - reference) > 1.0e-6f)
                return 6;
        }
    }
    // Bidirectionality, stress and silence: finite output in every route.
    for (int a = 0; a < 6; ++a)
        for (int b = 0; b < 6; ++b)
            for (int routing = 0; routing < 4; ++routing)
            {
                MaterialDSP first, second;
                first.prepare (48000.0,2); second.prepare (48000.0,2);
                first.configure (static_cast<Material> (a), {});
                second.configure (static_cast<Material> (b), {});
                MaterialReactionLink link;
                link.prepare (48000.0);
                ReactionControls controls;
                controls.enabled = true;
                controls.amount = 0.87f;
                controls.direction = (a+b)%3;
                for (int i = 0; i < 1700; ++i)
                {
                    const float in = i % 500 < 250 ? sample (i,48000.0f) : 0.0f;
                    for (int channel = 0; channel < 2; ++channel)
                    {
                        const float out = link.process (first,second,
                            static_cast<Material> (a),static_cast<Material> (b),
                            routing, 0.5f, 0.45f,channel,in,controls);
                        if (! finite (out) || std::abs (out) > 8.01f) return 7;
                    }
                }
                if (! finite (link.activityAB()) || ! finite (link.activityBA()))
                    return 8;
            }

    // These three headline pairs must actually change audio, not just
    // rename a preset or animate a GUI.
    const struct Pair { Material a, b; int route; } pairs[] = {
        {liquid, plasma, 0}, {metal, crystal, 0}, {vapor, light, 0}
    };
    for (const auto& pair : pairs)
    {
        MaterialDSP a,b,refA,refB;
        a.prepare (48000.0,1); b.prepare (48000.0,1);
        refA.prepare (48000.0,1); refB.prepare (48000.0,1);
        organix::Parameters pa, pb;
        pa.amount = 0.82f; pb.amount = 0.83f;
        pa.motion = 0.78f; pb.motion = 0.75f;
        pa.density = 0.87f; pb.density = 0.81f;
        pa.feedback = 0.5f; pb.feedback = 0.5f;
        a.configure (pair.a,pa); b.configure (pair.b,pb);
        refA.configure (pair.a,pa); refB.configure (pair.b,pb);
        MaterialReactionLink link;
        link.prepare (48000.0);
        ReactionControls on;
        on.enabled = true;
        on.amount = 1.0f;
        on.direction = 0; // directional A -> B
        float difference = 0.0f;
        for (int i = 0; i < 22000; ++i)
        {
            const float input = sample (i,48000.0f);
            const auto wet = link.process (a,b,pair.a,pair.b,
                                          pair.route,0.35f,0.7f,0,input,on);
            const auto dry = processReference (refA,refB,pair.route,0.35f,0.7f,0,input);
            difference += std::abs (wet - dry);
        }
        if (! finite (difference) || difference < 0.006f) return 9;
    }
    std::cout << "PASS: all 36 directed DSP reactions, neutral legacy path, 3 flagship changes, 4 routings, stereo stress\n";
    return 0;
}
