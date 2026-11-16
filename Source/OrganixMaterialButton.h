#pragma once
#include "OrganixUITheme.h"
#include <cmath>

namespace organix::ui
{
inline void drawMaterialGlyph (juce::Graphics& g, int material,
                               juce::Rectangle<float> area, juce::Colour colour)
{
    const float x = area.getCentreX(), y = area.getCentreY();
    const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.39f;
    g.setColour (colour.withAlpha (0.88f));
    juce::Path p;
    switch (material)
    {
        case 0: // Three fluid ripples
            for (int row = -1; row <= 1; ++row)
            {
                p.clear();
                p.startNewSubPath (x - r, y + row * r * 0.57f);
                for (int j = 1; j <= 20; ++j)
                {
                    const float t = static_cast<float> (j) / 20.0f;
                    p.lineTo (x + r * (2.0f * t - 1.0f),
                              y + row * r * 0.57f + std::sin (t * juce::MathConstants<float>::twoPi * 1.6f) * r * 0.13f);
                }
                g.strokePath (p, juce::PathStrokeType (1.25f));
            }
            break;
        case 1: // Crystal diamond
            p.startNewSubPath (x, y - r);
            p.lineTo (x + r * 0.75f, y);
            p.lineTo (x, y + r);
            p.lineTo (x - r * 0.75f, y);
            p.closeSubPath();
            g.strokePath (p, juce::PathStrokeType (1.25f));
            g.drawLine (x, y - r, x, y + r, 0.9f);
            g.drawLine (x - r * 0.75f, y, x + r * 0.75f, y, 0.9f);
            break;
        case 2: // Electrical bolt
            p.startNewSubPath (x + r * 0.06f, y - r);
            p.lineTo (x - r * 0.58f, y + r * 0.08f);
            p.lineTo (x + r * 0.02f, y + r * 0.08f);
            p.lineTo (x - r * 0.10f, y + r);
            p.lineTo (x + r * 0.63f, y - r * 0.15f);
            p.lineTo (x + r * 0.09f, y - r * 0.15f);
            p.closeSubPath();
            g.strokePath (p, juce::PathStrokeType (1.3f));
            break;
        case 3: // Air and vapor
            p.startNewSubPath (x - r * 0.86f, y + r * 0.37f);
            p.cubicTo (x - r * 1.14f, y - r * 0.3f, x - r * 0.30f, y - r * 0.57f, x - r * 0.1f, y - r * 0.25f);
            p.cubicTo (x + r * 0.1f, y - r * 1.11f, x + r * 1.13f, y - r * 0.74f, x + r * 0.79f, y + r * 0.18f);
            g.strokePath (p, juce::PathStrokeType (1.2f));
            g.drawLine (x - r * 0.67f, y + r * 0.65f, x + r * 0.67f, y + r * 0.65f, 1.1f);
            break;
        case 4: // Metal pentagon
            for (int k = 0; k <= 5; ++k)
            {
                const float a = -juce::MathConstants<float>::halfPi +
                                k * juce::MathConstants<float>::twoPi / 5.0f;
                const float px = x + std::cos (a) * r, py = y + std::sin (a) * r;
                if (k == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            p.closeSubPath();
            g.strokePath (p, juce::PathStrokeType (1.2f));
            g.drawLine (x, y, x + r * 0.73f, y - r * 0.55f, 0.75f);
            g.drawLine (x, y, x - r * 0.78f, y - r * 0.40f, 0.75f);
            break;
        case 5: // Radiating light
            g.drawEllipse (x - r * 0.37f, y - r * 0.37f, r * 0.74f, r * 0.74f, 1.25f);
            for (int k = 0; k < 8; ++k)
            {
                const float a = k * juce::MathConstants<float>::twoPi / 8.0f;
                g.drawLine (x + std::cos (a) * r * 0.60f, y + std::sin (a) * r * 0.60f,
                            x + std::cos (a) * r, y + std::sin (a) * r, 1.1f);
            }
            break;
    }
}

class MaterialButton final : public juce::TextButton
{
public:
    explicit MaterialButton (int materialNumber = 0) : material (materialNumber) {}
    void setMaterial (int value) { material = juce::jlimit (0, 5, value); repaint(); }
    void setLayerState (bool isSelected, bool isA, bool isB)
    {
        if (selected != isSelected || a != isA || b != isB)
        {
            selected = isSelected; a = isA; b = isB;
            repaint();
        }
    }

    void paintButton (juce::Graphics& g, bool hovered, bool pressed) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const auto accentColour = accentFor (material);
        g.setColour (pressed ? inset.brighter (0.34f) :
                     selected ? inset.brighter (0.11f) :
                     hovered ? surface.brighter (0.18f) : surface);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (selected ? accentColour.withAlpha (0.64f) : border);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
        if (selected)
        {
            g.setColour (accentColour);
            g.fillRoundedRectangle (2.0f, 6.0f, 3.0f, r.getHeight() - 11.0f, 1.0f);
        }
        drawMaterialGlyph (g, material, { 11.0f, r.getCentreY() - 13.0f, 26.0f, 26.0f }, accentColour);
        g.setColour (selected ? accentColour : secondary);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawFittedText (name[static_cast<size_t> (material)],
                          { 46, static_cast<int> (r.getCentreY() - 15.0f), juce::jmax (62, getWidth() - 71), 18 },
                          juce::Justification::centredLeft, 1, 0.84f);
        g.setColour (subdued);
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.drawFittedText (juce::String (material + 1).paddedLeft ('0', 2) + "  /  MATERIAL",
                          { 46, static_cast<int> (r.getCentreY() + 4.0f), juce::jmax (60, getWidth() - 60), 16 },
                          juce::Justification::centredLeft, 1, 0.76f);
        if (a || b)
        {
            g.setColour (accentColour.withAlpha (0.9f));
            g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
            g.drawFittedText (a && b ? "A+B" : a ? "A" : "B",
                              { getWidth() - 35, 5, 28, 14 },
                              juce::Justification::centredRight, 1);
        }
    }
private:
    int material = 0;
    bool selected = false, a = false, b = false;
};
}
