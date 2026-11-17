#include "../Source/OrganixSpectralView.h"
#include "../Source/OrganixSpectralTransient.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
constexpr float tau=6.2831853071795864769f;
bool finite(float v){return std::isfinite(v)&&v>=0.0f&&v<=1.0f;}
}
int main()
{
    using A=organix::SpectralViewAnalyzer;
    A a;
    a.prepare(48000.0);
    std::array<float,256> before{},after{};
    for(int block=0;block<75;++block)
    {
        for(int i=0;i<256;++i)
        {
            const int n=block*256+i;
            before[static_cast<size_t>(i)]=0.25f*std::sin(tau*220.0f*n/48000.0f);
            after[static_cast<size_t>(i)]=0.55f*std::sin(tau*5000.0f*n/48000.0f);
        }
        a.analyze(A::Stream::Before,before.data(),nullptr,256,1);
        a.analyze(A::Stream::After,after.data(),nullptr,256,1);
    }
    int beforeMax=0,afterMax=0;
    for(int k=0;k<A::displayBands;++k)
    {
        if(!finite(a.band(A::Stream::Before,k))||
           !finite(a.band(A::Stream::After,k)))return 1;
        if(a.band(A::Stream::Before,k)>a.band(A::Stream::Before,beforeMax))
            beforeMax=k;
        if(a.band(A::Stream::After,k)>a.band(A::Stream::After,afterMax))
            afterMax=k;
    }
    // Real spectral content must move on the frequency axis, not merely
    // change a decorative RMS animation.
    if(beforeMax>=afterMax-12||beforeMax<1||afterMax>=A::displayBands)
        return 2;
    if(a.energy(A::Stream::Before)<=0.05f||
       a.energy(A::Stream::After)<=a.energy(A::Stream::Before))
        return 3;
    if(a.brightness(A::Stream::After)<=a.brightness(A::Stream::Before))
        return 4;

    // Silence resets the published spectral data towards zero.
    before.fill(0.0f);
    after.fill(0.0f);
    for(int block=0;block<90;++block)
    {
        a.analyze(A::Stream::Before,before.data(),nullptr,256,1);
        a.analyze(A::Stream::After,after.data(),nullptr,256,1);
    }
    if(a.energy(A::Stream::Before)>0.001f)return 5;
    for(int k=0;k<A::displayBands;++k)
        if(a.band(A::Stream::Before,k)>0.03f)return 6;

    // Real output transients should differ from the input if a transient
    // arrives only on the output side.
    a.reset();
    for(int block=0;block<10;++block)
    {
        before.fill(0.0f);after.fill(0.0f);
        if(block==1||block==5)after[0]=0.92f;
        a.analyze(A::Stream::Before,before.data(),nullptr,256,1);
        a.analyze(A::Stream::After,after.data(),nullptr,256,1);
    }
    if(a.transient(A::Stream::After)<=a.transient(A::Stream::Before))
        return 7;

    // Stereo must be safe, and bad input must never leak NaN to the GUI.
    a.prepare(192000.0);
    before.fill(std::numeric_limits<float>::quiet_NaN());
    after.fill(0.15f);
    for(int i=0;i<30;++i)
        a.analyze(A::Stream::Before,before.data(),after.data(),256,2);
    for(int k=0;k<A::displayBands;++k)
        if(!finite(a.band(A::Stream::Before,k)))return 8;
    if(!finite(a.motion(A::Stream::Before))||
       !finite(a.centroid(A::Stream::Before)))return 9;

    // Transient shaper is real DSP, neutral at zero depth, bounded for
    // both positive enhancement and negative softening.
    organix::SpectralTransientShaper neutral,boost,soften;
    neutral.prepare(48000.0);boost.prepare(48000.0);soften.prepare(48000.0);
    float sumBoost=0.0f,sumSoften=0.0f;
    for(int i=0;i<9600;++i)
    {
        const float x=i%1600<36?0.40f:0.045f;
        const float unchanged=neutral.process(0,x,0.0f);
        const float added=boost.process(0,x,1.0f);
        const float reduced=soften.process(0,x,-1.0f);
        if(unchanged!=x||!std::isfinite(added)||!std::isfinite(reduced))
            return 10;
        if(added>1.00001f||reduced < -1.00001f)return 11;
        if(i%1600<36)
        {
            sumBoost+=added;
            sumSoften+=reduced;
        }
    }
    if(sumBoost<sumSoften*1.10f)return 12;
    if(neutral.process(1,0.10f,0.0f)!=0.10f)return 13;
    std::cout<<"PASS: dual FFT 48-band pitch separation, before/after energy, silence, output transients, 192kHz stereo NaN, neutral/enhance/soften transient DSP\n";
    return 0;
}
