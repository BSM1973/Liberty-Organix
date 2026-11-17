#include "../Source/OrganixViewport.h"
#include "../Source/OrganixUILayout.h"
#include <array>
#include <cmath>
#include <iostream>

namespace
{
bool check (int width, int height)
{
    const auto view = organix::layout::chooseViewport (width, height);
    if (view.scale <= 0.0f || view.scale > 1.0f
        || view.windowWidth <= 0 || view.windowHeight <= 0
        || view.windowWidth > width || view.windowHeight > height)
        return false;

    if (view.windowWidth > width - 72
        || view.windowHeight > height - 96)
        return false;

    // Full legacy JUCE editor remains inside its proven geometry envelope.
    const auto geometry = organix::layout::calculate (
        view.contentWidth, view.contentHeight);
    if (! geometry.valid (view.contentWidth, view.contentHeight))
        return false;

    // The two overlay pages share this same scaled viewport; no scroll bars
    // or direct DPI-dependent changes to individual GUI controls are needed.
    if (view.contentWidth < 1240 || view.contentHeight < 820)
        return false;

    const float transformedWidth = view.contentWidth * view.scale;
    const float transformedHeight = view.contentHeight * view.scale;
    if (std::fabs (transformedWidth - view.windowWidth) >= 1.1f
        || std::fabs (transformedHeight - view.windowHeight) >= 1.1f)
        return false;

    return true;
}
}

int main()
{
    // Emulate logical macOS display userAreas, not physical Retina pixels.
    constexpr std::array<std::pair<int, int>, 9> screens {{
        {1280, 800},   // common 13-inch logical display mode
        {1280, 720},   // reduced DAW workspace
        {1440, 900},   // 13-inch "more space" mode
        {1512, 982},   // compact/high-density laptop
        {1024, 768},   // small desktop
        {1600, 1000},  // mid-size monitor
        {1920, 1080},  // external display, constrained height
        {1920, 1200},  // studio reference layout
        {2560, 1440}   // large external display
    }};

    for (const auto& [width, height] : screens)
    {
        if (! check (width, height))
        {
            std::cerr << "Viewport overflow at " << width << "x"
                      << height << "\n";
            return 1;
        }
        const auto result = organix::layout::chooseViewport (width, height);
        std::cout << width << "x" << height << " -> "
                  << result.windowWidth << "x" << result.windowHeight
                  << " (content " << result.contentWidth << "x"
                  << result.contentHeight << ", zoom " << result.scale
                  << ")\n";
    }
    const auto fallback = organix::layout::chooseViewport (0, 0);
    if (fallback.windowWidth < 1 || fallback.windowHeight < 1)
        return 2;

    std::cout << "PASS: full no-scroll viewport on 9 simulated display sizes\n";
    return 0;
}
