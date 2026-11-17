#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace organix
{
// Real sound processor, not an animation-only transient detector.
// Input is the calculated wet effect signal; the final host bypass is
// applied afterward. Depth zero preserves the input bit-for-bit.
class SpectralTransientShaper
{
public:
    void prepare(double sampleRate) noexcept
    {
        const double fs=std::isfinite(sampleRate)
            ?std::clamp(sampleRate,8000.0,384000.0):48000.0;
        fastCoefficient=1.0f-std::exp(
            -1.0f/static_cast<float>(fs*0.0018));
        slowCoefficient=1.0f-std::exp(
            -1.0f/static_cast<float>(fs*0.045));
        reset();
    }
    void reset() noexcept
    {
        fast.fill(0.0f);slow.fill(0.0f);
    }
    float process(int channel,float input,float signedDepth) noexcept
    {
        if(channel<0||channel>=2)return std::isfinite(input)?input:0.0f;
        const float x=std::isfinite(input)?std::clamp(input,-1.0f,1.0f):0.0f;
        auto& attack=fast[static_cast<size_t>(channel)];
        auto& body=slow[static_cast<size_t>(channel)];
        const float absolute=std::abs(x);
        attack+=(absolute-attack)*fastCoefficient;
        body+=(absolute-body)*slowCoefficient;
        const float depth=std::isfinite(signedDepth)
            ?std::clamp(signedDepth,-1.0f,1.0f):0.0f;
        if(depth==0.0f)return x;
        const float transient=std::clamp(
            (attack-body)/(0.06f+body*1.2f),0.0f,1.0f);
        const float gain=depth>=0.0f
            ?1.0f+depth*transient*0.90f
            :1.0f+depth*transient*0.75f;
        return std::clamp(x*gain,-1.0f,1.0f);
    }
private:
    std::array<float,2> fast{},slow{};
    float fastCoefficient=0.0f,slowCoefficient=0.0f;
};
}
