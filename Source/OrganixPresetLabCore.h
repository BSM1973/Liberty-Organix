#pragma once
#include "FactoryPresets.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace organix
{
// Metadata search is descriptive and explicitly NOT measured spectral
// classification. Real sound character must be auditioned in a DAW.
enum class PresetCharacter : int
{
    All=0, Ambient, Warm, Bright, Dark, Rhythmic, Organic,
    Metallic, Aggressive, Soft, Wide, Experimental
};

inline const char* characterLabel(PresetCharacter ch,bool french) noexcept
{
    constexpr const char* fr[]={
        "TOUS LES CARACTÈRES","ATMOSPHÉRIQUE","CHAUD","BRILLANT",
        "SOMBRE","RYTHMIQUE","ORGANIQUE","MÉTALLIQUE",
        "AGRESSIF","DOUX","SPACIEUX","EXPÉRIMENTAL"
    };
    constexpr const char* en[]={
        "ALL CHARACTERS","ATMOSPHERIC","WARM","BRIGHT","DARK",
        "RHYTHMIC","ORGANIC","METALLIC","AGGRESSIVE","SOFT",
        "SPACIOUS","EXPERIMENTAL"
    };
    const int index=std::clamp(static_cast<int>(ch),0,11);
    return french?fr[index]:en[index];
}
inline bool containsWord(std::string_view text,std::string_view word) noexcept
{
    if(word.empty()||word.size()>text.size())return false;
    for(size_t i=0;i+word.size()<=text.size();++i)
    {
        size_t k=0;
        for(;k<word.size();++k)
        {
            char a=text[i+k],b=word[k];
            if(a>='A'&&a<='Z')a=static_cast<char>(a+32);
            if(b>='A'&&b<='Z')b=static_cast<char>(b+32);
            if(a!=b)break;
        }
        if(k==word.size())return true;
    }
    return false;
}
inline bool matchesCharacter(const FactoryPreset& p,PresetCharacter ch) noexcept
{
    if(ch==PresetCharacter::All)return true;
    const std::string_view desc=p.description;
    const std::string_view name=p.name;
    const auto has=[&](std::string_view s)
        {return containsWord(desc,s)||containsWord(name,s);};
    switch(ch)
    {
        case PresetCharacter::Ambient:
            return p.category==1||p.category==7||has("atmosphere")||
                   has("cloud")||has("ambient")||has("ethereal")||
                   has("diffuse")||has("ghost");
        case PresetCharacter::Warm:
            return has("warm")||has("soft")||has("smooth")||has("gentle")||
                   has("organic")||p.category==10;
        case PresetCharacter::Bright:
            return p.a==Material::Light||has("bright")||has("shimmer")||
                   has("luminous")||has("radiant")||has("air");
        case PresetCharacter::Dark:
            return has("dark")||has("ominous")||has("frozen")||
                   has("industrial")||has("heavy");
        case PresetCharacter::Rhythmic:
            return p.category==4||p.category==9||has("pulse")||
                   has("beat")||has("sequenc");
        case PresetCharacter::Organic:
            return p.a==Material::Liquid||has("organic")||has("water")||
                   has("flow")||has("natural");
        case PresetCharacter::Metallic:
            return p.a==Material::Metal||has("metal")||
                   has("resonan")||has("percuss");
        case PresetCharacter::Aggressive:
            return p.category==8||has("aggressive")||has("destruct")||
                   has("distortion")||has("intense")||has("powerful");
        case PresetCharacter::Soft:
            return p.category==7||p.category==10||has("delicate")||
                   has("subtle")||has("gentle")||has("peaceful");
        case PresetCharacter::Wide:
            return p.category==1||has("stereo")||has("expansive")||
                   has("space")||has("wide")||has("infinite");
        case PresetCharacter::Experimental:
            return p.category==8||has("experimental")||has("quantum")||
                   has("alien")||has("glitch")||has("unusual");
        case PresetCharacter::All: return true;
    }
    return false;
}

// Each group lock is independent. Output/input gain, bypass, material IDs
// and the primary routing are NEVER randomized: safe audition first.
enum LockGroup : unsigned
{
    LockLayerA=1u<<0,LockLayerB=1u<<1,LockSpace=1u<<2,
    LockMotion=1u<<3,LockMacros=1u<<4
};
struct VariationParameter
{
    const char* id;
    float minimum;
    float maximum;
    float magnitude; // fraction of full parameter range at intensity 1
    unsigned group;
};
// 44 deliberately selected actual JUCE APVTS parameter IDs.
inline constexpr std::array<VariationParameter,44> variationParameters {{
    {"aAmount",0,1,0.22f,LockLayerA},
    {"aMotion",0,1,0.24f,LockLayerA},
    {"aTone",0,1,0.23f,LockLayerA},
    {"aFeedback",0,0.70f,0.15f,LockLayerA},
    {"aShape01",0,1,0.16f,LockLayerA},
    {"aShape02",0,1,0.16f,LockLayerA},
    {"aShape03",0,1,0.15f,LockLayerA},
    {"aShape04",0,1,0.10f,LockLayerA},
    {"aShape05",0,1,0.17f,LockLayerA},
    {"aShape06",0,1,0.15f,LockLayerA},
    {"bAmount",0,1,0.22f,LockLayerB},
    {"bMotion",0,1,0.23f,LockLayerB},
    {"bTone",0,1,0.20f,LockLayerB},
    {"bFeedback",0,0.70f,0.15f,LockLayerB},
    {"bShape01",0,1,0.16f,LockLayerB},
    {"bShape02",0,1,0.16f,LockLayerB},
    {"bShape03",0,1,0.14f,LockLayerB},
    {"bShape04",0,1,0.12f,LockLayerB},
    {"bShape05",0,1,0.15f,LockLayerB},
    {"bShape06",0,1,0.15f,LockLayerB},
    {"fusion",0,1,0.19f,LockSpace},
    {"balance",0.15f,0.85f,0.21f,LockSpace},
    {"drywet",0.12f,0.93f,0.12f,LockSpace},
    {"width",0.70f,1.55f,0.20f,LockSpace},
    {"motionRate",0.05f,3.0f,0.10f,LockMotion},
    {"morphX",0,1,0.12f,LockMotion},
    {"morphY",0,1,0.12f,LockMotion},
    {"trajectorySpeed",0.05f,3.0f,0.10f,LockMotion},
    {"trajectoryDepth",0,1,0.15f,LockMotion},
    {"macroTransform",0,1,0.18f,LockMacros},
    {"macroMotion",0,1,0.18f,LockMacros},
    {"macroEnergy",0,1,0.16f,LockMacros},
    {"macroDensity",0,1,0.16f,LockMacros},
    {"macroSpace",0,1,0.18f,LockMacros},
    {"macroHarmonics",0,1,0.16f,LockMacros},
    {"macroEvolution",0,1,0.18f,LockMacros},
    {"macroFusion",0,1,0.14f,LockMacros},
    {"reactionAmount",0,0.82f,0.12f,LockSpace},
    {"reactionResponse",0,1,0.15f,LockMotion},
    {"dnaDepth",0,0.85f,0.14f,LockMotion},
    {"dnaSensitivity",0,1,0.14f,LockMotion},
    {"spectralTransient",-0.50f,0.50f,0.13f,LockLayerA},
    {"matrixMasterDepth",0.15f,0.93f,0.14f,LockMacros},
    {"matrixRandomRate",0.05f,4.0f,0.10f,LockMotion},
}};

class PresetVariationEngine
{
public:
    explicit PresetVariationEngine(std::uint32_t seed=0x2151973u) noexcept : state(seed?seed:1u) {}
    void reseed(std::uint32_t seed) noexcept {state=seed?seed:1u;}
    float nextSigned() noexcept
    {
        state^=state<<13;state^=state>>17;state^=state<<5;
        return (static_cast<float>((state>>8)&0xffffu)/32767.5f)-1.0f;
    }
    // Operates on the same 44 values as the host UI; all locks are strict,
    // and extreme parameters are limited before sending them to the DSP.
    int mutate(std::array<float,variationParameters.size()>& values,
               float amount,unsigned locks) noexcept
    {
        amount=std::isfinite(amount)?std::clamp(amount,0.0f,1.0f):0.0f;
        int changes=0;
        for(size_t i=0;i<values.size();++i)
        {
            const auto& p=variationParameters[i];
            const float signedValue=nextSigned();
            const float probability=nextSigned();
            if((locks&p.group)!=0||amount<=0.0f||probability<-0.20f)continue;
            const float old=std::isfinite(values[i])
                ?std::clamp(values[i],p.minimum,p.maximum)
                :(p.minimum+p.maximum)*0.5f;
            float next=old+signedValue*amount*p.magnitude*
                             (p.maximum-p.minimum);
            next=std::clamp(next,p.minimum,p.maximum);
            // Steps >0.004 prevent a preset variation without audible or
            // controllable consequences due to UI quantization.
            if(std::abs(next-old)<0.004f)continue;
            values[i]=next;
            ++changes;
        }
        return changes;
    }
private:
    std::uint32_t state;
};
}
