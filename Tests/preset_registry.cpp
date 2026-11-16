#include "../Source/FactoryPresets.h"
#include <cassert>
#include <set>
#include <string>
#include <iostream>

int main()
{
    std::set<std::string> unique;
    int single = 0, paired = 0;
    for (const auto& preset : organix::factoryPresets)
    {
        assert (preset.category >= 1 && preset.category <= 10);
        assert (preset.a >= organix::Material::Liquid && preset.a <= organix::Material::Light);
        assert (preset.b >= -1 && preset.b <= 5);
        assert (unique.insert (preset.name).second);
        assert (std::string (preset.description).size() > 3);
        if (preset.b < 0) ++single; else ++paired;
    }
    assert (unique.size() == 100);
    assert (single > 0 && paired > 0);
    std::cout << "OK: 100 unique factory names, validated engine pairing and categories\n";
}
