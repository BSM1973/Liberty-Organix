// Audition *the actual OrganixAudioProcessor::processBlock* rather than
// the simplified MaterialDSP preview. Objective QA only: never claim that
// automatically rendered WAVs have received a human musical audition.
#include "../Source/PluginProcessor.h"
#include "../Source/FactorySoundDesigns.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;
constexpr float twoPi = 6.28318530717958647692f;
constexpr std::array<int, 16> focus {{21,22,24,26,27,28,29,30,31,32,38,81,82,84,86,89}};
constexpr std::array<const char*, 5> switchIds {{
    "dnaEnable","morphEnable","trajectoryEnable","reactionEnable","matrixEnable"
}};
constexpr std::array<unsigned,5> featureFlags {{
    organix::D,organix::M,organix::T,organix::R,organix::X
}};
bool isFocus (int oneBased) noexcept
{
    return std::find (focus.begin(),focus.end(),oneBased)!=focus.end();
}
double asDb (double amplitude) noexcept
{
    return 20.0 * std::log10 (std::max (1.0e-12,amplitude));
}
float inputSample (int category,int sample,int channel)
{
    const float t=static_cast<float>(sample/sampleRate);
    const float spread=channel==0?1.0f:1.004f;
    const float phase=channel==0?0.0f:0.031f;
    if(category==3) // decaying harmonic guitar pick, repeated
    {
        const float elapsed=std::fmod(t,0.55f);
        const float env=std::exp(-elapsed*7.0f);
        return env*(0.30f*std::sin(twoPi*110.0f*spread*t+phase)
                    +0.13f*std::sin(twoPi*220.0f*spread*t)
                    +0.055f*std::sin(twoPi*330.0f*spread*t));
    }
    if(category==4) // kick/snare-like percussive excitation
    {
        const float elapsed=std::fmod(t,0.25f);
        const float attack=std::exp(-elapsed*32.0f);
        const float pitch=58.0f+55.0f*std::exp(-elapsed*28.0f);
        return attack*(0.34f*std::sin(twoPi*pitch*t)
                     +0.16f*std::sin(twoPi*1723.0f*spread*t));
    }
    if(category==9) // pulsed rhythmic harmonic source
    {
        const float elapsed=std::fmod(t,0.25f);
        const float env=std::exp(-elapsed*15.0f);
        return env*(0.27f*std::sin(twoPi*220.0f*spread*t)
                   +0.11f*std::sin(twoPi*660.0f*t+phase));
    }
    if(category==6) // dry centered bass, low notes
        return 0.27f*std::sin(twoPi*55.0f*t)
              +0.08f*std::sin(twoPi*110.0f*t);
    if(category==2) // voice-like moving harmonic stack, not a voice recording
    {
        const float wobble=2.5f*std::sin(twoPi*4.7f*t);
        return 0.20f*std::sin(twoPi*(180.0f+wobble)*t+phase)
              +0.09f*std::sin(twoPi*360.0f*t)
              +0.05f*std::sin(twoPi*540.0f*t);
    }
    return 0.18f*std::sin(twoPi*110.0f*spread*t)
         +0.11f*std::sin(twoPi*277.18f*t+phase)
         +0.07f*std::sin(twoPi*659.25f*t);
}
void write16(std::ofstream& out,std::uint16_t value)
{
    out.put(static_cast<char>(value&255));
    out.put(static_cast<char>((value>>8)&255));
}
void write32(std::ofstream& out,std::uint32_t value)
{
    write16(out,static_cast<std::uint16_t>(value&65535));
    write16(out,static_cast<std::uint16_t>((value>>16)&65535));
}
void writeStereoWav(const std::filesystem::path& path,
                    const std::vector<float>& samples)
{
    std::ofstream out(path,std::ios::binary);
    if(!out)throw std::runtime_error("Cannot write "+path.string());
    const auto bytes=static_cast<std::uint32_t>(samples.size()*2);
    out.write("RIFF",4);write32(out,36u+bytes);
    out.write("WAVEfmt ",8);write32(out,16);
    write16(out,1);write16(out,2);write32(out,48000);
    write32(out,192000);write16(out,4);write16(out,16);
    out.write("data",4);write32(out,bytes);
    for(float value:samples)
    {
        const auto sample=static_cast<std::int16_t>(std::lround(
            std::clamp(value,-1.0f,1.0f)*32767.0f));
        write16(out,static_cast<std::uint16_t>(sample));
    }
}
struct Render
{
    double rms=0.0,peak=0.0,inputRms=0.0,correlation=0.0;
    bool finite=true;
    std::vector<float> output;
};
Render process (int program, int seconds, const char* disabledTechnology,
                bool saveSamples, bool probeMorph = false)
{
    const auto& design=organix::factoryPresets[static_cast<size_t>(program)];
    auto plugin=std::make_unique<OrganixAudioProcessor>();
    plugin->setPlayConfigDetails(2,2,sampleRate,blockSize);
    plugin->prepareToPlay(sampleRate,blockSize);
    plugin->loadFactoryPreset(program);
    if(probeMorph)
    {
        // Scene A is often identical to the base preset. Test a real
        // nonzero XY blend with trajectory disabled in BOTH arms.
        const auto setValue = [&] (const char* id,float value)
        {
            if(auto* param=plugin->parameters.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };
        setValue("trajectoryEnable",0.0f);
        setValue("morphEnable",1.0f);
        setValue("morphX",0.78f);
        setValue("morphY",0.67f);
    }
    if(disabledTechnology!=nullptr)
    {
        if(auto* param=plugin->parameters.getParameter(disabledTechnology))
            param->setValueNotifyingHost(param->convertTo0to1(0.0f));
        else throw std::runtime_error("Missing technology parameter");
    }
    juce::AudioBuffer<float> buffer(2,blockSize);
    juce::MidiBuffer midi;
    Render result;
    const int total=seconds*static_cast<int>(sampleRate);
    if(saveSamples)result.output.reserve(static_cast<size_t>(total)*2);
    double energy=0.0,inputEnergy=0.0,lr=0.0,ll=0.0,rr=0.0;
    for(int start=0;start<total;start+=blockSize)
    {
        const int size=std::min(blockSize,total-start);
        for(int i=0;i<size;++i)
        {
            const int position=start+i;
            const float a=inputSample(design.category,position,0);
            const float b=inputSample(design.category,position,1);
            buffer.setSample(0,i,a);
            buffer.setSample(1,i,b);
            inputEnergy+=0.5*(static_cast<double>(a)*a+
                              static_cast<double>(b)*b);
        }
        if(size<blockSize)
            for(int ch=0;ch<2;++ch)
                for(int i=size;i<blockSize;++i)buffer.setSample(ch,i,0.0f);
        midi.clear();
        plugin->processBlock(buffer,midi);
        for(int i=0;i<size;++i)
        {
            const double l=buffer.getSample(0,i),r=buffer.getSample(1,i);
            if(!std::isfinite(l)||!std::isfinite(r))
            { result.finite=false;break; }
            energy+=0.5*(l*l+r*r);
            result.peak=std::max(result.peak,std::max(std::abs(l),std::abs(r)));
            lr+=l*r;ll+=l*l;rr+=r*r;
            if(saveSamples)
            {
                result.output.push_back(static_cast<float>(l));
                result.output.push_back(static_cast<float>(r));
            }
        }
        if(!result.finite)break;
    }
    plugin->releaseResources();
    result.rms=std::sqrt(energy/std::max(1,total));
    result.inputRms=std::sqrt(inputEnergy/std::max(1,total));
    result.correlation=ll>0.0&&rr>0.0?lr/std::sqrt(ll*rr):0.0;
    return result;
}
std::string filename(const char* name)
{
    std::string out;
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(name);*p;++p)
    {
        const char c=static_cast<char>(*p);
        if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
           (c>='0'&&c<='9'))out.push_back(c);
        else if(c==' '||c=='-')out.push_back('_');
    }
    return out;
}
}
int main(int argc,char** argv)
{
    // The real APVTS and JUCE AudioProcessor are instantiated for each test.
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    const bool save=argc==3&&std::string(argv[1])=="--render";
    const std::filesystem::path destination=
        save?std::filesystem::path(argv[2]):std::filesystem::path();
    if(save)std::filesystem::create_directories(destination);
    std::ofstream csv;
    if(save)
    {
        csv.open(destination/"organix_real_processor_100_QA.csv");
        if(!csv)throw std::runtime_error("Cannot create processor QA report");
        csv<<"number,name,category,technology_mask,input_rms_dbfs,output_rms_dbfs,"
            <<"gain_vs_input_db,peak_dbfs,crest_db,stereo_correlation,"
            <<"DNA_off_rms_delta_db,Morph_off_rms_delta_db,"
            <<"Trajectory_off_rms_delta_db,Reactions_off_rms_delta_db,"
            <<"Matrix_off_rms_delta_db,DNA_diff_dbfs,Morph_diff_dbfs,"
            <<"Trajectory_diff_dbfs,Reactions_diff_dbfs,Matrix_diff_dbfs,"
            <<"flag,review_status\n";
    }
    int nonfinite=0,silent=0,focusCount=0,featureChecks=0;
    for(int i=0;i<100;++i)
    {
        const auto& preset=organix::factoryPresets[static_cast<size_t>(i)];
        const auto& design=organix::factorySoundDesigns[static_cast<size_t>(i)];
        const bool priority=isFocus(i+1);
        // Every program gets a processor output preview. The 16 priority
        // examples are longer, to expose feedback and slowly evolving motion.
        // All measurements use the real OrganixAudioProcessor.
        const auto on=process(i,1,nullptr,true);
        if(!on.finite){++nonfinite;continue;}
        if(on.rms<1.0e-6)++silent;
        if(save)
        {
            if(priority)++focusCount;
            const auto number=std::to_string(1000+i+1).substr(1);
            const auto extended=process(i,priority?3:2,nullptr,true);
            if(!extended.finite){++nonfinite;continue;}
            writeStereoWav(destination/(number+"_"+filename(preset.name)+
                "_real_processor.wav"),extended.output);
        }
        std::array<double,5> offDelta{}, audibleDifference{};
        offDelta.fill(std::numeric_limits<double>::quiet_NaN());
        audibleDifference.fill(std::numeric_limits<double>::quiet_NaN());
        for(size_t feature=0;feature<featureFlags.size();++feature)
        {
            if((design.technologies&featureFlags[feature])==0)continue;
            // Compare outputs with the named technology disabled, using
            // the SAME input signal and completely reset DSP/transport.
            const bool isMorphProbe = featureFlags[feature] == organix::M;
            const auto reference = isMorphProbe
                ? process(i,1,nullptr,true,true) : on;
            const auto off=process(i,1,switchIds[feature],true,isMorphProbe);
            if(!reference.finite||!off.finite){++nonfinite;continue;}
            offDelta[feature]=asDb(reference.rms)-asDb(off.rms);
            double squaredDifference=0.0;
            const auto total=std::min(reference.output.size(),off.output.size());
            for(size_t frame=0;frame<total;++frame)
            {
                const double change=reference.output[frame]-off.output[frame];
                squaredDifference+=change*change;
            }
            audibleDifference[feature]=asDb(std::sqrt(
                squaredDifference/std::max<size_t>(1,total)));
            ++featureChecks;
        }
        if(save)
        {
            csv<<i+1<<",\""<<preset.name<<"\","<<preset.category<<","
               <<static_cast<int>(design.technologies)<<","
               <<asDb(on.inputRms)<<","<<asDb(on.rms)<<","
               <<asDb(on.rms)-asDb(on.inputRms)<<","
               <<asDb(on.peak)<<","<<asDb(on.peak)-asDb(on.rms)<<","
               <<on.correlation;
            for(auto v:offDelta)
            {
                csv<<",";
                if(std::isfinite(v))csv<<v;
            }
            for(auto v:audibleDifference)
            {
                csv<<",";
                if(std::isfinite(v))csv<<v;
            }
            csv<<","<<(priority?"PRIORITY_16":"GENERAL_100")
               <<",PENDING_HUMAN_LISTENING\n";
        }
    }
    std::cout<<"Actual OrganixAudioProcessor QA: 100 programs, "
             <<featureChecks<<" feature bypass comparisons; "
             <<focusCount<<" priority WAVs among 100 real JUCE previews; silent="<<silent
             <<" nonfinite="<<nonfinite
             <<". These are synthetic sources processed by the real plugin,"
             <<" NOT a human listening evaluation or DAW hosting test.\n";
    if(nonfinite||silent)return 1;
    return 0;
}
