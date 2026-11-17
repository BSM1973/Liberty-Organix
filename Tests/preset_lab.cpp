#include "../Source/OrganixPresetLabCore.h"
#include <array>
#include <cmath>
#include <iostream>
#include <set>

int main()
{
    using namespace organix;
    if(variationParameters.size()!=44)return 1;
    for(const auto& spec:variationParameters)
        if(spec.id==nullptr||spec.minimum>=spec.maximum||
           spec.magnitude<=0||spec.magnitude>0.5f||spec.group==0)
            return 2;

    int tagsWithMatches=0;
    for(int character=1;character<=11;++character)
    {
        int matches=0;
        for(const auto& preset:factoryPresets)
            if(matchesCharacter(preset,static_cast<PresetCharacter>(character)))
                ++matches;
        if(matches<=0 || matches>=100)return 3;
        ++tagsWithMatches;
    }
    if(tagsWithMatches!=11)return 4;
    for(const auto& preset:factoryPresets)
    {
        int count=0;
        for(int i=1;i<=11;++i)
            if(matchesCharacter(preset,static_cast<PresetCharacter>(i)))
                ++count;
        if(count==0)return 5;
    }
    std::array<float,variationParameters.size()> initial{};
    for(size_t i=0;i<initial.size();++i)
        initial[i]=(variationParameters[i].minimum+
                     variationParameters[i].maximum)*0.5f;
    PresetVariationEngine e1(1973u),e2(1973u);
    auto a=initial,b=initial;
    const auto changedA=e1.mutate(a,0.7f,LockLayerA|LockMacros);
    const auto changedB=e2.mutate(b,0.7f,LockLayerA|LockMacros);
    if(changedA<6 || changedA!=changedB || a!=b)return 6;
    for(size_t i=0;i<a.size();++i)
    {
        const auto& spec=variationParameters[i];
        if(!std::isfinite(a[i])||a[i]<spec.minimum||a[i]>spec.maximum)
            return 7;
        if((spec.group&(LockLayerA|LockMacros))!=0&&a[i]!=initial[i])
            return 8;
        if(std::abs(a[i]-initial[i])>0.5f*(spec.maximum-spec.minimum))
            return 9;
    }
    auto none=initial;
    if(e1.mutate(none,1.0f,LockLayerA|LockLayerB|
                           LockSpace|LockMotion|LockMacros)!=0||none!=initial)
        return 10;
    none=initial;
    if(e1.mutate(none,0.0f,0)!=0||none!=initial)return 11;
    std::cout<<"PASS: 100 factory presets indexed with 11 descriptive characters; 44 bounded musical controls, 5 exact locks, deterministic random/zero locks\n";
    return 0;
}
