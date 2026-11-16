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

    // Missing reference-panel controls: FREE/SYNC, trajectory, division and
    // three Output knobs including actual M/S width, at every supported size.
    const int choiceWidth = std::clamp (
        static_cast<int> (std::lround (g.movement.w * 0.45)), 140, 170);
    const int choiceX = g.movement.right() - choiceWidth - 12;
    const Rect freeSwitch { g.movement.right() - 87, g.movement.y + 10, 74, 25 };
    const Rect trajectoryLabel { choiceX, g.movement.y + 42, choiceWidth, 17 };
    const Rect trajectoryMenu { choiceX, g.movement.y + 61, choiceWidth, 29 };
    const Rect divisionLabel { choiceX, g.movement.y + 98, choiceWidth, 16 };
    const Rect divisionMenu { choiceX, g.movement.y + 116, choiceWidth, 29 };
    const int leftKnobWidth = (choiceX - (g.movement.x + 10) - 4) / 2;
    const Rect leftKnobs { g.movement.x + 10, g.movement.y + 57,
                           2 * leftKnobWidth, g.movement.h - 62 };
    if (! freeSwitch.valid() || ! trajectoryLabel.valid() ||
        ! trajectoryMenu.valid() || ! divisionLabel.valid() ||
        ! divisionMenu.valid() || ! leftKnobs.valid()) return false;
    for (const auto& r : {freeSwitch, trajectoryLabel, trajectoryMenu,
                          divisionLabel, divisionMenu, leftKnobs})
        if (r.x < g.movement.x || r.right() > g.movement.right() ||
            r.y < g.movement.y || r.bottom() > g.movement.bottom()) return false;
    if (leftKnobWidth < 64 || leftKnobs.intersects (trajectoryMenu) ||
        leftKnobs.intersects (divisionMenu) ||
        trajectoryMenu.intersects (divisionMenu)) return false;
    const int outputKnobWidth = (g.output.w - 24) / 3;
    if (outputKnobWidth < 100 ||
        3 * outputKnobWidth > g.output.w - 24) return false;

    // Brand and preset toolbar (including plain FR/EN buttons) must not collide.
    // Extra 82px DNA tab, preserving the brand without forcing the preset
    // toolbar into the Liberty wordmark on compact 1240px editor widths.
    const int actionSpan = 86 + 82 + 120 + 127 + 8;
    const int toolbarWidth = std::min (485, std::max (340, g.header.w - actionSpan - 366));
    const int toolbarLeft = g.header.right() - actionSpan - toolbarWidth;
    const int brandRight = g.header.x + 59 + 290;
    if (toolbarWidth < 360 || toolbarLeft - brandRight < 16) return false;

    // Six DNA knobs + five live feature meters share a separate footer page.
    const int meterX = static_cast<int> (std::lround (g.footer.w * 0.605));
    const int dialCell = std::max (73, (meterX - 43) / 6);
    const int lastDialRight = 11 + 6 * dialCell;
    const int meterBarStart = meterX + 129;
    const int meterBarWidth = g.footer.w - meterBarStart - 43;
    if (lastDialRight >= meterX - 9 || meterBarWidth < 40 ||
        g.footer.h < 158 || meterX < 500) return false;
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
