#include "../Source/FactoryPresets.h"
#include "../Source/MaterialDSP.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
constexpr int sampleRate=48000;
constexpr float tau=6.2831853071795864769f;
float clamp01(float x){return std::clamp(x,0.0f,1.0f);}
float db(float x){return 20.0f*std::log10(std::max(1.0e-9f,x));}
float sourceSample(int category,int sample)
{
    const float t=static_cast<float>(sample)/sampleRate;
    const float burst=static_cast<float>(sample%16000)/16000.0f;
    const float beat=std::exp(-burst*18.0f);
    switch(category)
    {
        case 2: return 0.15f*std::sin(tau*190.0f*t)+
                       0.08f*std::sin(tau*380.0f*t)+
                       0.05f*std::sin(tau*570.0f*t);
        case 3: return (0.11f*std::sin(tau*147.0f*t)+
                        0.09f*std::sin(tau*294.0f*t))*beat;
        case 4: return beat*(0.25f*std::sin(tau*(70.0f-40.0f*burst)*t)+
                        0.11f*std::sin(tau*3877.0f*t));
        case 5: return 0.13f*std::sin(tau*220.0f*t)+
                       0.08f*std::sin(tau*330.0f*t)+
                       0.05f*std::sin(tau*550.0f*t);
        case 6: return 0.23f*std::sin(tau*55.0f*t)+
                       0.045f*std::sin(tau*110.0f*t);
        case 9: return beat*(0.13f*std::sin(tau*220.0f*t)+
                             0.10f*std::sin(tau*880.0f*t));
        default: return 0.12f*std::sin(tau*110.0f*t)+
                        0.08f*std::sin(tau*277.18f*t)+
                        0.05f*std::sin(tau*659.25f*t);
    }
}
void write16(std::ofstream& out,std::uint16_t n)
{
    out.put(static_cast<char>(n&0xffu));
    out.put(static_cast<char>((n>>8)&0xffu));
}
void write32(std::ofstream& out,std::uint32_t n)
{
    write16(out,static_cast<std::uint16_t>(n&0xffffu));
    write16(out,static_cast<std::uint16_t>(n>>16));
}
void wav(const std::filesystem::path& path,const std::vector<float>& samples)
{
    std::ofstream out(path,std::ios::binary);
    if(!out)throw std::runtime_error("Cannot create "+path.string());
    const auto count=static_cast<std::uint32_t>(samples.size());
    const std::uint32_t dataBytes=count*2u;
    out.write("RIFF",4);write32(out,36u+dataBytes);
    out.write("WAVEfmt ",8);write32(out,16);
    write16(out,1);write16(out,1);
    write32(out,sampleRate);write32(out,sampleRate*2);
    write16(out,2);write16(out,16);
    out.write("data",4);write32(out,dataBytes);
    for(auto x:samples)
    {
        const auto clamped=std::clamp(x,-1.0f,1.0f);
        const int q=static_cast<int>(std::lround(clamped*32767.0f));
        write16(out,static_cast<std::uint16_t>(
            static_cast<std::int16_t>(q)));
    }
}
std::string safeName(const char* raw)
{
    std::string output;
    for(const char* p=raw;*p;++p)
    {
        const char c=*p;
        if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
           (c>='0'&&c<='9'))output.push_back(c);
        else if(c==' '||c=='-')output.push_back('_');
    }
    return output;
}
struct Measure
{
    double rms=0,peak=0,difference=0,inputRms=0;
    bool clipped=false;
};
Measure render(int index,int seconds,std::vector<float>* samples)
{
    using namespace organix;
    const auto& preset=factoryPresets[static_cast<size_t>(index)];
    const int number=index%10;
    const float t=static_cast<float>(number)/9.0f;
    const bool subtle=preset.category==10||preset.category==7;
    const bool aggressive=preset.category==8||preset.category==4;
    const bool rhythmic=preset.category==9||preset.category==4;
    const bool ambient=preset.category==1||preset.category==7;
    const float base=subtle?0.19f:aggressive?0.75f:ambient?0.60f:0.46f;
    const float balance=0.32f+0.33f*t;
    const float fusion=aggressive?0.65f:subtle?0.10f:0.25f;
    const float wet=subtle?0.30f:aggressive?0.80f:ambient?0.88f:0.64f;
    const float outputGain=std::pow(10.0f,(aggressive?-4.0f:-1.5f)/20.0f);
    const int routing=preset.b<0?0:rhythmic?3:index%3==0?1:0;
    Parameters a,b;
    a.amount=clamp01(base+0.16f*t);
    b.amount=clamp01(base+0.14f*(1.0f-t));
    a.motion=clamp01((rhythmic?0.70f:ambient?0.18f:0.35f)+0.24f*t);
    b.motion=clamp01((rhythmic?0.55f:ambient?0.12f:0.27f)+0.20f*(1.0f-t));
    a.tone=clamp01(0.15f+0.68f*((index*7)%17)/16.0f);
    b.tone=clamp01(0.2f+0.6f*((index*11)%19)/18.0f);
    a.feedback=subtle?0.08f:ambient?0.50f:0.20f+0.20f*t;
    b.feedback=subtle?0.10f:ambient?0.42f:0.15f+0.23f*(1.0f-t);
    a.energy=b.energy=aggressive?0.76f:0.42f+t*0.2f;
    a.density=b.density=ambient?0.75f:0.3f+0.4f*t;
    MaterialDSP dspA,dspB;
    dspA.prepare(sampleRate,1);
    dspB.prepare(sampleRate,1);
    dspA.configure(preset.a,a);
    dspB.configure(static_cast<Material>(preset.b<0?
                   (static_cast<int>(preset.a)+1)%6:preset.b),b);
    const int count=seconds*sampleRate;
    if(samples){samples->clear();samples->reserve(static_cast<size_t>(count));}
    double squares=0,inputSquares=0,differences=0,peak=0;
    for(int i=0;i<count;++i)
    {
        const float x=sourceSample(preset.category,i);
        float fx=0.0f;
        if(preset.b<0)fx=dspA.process(0,x);
        else if(routing==1)fx=dspB.process(0,dspA.process(0,x));
        else if(routing==2)fx=dspA.process(0,dspB.process(0,x));
        else
        {
            const float ya=dspA.process(0,x);
            const float yb=dspB.process(0,x);
            fx=ya*(1.0f-balance)+yb*balance;
            if(routing==3)
                fx+=fusion*0.27f*std::tanh(3.0f*ya*yb);
        }
        const float y=(x*(1.0f-wet)+fx*wet)*outputGain;
        if(!std::isfinite(y))throw std::runtime_error(
            "nonfinite output "+std::to_string(index));
        squares+=static_cast<double>(y)*y;
        inputSquares+=static_cast<double>(x)*x;
        differences+=static_cast<double>(y-x)*static_cast<double>(y-x);
        peak=std::max(peak,static_cast<double>(std::abs(y)));
        if(samples)samples->push_back(std::clamp(y,-1.0f,1.0f));
    }
    const double n=static_cast<double>(count);
    return {std::sqrt(squares/n),peak,std::sqrt(differences/n),
            std::sqrt(inputSquares/n),peak>=0.999};
}
}
int main(int argc,char** argv)
{
    const bool save=argc==3&&std::string(argv[1])=="--render";
    std::filesystem::path destination;
    if(save)
    {
        destination=argv[2];
        std::filesystem::create_directories(destination);
    }
    std::ofstream report;
    if(save)
    {
        report.open(destination/"factory_preset_audio_qa.csv");
        if(!report)throw std::runtime_error("Cannot save QA CSV");
        report<<"number,name,category,source,output_rms_dbfs,peak_dbfs,crest_db,delta_rms_dbfs,suggested_trim_db,flag\n";
    }
    int silent=0,nonfinite=0,peaky=0,low=0;
    double smallest=1.0e6,largest=-1.0e6;
    for(int i=0;i<100;++i)
    {
        std::vector<float> preview;
        Measure m;
        try{m=render(i,save?3:1,save?&preview:nullptr);}
        catch(const std::exception& ex)
        {std::cerr<<ex.what()<<'\n';++nonfinite;continue;}
        const double rmsDb=db(static_cast<float>(m.rms));
        const double peakDb=db(static_cast<float>(m.peak));
        const double deltaDb=db(static_cast<float>(m.difference));
        const double proposedTrim=std::clamp(-18.0-rmsDb,-6.0,6.0);
        const bool isSilent=m.rms<0.001;
        const bool tooLow=rmsDb<-35.0;
        if(isSilent)++silent;
        if(tooLow)++low;
        if(m.clipped)++peaky;
        smallest=std::min(smallest,rmsDb);
        largest=std::max(largest,rmsDb);
        if(save)
        {
            const auto& p=organix::factoryPresets[static_cast<size_t>(i)];
            std::string flag=m.clipped?"NEAR_0DBFS":tooLow?"LOW_LEVEL":"CHECK_BY_EAR";
            report<<i+1<<",\""<<p.name<<"\","<<p.category
                  <<",SYNTHETIC_48KHZ,"
                  <<rmsDb<<","<<peakDb<<","<<peakDb-rmsDb
                  <<","<<deltaDb<<","<<proposedTrim<<","<<flag<<"\n";
            const char indexBuffer[8]={};
            (void)indexBuffer;
            auto filename=std::to_string(1000+i+1).substr(1)+"_"+
                          safeName(p.name)+".wav";
            wav(destination/filename,preview);
        }
    }
    std::cout<<"DSP QA: 100 factory preset foundations, RMS range "
             <<smallest<<".."<<largest
             <<" dBFS; silent="<<silent
             <<" low(-35dB)="<<low
             <<" near0dB="<<peaky<<" nonfinite="<<nonfinite
             <<". Synthetic source tests are NOT listening reviews.\n";
    if(nonfinite||silent)return 1;
    return 0;
}
