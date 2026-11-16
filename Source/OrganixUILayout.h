#pragma once
#include <algorithm>
#include <cmath>

// JUCE-independent geometry contract. This is the *same* geometry used by the
// JUCE editor in resized(), not a separate approximation solely for tests.
namespace organix::layout
{
struct Rect
{
    int x = 0, y = 0, w = 0, h = 0;
    int right() const noexcept { return x + w; }
    int bottom() const noexcept { return y + h; }
    bool valid() const noexcept { return w > 0 && h > 0; }
    bool intersects (const Rect& other) const noexcept
    {
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }
    bool inside (int width, int height) const noexcept
    {
        return valid() && x >= 0 && y >= 0 && right() <= width && bottom() <= height;
    }
};

struct Geometry
{
    Rect header, left, center, right, footer, fusion, movement, output;
    bool valid (int width, int height) const noexcept
    {
        return header.inside (width, height) && left.inside (width, height) &&
               center.inside (width, height) && right.inside (width, height) &&
               footer.inside (width, height) &&
               fusion.inside (width, height) && movement.inside (width, height) &&
               output.inside (width, height) &&
               ! left.intersects (center) && ! center.intersects (right) &&
               ! left.intersects (right) && ! footer.intersects (center) &&
               ! footer.intersects (left) && ! footer.intersects (right) &&
               ! fusion.intersects (movement) && ! movement.intersects (output) &&
               ! fusion.intersects (output) && center.w >= 580 &&
               center.h >= 390 && right.w >= 350;
    }
};

inline Geometry calculate (int width, int height)
{
    constexpr int gutter = 16, gap = 14, headerHeight = 94;
    const int availableW = width - 2 * gutter;
    const int footH = std::clamp (static_cast<int> (std::lround (height * 0.19)), 160, 190);
    const int statusMargin = height >= 910 ? 34 : 12;
    const int headerY = gutter;
    const int bodyY = headerY + headerHeight + gap;
    const int footerY = height - gutter - statusMargin - footH;
    const int bodyHeight = footerY - gap - bodyY;
    const int leftWidth = std::clamp (static_cast<int> (std::lround (width * 0.116)), 165, 194);
    const int rightWidth = std::clamp (static_cast<int> (std::lround (width * 0.277)), 350, 424);
    const int centerWidth = availableW - leftWidth - rightWidth - 2 * gap;
    const Rect header { gutter, headerY, availableW, headerHeight };
    const Rect left { gutter, bodyY, leftWidth, bodyHeight };
    const Rect center { left.right() + gap, bodyY, centerWidth, bodyHeight };
    const Rect right { center.right() + gap, bodyY, rightWidth, bodyHeight };
    const Rect footer { gutter, footerY, availableW, footH };

    const int fusionWidth = static_cast<int> (std::lround (availableW * 0.345));
    const int movementWidth = static_cast<int> (std::lround (availableW * 0.285)) - gap;
    const Rect fusion { footer.x, footer.y, fusionWidth, footH };
    const Rect movement { fusion.right() + gap, footer.y, movementWidth, footH };
    const Rect output { movement.right() + gap, footer.y,
                        footer.right() - (movement.right() + gap), footH };
    return { header, left, center, right, footer, fusion, movement, output };
}
} // namespace organix::layout
