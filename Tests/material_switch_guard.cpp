#include "../Source/MaterialDSP.h"
#include <array>
#include <cmath>
#include <iostream>

int main()
{
    for(double sampleRate : {44100.0,48000.0,96000.0})
    {
        organix::MaterialDSP engine;
        engine.prepare(sampleRate,2);
        organix::Parameters p;
        p.amount=0.9f;p.motion=0.8f;p.energy=0.9f;
        p.tone=0.8f;p.feedback=0.88f;p.density=0.9f;
        p.shape.fill(0.8f);
        float last[2]{},maxBoundary=0.0f;
        int changes=0;
        for(int material=0;material<6;++material)
        {
            engine.configure(static_cast<organix::Material>(material),p);
            for(int i=0;i<static_cast<int>(sampleRate*0.075);++i)
                for(int channel=0;channel<2;++channel)
                {
                    const auto t=static_cast<float>((i+material*sampleRate*0.075)/sampleRate);
                    const auto input=0.31f*std::sin(6.2831853f*127.0f*t)
                                    +0.11f*std::sin(6.2831853f*1532.0f*t);
                    const auto output=engine.process(channel,input);
                    if(!std::isfinite(output)||std::abs(output)>4.0001f)
                    {
                        std::cerr<<"Unbounded / nonfinite DSP output\n";
                        return 1;
                    }
                    if(material>0 && i==0)
                    {
                        const float discontinuity=std::abs(output-last[channel]);
                        maxBoundary=std::max(maxBoundary,discontinuity);
                        if(discontinuity>1.0e-5f)
                        {
                            std::cerr<<"Switch discontinuity at "<<sampleRate
                                     <<"Hz material="<<material
                                     <<" step="<<discontinuity<<"\n";
                            return 2;
                        }
                        ++changes;
                    }
                    last[channel]=output;
                }
        }
        if(changes!=10)return 3;
        std::cout<<"PASS: material switch crossfade "<<sampleRate
                 <<"Hz, zero first-sample wet jump; max "<<maxBoundary<<"\n";
    }
    // Configuration caching must not change the actual DSP output.
    // These two engines receive exactly the same signal and parameters;
    // one is redundantly reconfigured every 16 frames, as tiny buffers do.
    for(double sampleRate : {44100.0,96000.0})
    {
        organix::MaterialDSP stable,repeated;
        stable.prepare(sampleRate,2);
        repeated.prepare(sampleRate,2);
        organix::Parameters p;
        p.amount=0.8f;p.tone=0.6f;p.feedback=0.7f;
        p.motion=0.6f;p.density=0.5f;p.energy=0.7f;
        p.shape.fill(0.65f);
        stable.configure(organix::Material::Metal,p);
        repeated.configure(organix::Material::Metal,p);
        for(int i=0;i<8192;++i)
        {
            if(i%16==0)
                repeated.configure(organix::Material::Metal,p);
            const float x=0.27f*std::sin(6.2831853f*
                                  static_cast<float>(i*210.0/sampleRate));
            const float a=stable.process(0,x);
            const float b=repeated.process(0,x);
            if(!std::isfinite(a)||!std::isfinite(b)||std::abs(a-b)>1.0e-6f)
            {
                std::cerr<<"Unstable DSP under redundant configure at "<<sampleRate
                         <<"Hz, frame "<<i<<"\n";
                return 4;
            }
        }
        std::cout<<"PASS: repeated 16-frame configure remains sample-exact at "
                 <<sampleRate<<"Hz\n";
    }
    return 0;
}
