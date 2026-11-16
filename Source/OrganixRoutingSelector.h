#pragma once
#include "OrganixI18n.h"
#include "PluginProcessor.h"
#include <array>

// Four direct, host-automatable routing choices. No platform popup menu.
// Graphical paths and labels are translated, while the APVTS 'routing' ID stays stable.
class OrganixRoutingSelector final : public juce::Component
{
public:
    class Tile final : public juce::TextButton
    {
    public:
        explicit Tile (int index = 0) : mode (index) {}
        void setState (bool selected, bool french)
        {
            if (active != selected || fr != french)
            {
                active = selected; fr = french; repaint();
            }
        }
        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            using namespace organix::ui;
            const auto bounds = getLocalBounds().toFloat().reduced (0.6f);
            const auto c = active ? accentFor (0) : secondary;
            g.setColour (down ? inset.brighter (0.38f) : active ? inset.brighter (0.16f)
                        : over ? inset.brighter (0.08f) : surface);
            g.fillRoundedRectangle (bounds, 5.0f);
            g.setColour (active ? c.withAlpha (0.82f) : border);
            g.drawRoundedRectangle (bounds, 5.0f, active ? 1.4f : 0.9f);

            // Mini patch diagram: parallel, serial A->B, serial B->A, interactive cross.
            const float x = 13.0f, y = bounds.getCentreY();
            g.setColour (c);
            g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
            const char* schemes[] = {"A+B", "A>B", "B>A", "A×B"};
            g.drawFittedText (juce::String::fromUTF8 (schemes[mode]),
                              { 5, 2, 38, getHeight() - 4 }, juce::Justification::centred, 1);
            juce::ignoreUnused (x, y);
            static constexpr const char* french[] = {"PARALLÈLE", "SÉRIE A→B", "SÉRIE B→A", "CROISÉE"};
            static constexpr const char* english[] = {"PARALLEL", "SERIAL A→B", "SERIAL B→A", "CROSS"};
            g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
            g.drawFittedText (juce::String::fromUTF8 (fr ? french[mode] : english[mode]),
                              { 42, 2, juce::jmax (20, getWidth() - 47), getHeight() - 4 },
                              juce::Justification::centredLeft, 1, 0.84f);
        }
    private:
        int mode = 0;
        bool active = false, fr = true;
    };
    explicit OrganixRoutingSelector (OrganixAudioProcessor& p) : audio (p)
    {
        for (int i = 0; i < 4; ++i)
        {
            tiles[static_cast<size_t> (i)].setState (i == 0, true);
            tiles[static_cast<size_t> (i)].onClick = [this, i]
            {
                if (auto* param = audio.parameters.getParameter ("routing"))
                {
                    param->beginChangeGesture();
                    param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (i)));
                    param->endChangeGesture();
                }
                refresh();
            };
            addAndMakeVisible (tiles[static_cast<size_t> (i)]);
        }
    }
    void setFrench (bool value) { french = value; refresh(); }
    void refresh()
    {
        const auto* value = audio.parameters.getRawParameterValue ("routing");
        const int selected = value == nullptr ? 0 : juce::jlimit (0, 3, static_cast<int> (value->load()));
        for (int i = 0; i < 4; ++i)
            tiles[static_cast<size_t> (i)].setState (selected == i, french);
    }
    void resized() override
    {
        // 2x2 at the smallest editor size; at larger sizes this remains precise.
        auto r = getLocalBounds();
        const auto top = r.removeFromTop (r.getHeight() / 2);
        const int halfTop = top.getWidth() / 2;
        const int halfBottom = r.getWidth() / 2;
        tiles[0].setBounds (top.getX(), top.getY(), halfTop - 2, top.getHeight() - 2);
        tiles[1].setBounds (top.getX() + halfTop + 2, top.getY(), top.getWidth() - halfTop - 2, top.getHeight() - 2);
        tiles[2].setBounds (r.getX(), r.getY() + 2, halfBottom - 2, r.getHeight() - 2);
        tiles[3].setBounds (r.getX() + halfBottom + 2, r.getY() + 2, r.getWidth() - halfBottom - 2, r.getHeight() - 2);
    }
private:
    OrganixAudioProcessor& audio;
    bool french = true;
    std::array<Tile, 4> tiles {{ Tile (0), Tile (1), Tile (2), Tile (3) }};
};
