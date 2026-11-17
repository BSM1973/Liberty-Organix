// Stress test of the REAL OrganixAudioProcessor (not standalone material mocks).
// Host parameter changes are intentionally applied between processBlock calls;
// this never writes APVTS on the audio callback thread.
#include "../Source/PluginProcessor.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;
constexpr double pi2 = 6.2831853071795864769;
constexpr std::array<int,8> sizes {{16,32,64,128,256,512,1024,2048}};
constexpr std::array<int,3> rates {{44100,48000,96000}};

void change(OrganixAudioProcessor& plugin, const char* id,float plain)
{
    auto* param=plugin.parameters.getParameter(id);
    if(!param)throw std::runtime_error(std::string("Missing parameter: ")+id);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->convertTo0to1(plain));
    param->endChangeGesture();
}
struct Summary
{
    int sampleRate=0,bufferSize=0,channels=0,callbacks=0;
    int materialChanges=0,presetChanges=0,automationChanges=0;
    double maxAbs=0.0,maxStep=0.0,maxSwitchStep=0.0;
    double peakCpu=0.0,averageCpu=0.0,p95Cpu=0.0;
    int nonfinite=0,over0db=0,overBudget=0;
    bool valid=true;
};
Summary run(int sampleRate,int frames,int channels)
{
    auto processor=std::make_unique<OrganixAudioProcessor>();
    processor->setPlayConfigDetails(channels,channels,sampleRate,frames);
    processor->prepareToPlay(sampleRate,frames);
    change(*processor,"drywet",0.76f);
    change(*processor,"enableB",1.0f);
    change(*processor,"reactionEnable",1.0f);
    change(*processor,"reactionAmount",0.9f);
    change(*processor,"reactionDirection",2.0f);
    change(*processor,"matrixEnable",1.0f);
    change(*processor,"matrixMasterDepth",1.0f);
    change(*processor,"mm01Enable",1.0f);
    change(*processor,"mm01Source",1.0f);
    change(*processor,"mm01Target",4.0f); // feedback A
    change(*processor,"mm01Depth",0.65f);
    change(*processor,"mm01Polarity",1.0f);
    change(*processor,"aFeedback",0.88f);
    change(*processor,"bFeedback",0.88f);
    Summary report;
    report.sampleRate=sampleRate;report.bufferSize=frames;report.channels=channels;
    juce::AudioBuffer<float> data(channels,frames);
    juce::MidiBuffer midi;
    const int totalFrames=sampleRate*2; // two seconds of high-activity input
    int sent=0; double last[2]{};bool first=true;
    std::vector<double> elapsedPercents;
    elapsedPercents.reserve(static_cast<size_t>((totalFrames+frames-1)/frames));
    while(sent<totalFrames)
    {
        const int size=std::min(frames,totalFrames-sent);
        bool switching=false;
        // Sample-position changes stay independent of block size.
        const int cell=(sent/(sampleRate/12));
        const int next=(sent+size)/(sampleRate/12);
        if(cell!=next||sent==0)
        {
            const int m=(cell+2)%6;
            change(*processor,"materialA",static_cast<float>(m));
            change(*processor,"materialB",static_cast<float>((m+3)%6));
            change(*processor,"routing",static_cast<float>(cell%4));
            report.materialChanges+=2;
            switching=true;
        }
        if(sent>0 && (sent%(sampleRate/2))<frames)
        {
            processor->loadFactoryPreset((sent/(sampleRate/2)*31)%100);
            // Reapply stress after program recall.
            change(*processor,"aFeedback",0.88f);
            change(*processor,"bFeedback",0.88f);
            change(*processor,"reactionEnable",1.0f);
            change(*processor,"reactionAmount",0.9f);
            report.presetChanges++;
            switching=true;
        }
        // Fast automation, alternating every ~10ms or at a block boundary.
        const int fastSlot=sent/std::max(frames,sampleRate/100);
        const float level=(fastSlot%2)==0?0.06f:0.93f;
        change(*processor,"drywet",level);
        change(*processor,"output",fastSlot%2==0?-8.0f:6.0f);
        change(*processor,"width",fastSlot%2==0?0.7f:1.8f);
        change(*processor,"aTone",fastSlot%2==0?0.08f:0.92f);
        change(*processor,"bTone",fastSlot%2==0?0.93f:0.07f);
        report.automationChanges+=5;
        for(int ch=0;ch<channels;++ch)
            for(int i=0;i<size;++i)
            {
                const double t=double(sent+i)/sampleRate;
                const double pulse=std::exp(-std::fmod(t,0.09)*46.0);
                const double wave=0.18*std::sin(pi2*172.0*t)
                                 +0.08*std::sin(pi2*1157.0*t+ch*0.08)
                                 +0.20*pulse*std::sin(pi2*71.0*t);
                data.setSample(ch,i,static_cast<float>(wave));
            }
        for(int ch=0;ch<channels;++ch)
            for(int i=size;i<frames;++i)data.setSample(ch,i,0.0f);
        midi.clear();
        const auto start=Clock::now();
        processor->processBlock(data,midi);
        const double elapsed=std::chrono::duration<double>(Clock::now()-start).count();
        const double budget=size/double(sampleRate);
        const double cpu=100.0*elapsed/budget;
        report.peakCpu=std::max(report.peakCpu,cpu);
        if(cpu>100.0)++report.overBudget;
        elapsedPercents.push_back(cpu);
        ++report.callbacks;
        for(int i=0;i<size;++i)
        {
            for(int ch=0;ch<channels;++ch)
            {
                const double value=data.getSample(ch,i);
                if(!std::isfinite(value))
                {
                    ++report.nonfinite;
                    continue;
                }
                report.maxAbs=std::max(report.maxAbs,std::abs(value));
                if(std::abs(value)>1.001)++report.over0db;
                if(!first)
                {
                    const double step=std::abs(value-last[ch]);
                    report.maxStep=std::max(report.maxStep,step);
                    if(switching&&i==0)
                        report.maxSwitchStep=std::max(report.maxSwitchStep,step);
                }
                last[ch]=value;
            }
            first=false;
        }
        sent+=size;
    }
    processor->releaseResources();
    report.averageCpu=std::accumulate(elapsedPercents.begin(),elapsedPercents.end(),0.0)
                     /std::max<size_t>(1,elapsedPercents.size());
    std::sort(elapsedPercents.begin(),elapsedPercents.end());
    report.p95Cpu=elapsedPercents[std::min(elapsedPercents.size()-1,
                         static_cast<size_t>(0.95*(elapsedPercents.size()-1)))];
    report.valid=report.nonfinite==0&&report.over0db==0
                 &&report.maxStep<1.4&&report.maxSwitchStep<1.2;
    return report;
}
} // namespace

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const bool emit=argc==3&&std::string(argv[1])=="--report";
    std::ofstream csv;
    if(emit)
    {
        std::filesystem::create_directories(argv[2]);
        csv.open(std::filesystem::path(argv[2])/"organix_audio_stress.csv");
        csv<<"sample_rate,buffer_size,channels,callbacks,material_changes,"
             "preset_changes,automation_changes,max_abs,max_step,max_switch_step,"
             "cpu_avg_pct,cpu_p95_pct,cpu_peak_pct,over_budget_callbacks,"
             "nonfinite,over_0db,valid\n";
    }
    int passed=0,failed=0;
    for(int rate:rates)
      for(int frames:sizes)
        for(int channels:{1,2})
        {
            const auto t=run(rate,frames,channels);
            if(emit)
                csv<<t.sampleRate<<","<<t.bufferSize<<","<<t.channels<<","
                   <<t.callbacks<<","<<t.materialChanges<<","<<t.presetChanges<<","
                   <<t.automationChanges<<","<<t.maxAbs<<","<<t.maxStep<<","
                   <<t.maxSwitchStep<<","<<t.averageCpu<<","<<t.p95Cpu<<","
                   <<t.peakCpu<<","<<t.overBudget<<","<<t.nonfinite<<","
                   <<t.over0db<<","<<(t.valid?"PASS":"FAIL")<<"\n";
            (t.valid?passed:failed)++;
            std::cout<<rate<<"Hz / "<<frames<<" frames / "<<channels<<" ch: "
                     <<(t.valid?"PASS":"FAIL")<<" peak "<<t.maxAbs
                     <<" step "<<t.maxStep<<" switch "<<t.maxSwitchStep
                     <<" CPU p95 "<<t.p95Cpu<<"% (unloaded CI runner)"
                     <<" overruns "<<t.overBudget<<"/"<<t.callbacks<<"\n";
        }
    std::cout<<"REAL PROCESSOR STRESS "<<passed<<" passed / "<<failed
             <<" failed (48 combinations, sample-rate/block-size/mono-stereo). "
             <<"CI CPU timings are diagnostic only, not proof of real DAW latency.\n";
    return failed==0?0:1;
}
