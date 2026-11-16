#pragma once
#include <JuceHeader.h>
#include <array>

namespace organix::ui
{
inline const juce::Colour background { 0xff07111c };
inline const juce::Colour surface { 0xff0b1725 };
inline const juce::Colour panel { 0xff0e1d2d };
inline const juce::Colour inset { 0xff102235 };
inline const juce::Colour border { 0xff243b52 };
inline const juce::Colour line { 0xff1b3248 };
inline const juce::Colour text { 0xffe7eef8 };
inline const juce::Colour secondary { 0xff9eadc1 };
inline const juce::Colour subdued { 0xff8094ad };
inline const juce::Colour engaged { 0xff27c7a0 };
inline const std::array<juce::Colour, 6> accent {{
    juce::Colour (0xff39e0e8), juce::Colour (0xff8ec7ff),
    juce::Colour (0xffe96cff), juce::Colour (0xffb8a7ff),
    juce::Colour (0xffd8a15c), juce::Colour (0xfff2d989)
}};
inline constexpr std::array<const char*, 6> name {{
    "LIQUID", "CRYSTAL", "PLASMA", "VAPOR", "METAL", "LIGHT"
}};
inline constexpr std::array<const char*, 6> description {{
    "Fluid delay and phase motion",
    "Harmonic resonator lattice",
    "Nonlinear electric excitation",
    "Living granular atmosphere",
    "Inharmonic impact resonance",
    "Prismatic harmonic motion"
}};
inline constexpr std::array<std::array<const char*, 12>, 6> controls {{
    {{ "FLUIDITY", "VISCOSITY", "CURRENT", "RIPPLE", "DEPTH", "TURBULENCE", "WAVE SIZE", "SURFACE", "PRESSURE", "FLOW SPEED", "FEEDBACK", "STEREO FLOW" }},
    {{ "CRYSTALLIZATION", "FACETS", "BRILLIANCE", "RESONANCE", "HARMONIC ORDER", "SHIMMER", "PURITY", "GROWTH", "FRACTURE", "DETUNE", "SPARKLE", "DECAY" }},
    {{ "ENERGY", "VOLTAGE", "DISCHARGE", "ARC", "INSTABILITY", "HEAT", "IONIZATION", "SPARKS", "FREQUENCY SHIFT", "PLASMA DENSITY", "PULSE", "CONTAINMENT" }},
    {{ "VAPORIZATION", "DENSITY", "GRAIN SIZE", "DRIFT", "CLOUD SIZE", "DIFFUSION", "AIR", "FOG", "MOTION", "DISTANCE", "SCATTER", "DISSOLVE" }},
    {{ "MATERIAL", "HARDNESS", "THICKNESS", "TENSION", "RESONANCE", "RING", "IMPACT", "VIBRATION", "FRACTURE", "DAMPING", "BODY", "SHIMMER" }},
    {{ "LUMINOSITY", "RADIANCE", "SPECTRUM", "PRISM", "REFRACTION", "BEAM", "HALO", "GLOW", "SHIMMER", "PULSE", "SPEED", "EXPANSION" }}
}};
inline juce::Colour accentFor (int idx) { return accent[static_cast<size_t> (juce::jlimit (0, 5, idx))]; }
inline void drawPanel (juce::Graphics& g, juce::Rectangle<int> bounds, float radius = 8.0f)
{
    if (bounds.isEmpty()) return;
    auto rect = bounds.toFloat().reduced (0.5f);
    g.setColour (panel); g.fillRoundedRectangle (rect, radius);
    g.setColour (border); g.drawRoundedRectangle (rect, radius, 1.0f);
}
inline void drawTinyLabel (juce::Graphics& g, juce::String value, juce::Rectangle<int> r,
                           juce::Colour colour = secondary, float size = 10.0f,
                           juce::Justification alignment = juce::Justification::centredLeft)
{
    g.setColour (colour);
    g.setFont (juce::Font (juce::FontOptions (size, juce::Font::bold)));
    g.drawFittedText (value, r, alignment, 1, 0.8f);
}

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::ComboBox::backgroundColourId, inset);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, border);
        setColour (juce::TextButton::buttonColourId, surface);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::Label::textColourId, text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float proportion, float startAngle, float endAngle,
                           juce::Slider& slider) override
    {
        const auto ringColour = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const float diameter = juce::jmin (static_cast<float> (width - 4), static_cast<float> (height - 4));
        if (diameter <= 12.0f) return;
        const float radius = diameter * 0.42f;
        const float cx = static_cast<float> (x + width / 2);
        const float cy = static_cast<float> (y + height / 2);
        const float thickness = juce::jlimit (2.0f, 3.4f, diameter * 0.055f);
        const float angle = startAngle + proportion * (endAngle - startAngle);
        juce::Path track;
        track.addCentredArc (cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour (border.brighter (0.35f));
        g.strokePath (track, juce::PathStrokeType (thickness));
        juce::Path arc;
        arc.addCentredArc (cx, cy, radius, radius, 0.0f, startAngle, angle, true);
        g.setColour (ringColour);
        g.strokePath (arc, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (inset);
        g.fillEllipse (cx - radius + 5.0f, cy - radius + 5.0f, 2.0f * (radius - 5.0f), 2.0f * (radius - 5.0f));
        g.setColour (line.brighter (0.25f));
        g.drawEllipse (cx - radius + 5.0f, cy - radius + 5.0f, 2.0f * (radius - 5.0f), 2.0f * (radius - 5.0f), 1.0f);
        const auto innerR = radius * 0.67f;
        g.setColour (ringColour.brighter (0.2f));
        g.drawLine (cx + std::sin (angle) * innerR * 0.52f,
                    cy - std::cos (angle) * innerR * 0.52f,
                    cx + std::sin (angle) * innerR,
                    cy - std::cos (angle) * innerR, juce::jmax (1.4f, thickness * 0.68f));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isOver, bool isDown) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced (0.6f);
        const juce::Colour shade = isDown ? inset.brighter (0.45f) :
                                   isOver ? inset.brighter (0.24f) : backgroundColour;
        g.setColour (shade);
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (isDown ? engaged.withAlpha (0.7f) : border);
        g.drawRoundedRectangle (b, 5.0f, 1.0f);
    }
};
}
