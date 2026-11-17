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
    // The new MORPH button is an additional 86px. All three editor pages,
    // FR/EN, browser, preset toolbar and Liberty wordmark must remain separate.
    const int actionSpan = 86 + 82 + 86 + 120 + 127 + 8;
    const int toolbarWidth = std::min (485, std::max (310, g.header.w - actionSpan - 370));
    const int toolbarLeft = g.header.right() - actionSpan - toolbarWidth;
    const int brandRight = g.header.x + 59 + 290;
    if (toolbarWidth < 310 || toolbarLeft - brandRight < 16) return false;

    // Six DNA knobs + five live feature meters share a separate footer page.
    const int meterX = static_cast<int> (std::lround (g.footer.w * 0.605));
    const int dialCell = std::max (73, (meterX - 43) / 6);
    const int lastDialRight = 11 + 6 * dialCell;
    const int meterBarStart = meterX + 129;
    const int meterBarWidth = g.footer.w - meterBarStart - 43;
    if (lastDialRight >= meterX - 9 || meterBarWidth < 40 ||
        g.footer.h < 158 || meterX < 500) return false;
    // Morph pad and four scene cards must fit their own dedicated views.
    const int fieldWidth = g.center.w - 160;
    const int fieldHeight = g.center.h - 235;
    const int morphCardWidth = (g.footer.w - 24) / 4;
    const int morphButtonWidth = (morphCardWidth - 24) / 2 - 2;
    if (fieldWidth < 400 || fieldHeight < 180 ||
        morphCardWidth < 240 || morphButtonWidth < 100 ||
        g.footer.h - 45 < 110) return false;
    // Trajectory Designer occupies the same right panel as 12 material
    // controls, but never at the same time. Its 436px tall control column
    // must fit at the minimum editor height and leave footer separation.
    const int trajectoryRightPadding = 16;
    const int trajectoryBottom = 436;
    const int trajectoryControlWidth = g.right.w - 2 * trajectoryRightPadding;
    if (trajectoryControlWidth < 300 || trajectoryBottom > g.right.h - 22 ||
        g.right.intersects (g.center) || g.right.intersects (g.footer)) return false;
    // Four right-panel tabs (A/B/REACTIONS/MATRIX) remain independent.
    // MATRIX opens a full-width overlay below the header, not a fifth
    // button crammed into the top toolbar.
    const int tabWidth = (g.right.w - 52) / 4;
    const Rect tabA {g.right.x + 15, g.right.y + 12, tabWidth, 32};
    const Rect tabB {g.right.x + 19 + tabWidth, g.right.y + 12, tabWidth, 32};
    const Rect reactTab {g.right.x + 23 + 2 * tabWidth, g.right.y + 12, tabWidth, 32};
    const Rect matrixTab {g.right.x + 27 + 3 * tabWidth, g.right.y + 12, tabWidth, 32};
    const Rect reactionPanel {g.right.x + 10, g.right.y + 50,
                              g.right.w - 20, g.right.h - 58};
    if (tabWidth < 73 || tabA.intersects (tabB) ||
        tabB.intersects (reactTab) ||
        reactTab.intersects (matrixTab) ||
        matrixTab.right() > g.right.right() ||
        tabA.intersects (reactTab) ||
        reactTab.intersects (reactionPanel) ||
        reactionPanel.intersects (g.center) ||
        reactionPanel.intersects (g.footer) ||
        reactionPanel.x < g.right.x ||
        reactionPanel.right() > g.right.right() ||
        reactionPanel.y < g.right.y ||
        reactionPanel.bottom() > g.right.bottom() ||
        reactionPanel.h < 436) return false;
    // Real reaction controls: power 64..94, direction 125..154,
    // amount 191..222, response 250..281, result cards 288..346
    // and 350..408, with separate footer at panelHeight - 22.
    const Rect power {0,64,reactionPanel.w,30}, direction {0,125,reactionPanel.w,29};
    const Rect amount {0,191,reactionPanel.w,31}, response {0,250,reactionPanel.w,31};
    const Rect cardAB {0,288,reactionPanel.w,58}, cardBA {0,350,reactionPanel.w,58};
    const Rect finalText {0,reactionPanel.h-22,reactionPanel.w,15};
    if (power.intersects (direction) || direction.intersects (amount) ||
        amount.intersects (response) || response.intersects (cardAB) ||
        cardAB.intersects (cardBA) || cardBA.intersects (finalText)) return false;
    const Rect matrixWorkspace {g.left.x,g.left.y,
                                g.right.right()-g.left.x,
                                g.footer.bottom()-g.left.y};
    const int sidebarX=matrixWorkspace.w-316;
    const int matrixListColWidth=(sidebarX-35)/2;
    const int matrixListBottom=184+16*25;
    if (matrixWorkspace.w < 1190 ||
        matrixWorkspace.h < 650 ||
        matrixWorkspace.y < g.header.bottom() ||
        matrixWorkspace.right() > width ||
        matrixWorkspace.bottom() > height ||
        matrixListColWidth < 370 ||
        matrixListBottom + 37 > matrixWorkspace.h ||
        // Four envelope sliders end at y=474+3*33+29.
        474 + 3*33+29 > matrixWorkspace.h-22) return false;
    // SPECTRAL VIEW v1.1 retains all six skins and the existing XY field.
    // Its mode toggle and transient editor sit strictly in the 89..115
    // title row; FFT curves stay above XY drag help and 4-column meters.
    const int spectrumW=g.center.w, spectrumH=g.center.h;
    const Rect transientLabel {153,89,96,25};
    const Rect transientSlider {250,89,157,26};
    const Rect modeButton {spectrumW-137,89,117,26};
    const Rect topStatus {24,89,125,24};
    const Rect fftPlot {31,138,spectrumW-62,
        std::min(80,static_cast<int>((spectrumH-214)*0.21f))};
    const int meterY=spectrumH-73;
    const int colW=std::max(85,(spectrumW-40)/4);
    const Rect lastMeter {23+3*colW,meterY+9,colW-12,45};
    if (transientLabel.intersects(transientSlider) ||
        transientSlider.intersects(modeButton) ||
        topStatus.intersects(transientLabel) ||
        transientSlider.bottom()>=fftPlot.y ||
        modeButton.right()>spectrumW ||
        fftPlot.w < 500 || fftPlot.h < 20 ||
        fftPlot.bottom()>spectrumH-130 ||
        lastMeter.right()>spectrumW-16 ||
        lastMeter.bottom()>spectrumH-10) return false;
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
