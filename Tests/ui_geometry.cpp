#include "../Source/OrganixUILayout.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <utility>

static bool accepts (int width, int height)
{
    const auto g = organix::layout::calculate (width, height);
    if (!g.valid (width, height)) return false;
    // The 3 x 4 parameter grid is large enough for rotary + label + value.
    const int gridWidth = g.right.w - 30;
    const int gridHeight = g.right.h - 16 - 139 - 31;
    const int cellWidth = gridWidth / 3;
    const int cellHeight = gridHeight / 4;
    if (cellWidth < 100 || cellHeight < 70) return false;
    const int knobHeight = cellHeight - 4 - 18 - 18 - 2;
    if (knobHeight < 26) return false;

    // All six left cards must end above the legend at panel bottom.
    const int available = std::max (270, g.left.h - 72 - 50);
    const int cardH = std::max (38, (available - 5 * 7) / 6);
    const int lastCardBottom = g.left.y + 72 + 5 * (cardH + 7) + cardH;
    if (lastCardBottom > g.left.bottom() - 40) return false;

    // Footer knob grids must have sufficient width to avoid collision.
    if ((g.fusion.w - 24) / 3 < 100) return false;
    if ((g.movement.w - 24) / 2 < 100) return false;
    if ((g.output.w - 24) / 2 < 100) return false;

    // New two-row routing selector must never cover the A/B label area or
    // the three Fusion rotary controls, even at 1240x820.
    using organix::layout::Rect;
    const Rect routing { g.fusion.right() - 252, g.fusion.y + 8, 240, 63 };
    const Rect heading { g.fusion.x + 17, g.fusion.y + 10, 137, 19 };
    const Rect layerA { g.fusion.x + 18, g.fusion.y + 32, 139, 17 };
    const Rect layerB { g.fusion.x + 18, g.fusion.y + 49, 139, 17 };
    const Rect fusionDials { g.fusion.x + 12, g.fusion.y + 76,
                             g.fusion.w - 24, g.fusion.h - 81 };
    if (routing.x < g.fusion.x || routing.right() > g.fusion.right() ||
        routing.y < g.fusion.y || routing.bottom() > g.fusion.bottom()) return false;
    if (routing.intersects (heading) || routing.intersects (layerA) ||
        routing.intersects (layerB) || routing.intersects (fusionDials)) return false;
    if (fusionDials.h < 70) return false;

    // Brand and preset toolbar (including French/English menu) must not collide.
    const int actionSpan = 86 + 120 + 127 + 8;
    const int toolbarWidth = std::min (485, g.header.w - actionSpan - 315);
    const int toolbarLeft = g.header.right() - actionSpan - toolbarWidth;
    const int brandRight = g.header.x + 59 + 290;
    if (toolbarWidth < 360 || toolbarLeft - brandRight < 16) return false;
    return true;
}
int main()
{
    int checked = 0;
    for (int width = 1240; width <= 1940; width += 20)
    {
        for (int height = 820; height <= 1220; height += 20)
        {
            if (! accepts (width, height))
            {
                std::cerr << "UI panel overlap / undersized control at "
                          << width << " x " << height << "\n";
                return 1;
            }
            ++checked;
        }
    }
    const std::array<std::pair<int, int>, 6> critical {{
        {1240, 820}, {1504, 1008}, {1940, 1220},
        {1240, 1220}, {1940, 820}, {1450, 875}
    }};
    for (const auto& [width, height] : critical)
    {
        if (! accepts (width, height))
        {
            std::cerr << "UI critical viewport failed: " << width << " x " << height << "\n";
            return 2;
        }
    }
    std::cout << "PASS: " << checked << " sizes; panel/knob/sidebar geometry never overlaps\n";
}
