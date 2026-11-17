#include "../Source/FactorySoundDesigns.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>

int main(int argc,char** argv)
{
    using namespace organix;
    static_assert(factoryPresets.size()==100);
    static_assert(factorySoundDesigns.size()==100);
    std::set<std::string> distinctDesigns;
    std::array<int,5> featureUsage {};
    int paired=0,moving=0,reacting=0,matrices=0,quietMix=0,bassSafe=0;
    for(size_t i=0;i<factorySoundDesigns.size();++i)
    {
        const auto& p=factoryPresets[i];
        const auto& d=factorySoundDesigns[i];
        if(p.name==nullptr||p.name[0]=='\0')return 1;
        const float numeric[]={
            d.aAmount,d.aMotion,d.aTone,d.aFeedback,
            d.bAmount,d.bMotion,d.bTone,d.bFeedback,
            d.wet,d.width,d.outputDb,d.energy,d.density,
            d.dnaDepth,d.reactionDepth,d.transient,
            d.speed,d.modulationDepth
        };
        for(float v:numeric)if(!std::isfinite(v))return 2;
        for(float v:{d.aAmount,d.aMotion,d.aTone,d.bAmount,d.bMotion,d.bTone,
                     d.energy,d.density,d.wet,d.dnaDepth,d.reactionDepth})
            if(v<0.0f||v>1.0f)return 3;
        if(d.aFeedback<0.0f||d.aFeedback>0.55f||
           d.bFeedback<0.0f||d.bFeedback>0.55f||
           d.width<0.75f||d.width>1.40f||
           d.outputDb>0.0f||d.outputDb < -12.0f||
           d.transient<-0.40f||d.transient>0.40f)return 4;
        if(d.trajectoryShape>3||d.scenePalette>4||
           d.speed<0.05f||d.speed>6.0f)return 5;
        const std::uint8_t flags=d.technologies;
        if(flags==0||(flags&(~static_cast<unsigned>(D|M|T|R|X)))!=0)
            return 6;
        for(int k=0;k<5;++k)if((flags&(1u<<k))!=0)++featureUsage[k];
        if((flags&T)!=0)++moving;
        if((flags&R)!=0)++reacting;
        if((flags&X)!=0)
        {
            ++matrices;
            if(d.modulationSource<1||d.modulationSource>20||
               d.modulationTarget<1||d.modulationTarget>40||
               std::abs(d.modulationDepth)<0.01f||
               std::abs(d.modulationDepth)>0.6f)return 7;
        }
        if(p.b>=0||((flags&(M|T|R))!=0))++paired;
        if(p.category==6)
        {
            if(d.width>1.05f||d.wet>0.60f||
               d.outputDb>-3.5f)return 8;
            ++bassSafe;
        }
        if(p.category==10)
        {
            if(d.wet>0.40f||d.width>1.17f||
               d.outputDb>-0.1f)return 9;
            ++quietMix;
        }
        // Exact, not merely "distinct by index": disallow repeated sonic
        // configurations despite different names/categories.
        std::ostringstream fingerprint;
        for(float v:numeric)
            fingerprint<<std::lround(v*1000.0f)<<':';
        fingerprint<<static_cast<int>(d.technologies)<<':'
                   <<static_cast<int>(d.scenePalette)<<':'
                   <<static_cast<int>(d.modulationSource)<<':'
                   <<static_cast<int>(d.modulationTarget);
        if(!distinctDesigns.insert(fingerprint.str()).second)return 10;
    }
    if(distinctDesigns.size()!=100||bassSafe!=10||quietMix!=10||
       paired<45||moving<25||reacting<12||matrices<30)return 11;
    for(auto n:featureUsage)if(n<12)return 12;
    if(argc==3&&std::string(argv[1])=="--export")
    {
        const std::filesystem::path p(argv[2]);
        if(!p.parent_path().empty())
            std::filesystem::create_directories(p.parent_path());
        std::ofstream report(p,std::ios::binary);
        if(!report)return 13;
        report<<"number,name,category,material_A,material_B,DNA,"
              <<"Morph_4D,Trajectory,Material_Reactions,Mod_Matrix,"
              <<"A_amount,A_motion,A_tone,A_feedback,"
              <<"B_amount,B_motion,B_tone,B_feedback,"
              <<"wet,width,output_db,macro_energy,macro_density,"
              <<"dna_depth,reaction_amount,transient_shape,"
              <<"trajectory_shape,trajectory_speed,"
              <<"matrix_source,matrix_destination,matrix_depth,scene_palette\n";
        for(size_t i=0;i<factoryPresets.size();++i)
        {
            const auto& p=factoryPresets[i];
            const auto& d=factorySoundDesigns[i];
            const auto mask=d.technologies;
            report<<(i+1)<<",\""<<p.name<<"\","<<p.category<<","
                  <<static_cast<int>(p.a)<<","
                  <<(p.b>=0?p.b:
                     (static_cast<int>(p.a)+(d.scenePalette%3)+1)%6)<<","
                  <<((mask&D)!=0)<<","<<((mask&(M|T))!=0)<<","
                  <<((mask&T)!=0)<<","<<((mask&R)!=0)<<","
                  <<((mask&X)!=0)<<","
                  <<d.aAmount<<","<<d.aMotion<<","<<d.aTone<<","<<d.aFeedback<<","
                  <<d.bAmount<<","<<d.bMotion<<","<<d.bTone<<","<<d.bFeedback<<","
                  <<d.wet<<","<<d.width<<","<<d.outputDb<<","
                  <<d.energy<<","<<d.density<<","
                  <<d.dnaDepth<<","<<d.reactionDepth<<","<<d.transient<<","
                  <<static_cast<int>(d.trajectoryShape)<<","<<d.speed<<","
                  <<static_cast<int>(d.modulationSource)<<","
                  <<static_cast<int>(d.modulationTarget)<<","
                  <<d.modulationDepth<<","<<static_cast<int>(d.scenePalette)<<"\n";
        }
        if(!report.good())return 14;
    }
    std::cout<<"PASS 100 individual profiles; DNA="<<featureUsage[0]
             <<", Morph="<<featureUsage[1]
             <<", Trajectory="<<featureUsage[2]
             <<", Reactions="<<featureUsage[3]
             <<", Matrix="<<featureUsage[4]
             <<"; all 10 bass and 10 mix presets use safe output/width limits\n";
    return 0;
}
