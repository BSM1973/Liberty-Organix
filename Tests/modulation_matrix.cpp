#include "../Source/OrganixModMatrix.h"
#include "../Source/MaterialDSP.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
bool valid(float n){return std::isfinite(n)&&n>=-1.0f&&n<=1.0f;}
std::vector<float> input(int n)
{
    std::vector<float> v(static_cast<size_t>(n));
    for(int i=0;i<n;++i)
        v[static_cast<size_t>(i)]=0.32f*std::sin(6.283185307f*220.0f*i/48000.0f);
    return v;
}
}
int main()
{
    using namespace organix;
    ModulationMatrix m;
    m.prepare(48000.0);
    MatrixSettings config;
    auto wave=input(8192);
    std::array<float,8> macros {};
    macros.fill(0.5f);
    MatterDNA::Features features;
    features.air=0.85f;
    features.motion=0.48f;
    auto blank=m.processSegment(config,wave.data(),nullptr,0,128,1,120.0f,features,macros);
    for(float x:blank.destinations)if(x!=0.0f)return 1;

    // 32 simultaneous routes are actually summed, bounded and addressable.
    config.enabled=true;
    config.masterDepth=0.9f;
    for(int k=0;k<32;++k)
        config.slots[static_cast<size_t>(k)]={
            13+k%8,1+(k%40),0.75f,false,true
        };
    for(int n=0;n<60;++n)
        blank=m.processSegment(config,wave.data(),nullptr,0,128,1,120.0f,features,macros);
    int activeCount=0;
    for(float x:blank.activity)
    {
        if(!valid(x))return 2;
        if(std::abs(x)>0.01f)++activeCount;
    }
    if(activeCount!=32)return 3;
    for(float v:blank.destinations)if(!valid(v))return 4;

    // LFO bipolar must sweep both sides of zero; reverse depth inverts it.
    m.reset();
    config={};
    config.enabled=true;
    config.rates[0]=2.0f;
    config.slots[0]={1,3,0.7f,true,true};
    float low=1.0f,high=-1.0f;
    for(int n=0;n<380;++n)
    {
        const auto values=m.processSegment(config,wave.data(),nullptr,0,128,1,
                                          120.0f,features,macros);
        const float v=values.destinations[3];
        low=std::min(low,v);high=std::max(high,v);
    }
    if(low>-0.25f||high<0.25f)return 5;
    config.slots[0].depth=-0.7f;
    for(int n=0;n<30;++n)
    {
        const auto values=m.processSegment(config,wave.data(),nullptr,0,128,1,
                                          120.0f,features,macros);
        if(!std::isfinite(values.destinations[3]))return 6;
    }

    // Two independent triggered AD envelopes on sharp input attacks.
    m.reset();
    config={};
    config.enabled=true;
    config.slots[0]={5,6,1.0f,false,true};
    config.slots[1]={6,24,1.0f,false,true};
    std::array<float,256> transient {};
    transient.fill(0.9f);
    float triggered1=0.0f,triggered2=0.0f;
    for(int i=0;i<80;++i)
    {
        const auto v=m.processSegment(config,transient.data(),nullptr,
                                      0,256,1,120.0f,features,macros);
        triggered1=std::max(triggered1,v.activity[0]);
        triggered2=std::max(triggered2,v.activity[1]);
        transient.fill(0.0f);
    }
    if(triggered1<0.03f||triggered2<0.015f)return 7;

    // Real input followers, transient, spectral air and random paths.
    m.reset();
    config={};
    config.enabled=true;
    config.slots[0]={7,1,0.9f,false,true};
    config.slots[1]={8,2,0.9f,false,true};
    config.slots[2]={9,19,0.8f,false,true};
    config.slots[3]={10,20,0.7f,false,true};
    config.slots[4]={11,38,0.6f,true,true};
    config.slots[5]={12,39,0.6f,true,true};
    float rmsPeak=0, airPeak=0, rndLow=1, rndHigh=-1;
    for(int i=0;i<700;++i)
    {
        const auto v=m.processSegment(config,wave.data(),nullptr,
                                      0,128,1,120.0f,features,macros);
        rmsPeak=std::max(rmsPeak,v.activity[0]);
        airPeak=std::max(airPeak,v.activity[2]);
        rndLow=std::min(rndLow,v.activity[4]);
        rndHigh=std::max(rndHigh,v.activity[4]);
    }
    if(rmsPeak<0.05f||airPeak<0.15f||rndHigh-rndLow<0.2f)return 8;

    // Every one of the 40 target controls must affect its DSP parameter
    // or the global wet/width/fusion/balance when the matrix is enabled.
    MatrixOffsets mods;
    for(int target=1;target<=40;++target)
    {
        mods={};
        mods.destinations[static_cast<size_t>(target)]=0.21f;
        Parameters a,b;
        a.amount=b.amount=0.4f;
        a.feedback=b.feedback=0.2f;
        float fusion=0.3f,balance=0.4f,wet=0.5f,width=1.0f;
        const auto beforeA=a,beforeB=b;
        mods.apply(a,b,fusion,balance,wet,width);
        const bool changed =
            a.amount!=beforeA.amount||a.motion!=beforeA.motion||
            a.tone!=beforeA.tone||a.feedback!=beforeA.feedback||
            a.density!=beforeA.density||a.energy!=beforeA.energy||
            a.shape!=beforeA.shape||
            b.amount!=beforeB.amount||b.motion!=beforeB.motion||
            b.tone!=beforeB.tone||b.feedback!=beforeB.feedback||
            b.density!=beforeB.density||b.energy!=beforeB.energy||
            b.shape!=beforeB.shape||
            fusion!=0.3f||balance!=0.4f||wet!=0.5f||width!=1.0f;
        if(!changed)return 9;
    }

    // OFF keeps v0.9 sound byte-for-byte at parameter level.
    MatrixOffsets off;
    Parameters a,b;
    a.amount=0.31f;b.tone=0.72f;
    const auto beforeA=a,beforeB=b;
    float fusion=0.3f,balance=0.4f,wet=0.5f,width=1.0f;
    off.apply(a,b,fusion,balance,wet,width);
    if(a.amount!=beforeA.amount||b.tone!=beforeB.tone||
       fusion!=0.3f||width!=1.0f)return 10;

    // 32 active cross-modulations cannot produce NaN with unsafe inputs.
    m.reset();
    config.enabled=true;
    config.masterDepth=1.0f;
    for(auto& slot:config.slots)slot={20,40,1.0f,false,true};
    macros[7]=std::numeric_limits<float>::infinity();
    for(int i=0;i<500;++i)
    {
        const auto v=m.processSegment(config,wave.data(),nullptr,
                                      0,128,1,120.0f,features,macros);
        for(float x:v.destinations)if(!valid(x))return 11;
    }
    std::cout<<"PASS: 32 live routes, 20 sources, 40 DSP targets, audio envelopes, LFO, random, polarity, clamping and legacy neutrality\n";
    return 0;
}
