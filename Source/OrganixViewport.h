#pragma once
#include <algorithm>
#include <cmath>

// JUCE-independent size policy used by both the plugin and the CI tests.
// Dimensions are logical display points, not Retina physical pixels.
// The host keeps the actual plugin window; we only resize our own editor.
namespace organix::layout
{
struct Viewport
{
    int contentWidth = 1240;
    int contentHeight = 820;
    int windowWidth = 1240;
    int windowHeight = 820;
    float scale = 1.0f;
};

// On macOS 13-inch laptops the title bar/host chrome must also remain visible.
// Never use the display pixel scale as a substitute for usable desktop area.
inline Viewport chooseViewport (int usableWidth, int usableHeight) noexcept
{
    // Conservative fallback for hosts which report no display on first load.
    if (usableWidth <= 0 || usableHeight <= 0)
    {
        usableWidth = 1280;
        usableHeight = 800;
    }

    constexpr int horizontalAllowance = 72;
    constexpr int verticalAllowance = 96;
    constexpr int compactWidth = 1240, compactHeight = 820;
    constexpr int studioWidth = 1504, studioHeight = 1008;
    const int allowedWidth = std::max (1, usableWidth - horizontalAllowance);
    const int allowedHeight = std::max (1, usableHeight - verticalAllowance);

    Viewport result;
    if (allowedWidth >= studioWidth && allowedHeight >= studioHeight)
    {
        result.contentWidth = studioWidth;
        result.contentHeight = studioHeight;
    }
    else
    {
        result.contentWidth = compactWidth;
        result.contentHeight = compactHeight;
    }

    result.scale = std::max (0.01f, std::min ({
        1.0f,
        static_cast<float> (allowedWidth) / result.contentWidth,
        static_cast<float> (allowedHeight) / result.contentHeight
    }));
    result.windowWidth = std::max (1, static_cast<int> (
        std::floor (result.contentWidth * result.scale)));
    result.windowHeight = std::max (1, static_cast<int> (
        std::floor (result.contentHeight * result.scale)));
    return result;
}
} // namespace organix::layout
