#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace ui = organix::ui;
namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float tau = juce::MathConstants<float>::twoPi;

float vParam (OrganixAudioProcessor& a, const char* id)
{
    if (auto* v = a.parameters.getRawParameterValue (id))
        return v->load (std::memory_order_relaxed);
    return 0.0f;
}
void title (juce::Graphics& g, const juce::String& str, juce::Rectangle<int> r,
            float sz, juce::Colour colour, bool bold = false,
            juce::Justification align = juce::Justification::centredLeft)
{
    g.setColour (colour);
    g.setFont (juce::Font (juce::FontOptions (sz, bold ? juce::Font::bold : juce::Font::plain)));
    g.drawFittedText (str, r, align, 1, 0.83f);
}
void polygon (juce::Path& path, float cx, float cy, float radius,
              int sides, float rotation, float aspect = 1.0f)
{
    for (int k = 0; k <= sides; ++k)
    {
        const float angle = rotation + static_cast<float> (k) / sides * tau;
        const float x = cx + std::cos (angle) * radius * aspect;
        const float y = cy + std::sin (angle) * radius;
        if (k == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    path.closeSubPath();
}
}

MatterView::MatterView (OrganixAudioProcessor& p) : audio (p)
{
    setInterceptsMouseClicks (true, false);
    startTimerHz (30);
}
MatterView::~MatterView()
{
    stopTimer();
    if (gestureX != nullptr) gestureX->endChangeGesture();
    if (gestureY != nullptr) gestureY->endChangeGesture();
}
void MatterView::setXY (juce::Point<float> point)
{
    const auto plot = getLocalBounds().toFloat().reduced (20.0f, 20.0f)
                         .withTrimmedTop (104.0f).withTrimmedBottom (80.0f);
    const float x = juce::jlimit (0.0f, 1.0f, (point.x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
    const float y = juce::jlimit (0.0f, 1.0f, 1.0f - (point.y - plot.getY()) / juce::jmax (1.0f, plot.getHeight()));
    if (gestureX != nullptr) gestureX->setValueNotifyingHost (x);
    if (gestureY != nullptr) gestureY->setValueNotifyingHost (y);
}
void MatterView::mouseDown (const juce::MouseEvent& e)
{
    const char* xId = layer == 0 ? "aShape03" : "bShape03";
    const char* yId = layer == 0 ? "aShape08" : "bShape08";
    gestureX = audio.parameters.getParameter (xId);
    gestureY = audio.parameters.getParameter (yId);
    if (gestureX != nullptr) gestureX->beginChangeGesture();
    if (gestureY != nullptr) gestureY->beginChangeGesture();
    setXY (e.position);
}
void MatterView::mouseDrag (const juce::MouseEvent& e) { setXY (e.position); }
void MatterView::mouseUp (const juce::MouseEvent&)
{
    if (gestureX != nullptr) gestureX->endChangeGesture();
    if (gestureY != nullptr) gestureY->endChangeGesture();
    gestureX = gestureY = nullptr;
}
void MatterView::paint (juce::Graphics& g)
{
    const auto rect = getLocalBounds().toFloat();
    g.setColour (ui::surface); g.fillRoundedRectangle (rect.reduced (0.5f), 8.0f);
    g.setColour (ui::border); g.drawRoundedRectangle (rect.reduced (0.6f), 8.0f, 1.0f);

    const int id = juce::jlimit (0, 5, static_cast<int> (vParam (audio, layer == 0 ? "materialA" : "materialB")));
    const int other = juce::jlimit (0, 5, static_cast<int> (vParam (audio, layer == 0 ? "materialB" : "materialA")));
    const auto accent = ui::accentFor (id);
    const float energy = juce::jlimit (0.0f, 1.0f, audio.outputRms.load (std::memory_order_relaxed) * 3.8f);
    const float peak = juce::jlimit (0.0f, 1.0f, audio.outputPeak.load (std::memory_order_relaxed));
    const float movement = vParam (audio, layer == 0 ? "aMotion" : "bMotion");
    const float xControl = vParam (audio, layer == 0 ? "aShape03" : "bShape03");
    const float yControl = vParam (audio, layer == 0 ? "aShape08" : "bShape08");
    clock += 0.012f + movement * 0.018f;
    if (clock > 1000.0f) clock = 0.0f;

    const auto whole = getLocalBounds();
    title (g, "INTERACTIVE SOUND FIELD", whole.withTrimmedLeft (22).withTrimmedTop (14).withHeight (18),
           10.0f, ui::subdued, true);
    title (g, ui::name[static_cast<size_t> (id)], whole.withTrimmedLeft (22).withTrimmedTop (34).withHeight (34),
           26.0f, accent, false);
    title (g, layer == 0 ? "/ A" : "/ B", whole.withTrimmedLeft (191).withTrimmedTop (42).withHeight (21),
           11.0f, ui::subdued, true);
    title (g, "FUSED WITH", whole.withTrimmedRight (23).withTrimmedTop (26).withHeight (17),
           9.0f, ui::subdued, true, juce::Justification::centredRight);
    title (g, ui::name[static_cast<size_t> (other)], whole.withTrimmedRight (23).withTrimmedTop (45).withHeight (20),
           11.0f, ui::accentFor (other), true, juce::Justification::centredRight);
    g.setColour (ui::border); g.drawLine (20.0f, 84.0f, rect.getRight() - 20.0f, 84.0f, 1.0f);
    title (g, "MATERIAL SURFACE", whole.withTrimmedLeft (24).withTrimmedTop (89).withHeight (24),
           10.0f, ui::subdued);
    title (g, "LIVE DSP / AUDIO-REACTIVE", whole.withTrimmedRight (24).withTrimmedTop (89).withHeight (24),
           10.0f, ui::secondary, false, juce::Justification::centredRight);

    const float top = 125.0f, bottom = rect.getHeight() - 89.0f;
    const float plotH = juce::jmax (20.0f, bottom - top);
    const float cx = rect.getCentreX() + (xControl - 0.5f) * rect.getWidth() * 0.13f;
    const float cy = top + plotH * 0.5f - (yControl - 0.5f) * plotH * 0.16f;
    const float radius = juce::jmin (rect.getWidth() * 0.235f, plotH * 0.33f) * (0.9f + energy * 0.18f);
    const float wobble = 0.045f + 0.13f * energy;
    g.setColour (ui::line.withAlpha (0.8f));
    for (int n = -4; n <= 4; ++n)
    {
        const float x = cx + n * rect.getWidth() * 0.08f;
        g.drawLine (x, top, x, bottom, 0.7f);
    }
    for (int n = -2; n <= 2; ++n)
    {
        const float y = cy + n * plotH * 0.17f;
        g.drawLine (28.0f, y, rect.getWidth() - 28.0f, y, 0.7f);
    }
    g.setColour (accent.withAlpha (0.18f));
    g.drawEllipse (cx - radius * 1.1f, cy - radius * 1.1f, radius * 2.2f, radius * 2.2f, 1.0f);

    if (id == 0) // LIQUID: nested fluid contour waves.
    {
        for (int ring = 0; ring < 13; ++ring)
        {
            const float scale = 0.45f + 0.045f * ring;
            juce::Path wave;
            for (int j = 0; j <= 180; ++j)
            {
                const float a = tau * j / 180.0f;
                const float mod = 1.0f + wobble * std::sin (a * (3.0f + ring % 2) + clock * 1.5f - ring * 0.19f)
                                      + 0.038f * std::cos (a * 7.0f - clock + ring);
                const float x = cx + std::cos (a) * radius * scale * mod * 1.48f;
                const float y = cy + std::sin (a) * radius * scale * mod;
                if (j == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
            }
            g.setColour (accent.withAlpha (0.12f + ring * 0.027f));
            g.strokePath (wave, juce::PathStrokeType (0.8f));
        }
    }
    else if (id == 1) // CRYSTAL: translucent facets and harmonic axes.
    {
        juce::Path outer, inner;
        polygon (outer, cx, cy, radius, 6, pi * 0.5f, 1.12f);
        polygon (inner, cx, cy, radius * 0.52f, 6, pi * 0.5f + clock * 0.018f, 0.90f);
        g.setColour (accent.withAlpha (0.13f + energy * 0.06f)); g.fillPath (outer);
        g.setColour (accent.withAlpha (0.80f)); g.strokePath (outer, juce::PathStrokeType (1.7f));
        g.setColour (accent.withAlpha (0.50f)); g.strokePath (inner, juce::PathStrokeType (1.1f));
        for (int k = 0; k < 6; ++k)
        {
            const float a = pi * 0.5f + k * tau / 6.0f;
            g.setColour (accent.withAlpha (0.28f));
            g.drawLine (cx + radius * std::cos (a) * 1.12f, cy + radius * std::sin (a),
                        cx + radius * 0.52f * std::cos (a + clock * 0.018f) * 0.90f,
                        cy + radius * 0.52f * std::sin (a + clock * 0.018f), 0.8f);
        }
    }
    else if (id == 2) // PLASMA: electric filaments and irregular discharge.
    {
        for (int ring = 0; ring < 7; ++ring)
        {
            juce::Path arcs;
            for (int j = 0; j <= 128; ++j)
            {
                const float a = tau * j / 128.0f;
                const float fract = 1.0f + (wobble + ring * 0.007f) *
                      std::sin (a * 9.0f + clock * 3.0f + ring) +
                      0.10f * std::cos (a * 17.0f - clock * 0.8f);
                const float r = radius * (0.60f + ring * 0.045f) * fract;
                const float px = cx + r * std::cos (a) * 1.35f, py = cy + r * std::sin (a);
                if (j == 0) arcs.startNewSubPath (px, py); else arcs.lineTo (px, py);
            }
            g.setColour (accent.withAlpha (0.18f + ring * 0.04f));
            g.strokePath (arcs, juce::PathStrokeType (1.0f));
        }
    }
    else if (id == 3) // VAPOR: coherent diffusion + grains.
    {
        for (int ring = 0; ring < 5; ++ring)
        {
            juce::Path cloud;
            for (int j = 0; j <= 112; ++j)
            {
                const float a = tau * j / 112.0f;
                const float r = radius * (0.42f + ring * 0.14f) *
                   (1.0f + 0.09f * std::sin (a * 4.0f + clock * 0.50f + ring));
                const float x = cx + std::cos (a) * r * 1.45f;
                const float y = cy + std::sin (a) * r;
                if (j == 0) cloud.startNewSubPath (x, y); else cloud.lineTo (x, y);
            }
            cloud.closeSubPath();
            g.setColour (accent.withAlpha (0.018f + 0.012f * (5 - ring)));
            g.fillPath (cloud);
            g.setColour (accent.withAlpha (0.10f + 0.035f * ring));
            g.strokePath (cloud, juce::PathStrokeType (0.8f));
        }
    }
    else if (id == 4) // METAL: rigid resonant polyhedron and orbital frequency.
    {
        juce::Path outer, core;
        polygon (outer, cx, cy, radius, 5, -pi * 0.5f + clock * 0.012f, 1.55f);
        polygon (core, cx, cy, radius * 0.58f, 5, -pi * 0.5f - clock * 0.005f, 0.92f);
        g.setColour (accent.withAlpha (0.12f)); g.fillPath (outer);
        g.setColour (accent.withAlpha (0.85f)); g.strokePath (outer, juce::PathStrokeType (1.6f));
        g.setColour (accent.withAlpha (0.47f)); g.strokePath (core, juce::PathStrokeType (1.0f));
        g.setColour (accent.withAlpha (0.30f));
        for (int k = 0; k < 5; ++k)
        {
            const float a = -pi * 0.5f + k * tau / 5.0f;
            g.drawLine (cx, cy, cx + radius * std::cos (a) * 1.55f,
                        cy + radius * std::sin (a), 0.8f);
        }
        g.drawEllipse (cx - radius * 0.79f, cy - radius * 1.14f,
                       radius * 1.58f, radius * 2.28f, 0.9f);
    }
    else // LIGHT: prism, rays and luminous facets.
    {
        for (int k = 0; k < 20; ++k)
        {
            const float a = -pi * 0.5f + k * tau / 20.0f;
            g.setColour (accent.withAlpha (0.07f + 0.08f * (k % 3 == 0)));
            g.drawLine (cx + std::cos (a) * radius * 0.39f, cy + std::sin (a) * radius * 0.39f,
                        cx + std::cos (a) * radius * 1.38f, cy + std::sin (a) * radius * 1.38f, 0.75f);
        }
        juce::Path prism;
        polygon (prism, cx, cy, radius * 0.88f, 5, -pi * 0.5f + clock * 0.008f, 1.2f);
        g.setColour (accent.withAlpha (0.17f)); g.fillPath (prism);
        g.setColour (accent.withAlpha (0.9f)); g.strokePath (prism, juce::PathStrokeType (1.65f));
    }

    // Deterministic energy-reactive particles in PLASMA, VAPOR and LIGHT.
    if (id == 2 || id == 3 || id == 5)
    {
        for (int k = 0; k < 58; ++k)
        {
            const float a = static_cast<float> (k) * 2.39996f + clock * (id == 2 ? 0.24f : 0.04f);
            const float rr = radius * (0.3f + 1.18f * static_cast<float> ((k * 19) % 53) / 53.0f);
            const float px = cx + std::cos (a) * rr * 1.45f;
            const float py = cy + std::sin (a) * rr;
            const float diameter = 1.2f + 1.8f * (k % 3 == 0 ? 1.0f : 0.2f) + peak * 1.7f;
            g.setColour (accent.withAlpha (0.13f + 0.29f * (k % 4 == 0)));
            g.fillEllipse (px, py, diameter, diameter);
        }
    }

    g.setColour (accent.withAlpha (0.78f));
    g.drawEllipse (cx - 10.0f, cy - 10.0f, 20.0f, 20.0f, 1.0f);
    g.drawLine (cx - 5.0f, cy, cx + 5.0f, cy, 1.0f);
    g.drawLine (cx, cy - 5.0f, cx, cy + 5.0f, 1.0f);
    title (g, "DRAG TO SHAPE SOUND  /  X: " +
               juce::String (ui::controls[static_cast<size_t> (id)][2]) +
               "  /  Y: " + ui::controls[static_cast<size_t> (id)][7],
           whole.withTrimmedBottom (82).withTrimmedTop (whole.getHeight() - 119),
           9.0f, ui::subdued, false, juce::Justification::centred);

    const float meterY = rect.getBottom() - 73.0f;
    g.setColour (ui::border); g.drawLine (20.0f, meterY, rect.getRight() - 20.0f, meterY, 1.0f);
    const auto toDb = [] (float v) { return juce::Decibels::gainToDecibels (juce::jmax (1.0e-7f, v)); };
    const auto input = toDb (audio.inputRms.load (std::memory_order_relaxed));
    const auto output = toDb (audio.outputRms.load (std::memory_order_relaxed));
    const auto outPeak = toDb (audio.outputPeak.load (std::memory_order_relaxed));
    const int colW = juce::jmax (80, (getWidth() - 40) / 3);
    const juce::String metric[] = {
        "INPUT RMS", "OUTPUT RMS", "PEAK OUT"
    };
    const float vals[] = { input, output, outPeak };
    for (int i = 0; i < 3; ++i)
    {
        const int x = 23 + i * colW;
        title (g, metric[i], { x, static_cast<int> (meterY) + 9, colW - 12, 15 },
               9.0f, ui::subdued, true);
        title (g, juce::String (vals[i], 1) + " dBFS",
               { x, static_cast<int> (meterY) + 27, colW - 12, 22 },
               15.0f, ui::text);
    }
}

void OrganixAudioProcessorEditor::drawText (juce::Graphics& g, const juce::String& value,
                                              juce::Rectangle<int> bounds, float size,
                                              juce::Colour colour, bool bold,
                                              juce::Justification align)
{
    title (g, value, bounds, size, colour, bold, align);
}
void OrganixAudioProcessorEditor::setupDial (Dial& dial, const juce::String& id,
                                                const juce::String& label, juce::Colour accent,
                                                bool percent)
{
    dial.percent = percent;
    dial.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    dial.slider.setRotaryParameters (pi * 1.25f, pi * 2.75f, true);
    dial.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    dial.slider.setDoubleClickReturnValue (true, percent ? 0.5 : 0.0);
    dial.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    dial.slider.setColour (juce::Slider::rotarySliderOutlineColourId, ui::border);
    dial.slider.setMouseDragSensitivity (180);

    dial.label.setText (label, juce::dontSendNotification);
    dial.label.setColour (juce::Label::textColourId, ui::secondary);
    dial.label.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::plain)));
    dial.label.setJustificationType (juce::Justification::centred);
    dial.label.setInterceptsMouseClicks (false, false);

    dial.value.setJustificationType (juce::Justification::centred);
    dial.value.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    dial.value.setColour (juce::Label::textColourId, accent);
    dial.value.setColour (juce::Label::backgroundColourId, ui::panel);
    dial.value.setEditable (true, true, false);
    dial.value.onTextChange = [&dial]
    {
        double value = dial.value.getText().getDoubleValue();
        if (dial.percent) value /= 100.0;
        dial.slider.setValue (juce::jlimit (dial.slider.getMinimum(),
                                          dial.slider.getMaximum(), value),
                              juce::sendNotificationSync);
    };
    dial.slider.onValueChange = [&dial]
    {
        const auto value = dial.slider.getValue();
        const auto text = dial.percent
            ? juce::String (juce::roundToInt (value * 100.0)) + "%"
            : juce::String (value, 1) + " dB";
        dial.value.setText (text, juce::dontSendNotification);
    };

    addAndMakeVisible (dial.slider);
    addAndMakeVisible (dial.label);
    addAndMakeVisible (dial.value);
    if (id.isNotEmpty())
        dial.attachment = std::make_unique<SliderAttachment> (audio.parameters, id, dial.slider);
    dial.slider.onValueChange();
}
void OrganixAudioProcessorEditor::layoutDial (Dial& dial, juce::Rectangle<int> region)
{
    if (region.isEmpty())
    {
        dial.slider.setBounds (0, 0, 0, 0);
        dial.label.setBounds (0, 0, 0, 0);
        dial.value.setBounds (0, 0, 0, 0);
        return;
    }

    // Fidelity layout: rotary ring, then name, then numeric value.
    // Values always share the same baseline in a grid row.
    auto content = region.reduced (3, 2);
    dial.value.setBounds (content.removeFromBottom (18));
    dial.label.setBounds (content.removeFromBottom (18));
    const int diameter = juce::jmax (20, juce::jmin ({ 66, content.getWidth() - 8,
                                                      content.getHeight() - 2 }));
    dial.slider.setBounds (content.getCentreX() - diameter / 2,
                           content.getCentreY() - diameter / 2,
                           diameter, diameter);
}
int OrganixAudioProcessorEditor::activeMaterial (int layerIndex) const
{
    const auto* value = audio.parameters.getRawParameterValue (layerIndex == 0 ? "materialA" : "materialB");
    return value == nullptr ? 0 : juce::jlimit (0, 5, static_cast<int> (value->load (std::memory_order_relaxed)));
}
void OrganixAudioProcessorEditor::bindProperties()
{
    const auto id = activeMaterial (selectedLayer);
    for (int i = 0; i < 12; ++i)
    {
        auto& dial = properties[static_cast<size_t> (i)];
        dial.attachment.reset();
        const auto suffix = juce::String (i + 1).paddedLeft ('0', 2);
        const auto parameterId = juce::String (selectedLayer == 0 ? "aShape" : "bShape") + suffix;
        dial.label.setText (ui::controls[static_cast<size_t> (id)][static_cast<size_t> (i)], juce::dontSendNotification);
        dial.slider.setColour (juce::Slider::rotarySliderFillColourId, ui::accentFor (id));
        dial.slider.setColour (juce::Slider::textBoxTextColourId, ui::accentFor (id));
        dial.attachment = std::make_unique<SliderAttachment> (audio.parameters, parameterId, dial.slider);
    }
}
void OrganixAudioProcessorEditor::updateMaterialState()
{
    const int a = activeMaterial (0), b = activeMaterial (1);
    const int selected = activeMaterial (selectedLayer);
    const bool enabled = vParam (audio, "enableB") > 0.5f;
    for (int i = 0; i < 6; ++i)
    {
        auto& button = materialButtons[static_cast<size_t> (i)];
        const auto accent = ui::accentFor (i);
        const bool isSelected = i == selected;
        const auto number = juce::String (i + 1).paddedLeft ('0', 2);
        juce::String badges;
        if (i == a) badges += "  A";
        if (enabled && i == b) badges += "  B";
        button.setButtonText (number + "   " + ui::name[static_cast<size_t> (i)] + badges);
        button.setColour (juce::TextButton::buttonColourId, isSelected ? ui::inset : ui::surface);
        button.setColour (juce::TextButton::textColourOffId, isSelected ? accent : ui::secondary);
    }
    tabA.setColour (juce::TextButton::buttonColourId, selectedLayer == 0 ? ui::inset : ui::surface);
    tabB.setColour (juce::TextButton::buttonColourId, selectedLayer == 1 ? ui::inset : ui::surface);
    tabA.setColour (juce::TextButton::textColourOffId, selectedLayer == 0 ? ui::accentFor (a) : ui::secondary);
    tabB.setColour (juce::TextButton::textColourOffId, selectedLayer == 1 ? ui::accentFor (b) : ui::secondary);
    enableB.setButtonText (enabled ? "B ON" : "B OFF");
    enableB.setColour (juce::TextButton::textColourOffId, enabled ? ui::engaged : ui::secondary);
    const bool bypassed = vParam (audio, "bypass") > 0.5f;
    engaged.setButtonText (bypassed ? "BYPASSED" : "ENGAGED");
    engaged.setColour (juce::TextButton::buttonColourId, bypassed ? ui::surface : juce::Colour (0xff11352f));
    engaged.setColour (juce::TextButton::textColourOffId, bypassed ? ui::secondary : ui::engaged);
    for (auto& dial : properties)
    {
        dial.slider.setColour (juce::Slider::rotarySliderFillColourId, ui::accentFor (selected));
        dial.slider.setColour (juce::Slider::textBoxTextColourId, ui::accentFor (selected));
    }
    for (int i = 0; i < 12; ++i)
        properties[static_cast<size_t> (i)].label.setText (
            ui::controls[static_cast<size_t> (selected)][static_cast<size_t> (i)], juce::dontSendNotification);
    visualizer.setLayer (selectedLayer);
    repaint();
}
void OrganixAudioProcessorEditor::selectLayer (int index)
{
    if (selectedLayer != index)
    {
        selectedLayer = juce::jlimit (0, 1, index);
        bindProperties();
    }
    updateMaterialState();
}
void OrganixAudioProcessorEditor::selectMaterial (int index)
{
    const auto* key = selectedLayer == 0 ? "materialA" : "materialB";
    if (auto* p = audio.parameters.getParameter (key))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (juce::jlimit (0, 5, index))));
        p->endChangeGesture();
    }
    updateMaterialState();
}
void OrganixAudioProcessorEditor::setAudioToggle (const char* id, bool enabled)
{
    if (auto* p = audio.parameters.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (enabled ? 1.0f : 0.0f));
        p->endChangeGesture();
    }
    updateMaterialState();
}
void OrganixAudioProcessorEditor::refreshPreset()
{
    const int index = audio.currentPreset.load (std::memory_order_relaxed);
    if (index >= 0 && index < 100 && presets.getSelectedId() != index + 1)
        presets.setSelectedId (index + 1, juce::dontSendNotification);
}

OrganixAudioProcessorEditor::OrganixAudioProcessorEditor (OrganixAudioProcessor& p)
    : AudioProcessorEditor (&p), audio (p), visualizer (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (visualizer);

    for (int i = 0; i < 6; ++i)
    {
        auto& button = materialButtons[static_cast<size_t> (i)];
        button.setColour (juce::TextButton::buttonColourId, ui::surface);
        button.onClick = [this, i] { selectMaterial (i); };
        addAndMakeVisible (button);
    }
    for (int i = 0; i < 100; ++i)
    {
        if (i % 10 == 0)
            presets.addSectionHeading (organix::categoryName (
                organix::factoryPresets[static_cast<size_t> (i)].category));
        presets.addItem (organix::factoryPresets[static_cast<size_t> (i)].name, i + 1);
    }
    presets.onChange = [this] {
        if (presets.getSelectedId() > 0)
        {
            audio.loadFactoryPreset (presets.getSelectedId() - 1);
            updateMaterialState();
        }
    };
    previous.onClick = [this] {
        const auto idx = audio.currentPreset.load (std::memory_order_relaxed);
        presets.setSelectedId (1 + (idx + 99) % 100, juce::sendNotificationSync);
    };
    next.onClick = [this] {
        const auto idx = audio.currentPreset.load (std::memory_order_relaxed);
        presets.setSelectedId (1 + (idx + 1) % 100, juce::sendNotificationSync);
    };
    addAndMakeVisible (presets);
    addAndMakeVisible (previous);
    addAndMakeVisible (next);

    const juce::StringArray routes { "PARALLEL", "SERIAL A > B", "SERIAL B > A", "CROSS-FUSION" };
    for (int i = 0; i < routes.size(); ++i) routing.addItem (routes[i], i + 1);
    routingAttachment = std::make_unique<ComboAttachment> (audio.parameters, "routing", routing);
    addAndMakeVisible (routing);

    engaged.onClick = [this] { setAudioToggle ("bypass", vParam (audio, "bypass") < 0.5f); };
    tabA.onClick = [this] { selectLayer (0); };
    tabB.onClick = [this] { selectLayer (1); };
    enableB.onClick = [this] { setAudioToggle ("enableB", vParam (audio, "enableB") < 0.5f); };
    macroToggle.onClick = [this] { macroPage = ! macroPage; resized(); repaint(); };
    for (auto* c : { &engaged, &tabA, &tabB, &enableB, &macroToggle })
        addAndMakeVisible (*c);

    for (int i = 0; i < 12; ++i)
        setupDial (properties[static_cast<size_t> (i)], {}, ui::controls[0][static_cast<size_t> (i)], ui::accentFor (0));

    constexpr const char* bottomIds[] = { "fusion", "balance", "drywet",
                                          "macroMotion", "macroEvolution", "input", "output" };
    constexpr const char* bottomNames[] = { "FUSION", "BALANCE", "DRY / WET",
                                            "MOTION", "EVOLUTION", "INPUT", "OUTPUT" };
    for (int i = 0; i < 7; ++i)
        setupDial (bottom[static_cast<size_t> (i)], bottomIds[i], bottomNames[i],
                   i < 3 ? ui::accentFor (1) : i < 5 ? ui::accentFor (3) : ui::accentFor (0), i < 5);
    constexpr const char* macroIds[] = { "macroTransform", "macroMotion", "macroEnergy", "macroDensity",
                                         "macroSpace", "macroHarmonics", "macroEvolution", "macroFusion" };
    constexpr const char* macroNames[] = { "TRANSFORM", "MOTION", "ENERGY", "DENSITY",
                                           "SPACE", "HARMONICS", "EVOLUTION", "FUSION" };
    for (int i = 0; i < 8; ++i)
        setupDial (macros[static_cast<size_t> (i)], macroIds[i], macroNames[i],
                   i % 2 == 0 ? ui::accentFor (0) : ui::accentFor (3));
    bindProperties();
    refreshPreset();
    updateMaterialState();
    setResizable (true, true);
    setResizeLimits (1240, 820, 1940, 1220);
    setSize (1504, 1008);
    startTimerHz (12);
}
OrganixAudioProcessorEditor::~OrganixAudioProcessorEditor()
{
    stopTimer();
    for (auto& dial : properties) dial.attachment.reset();
    for (auto& dial : macros) dial.attachment.reset();
    for (auto& dial : bottom) dial.attachment.reset();
    routingAttachment.reset();
    setLookAndFeel (nullptr);
}
void OrganixAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::background);
    const auto margin = getLocalBounds().reduced (16);
    title (g, "LIBERTY MUSIC SOFTWARE", { headerBounds.getX() + 59, headerBounds.getY() + 13, 280, 19 },
           10.0f, ui::secondary, true);
    title (g, "ORGANIX", { headerBounds.getX() + 59, headerBounds.getY() + 29, 290, 40 },
           33.0f, ui::text, true);
    title (g, "LIVING AUDIO MATTER", { headerBounds.getX() + 59, headerBounds.getY() + 69, 250, 18 },
           10.0f, ui::subdued, true);
    g.setColour (ui::accentFor (0).withAlpha (0.9f));
    juce::Path emblem;
    polygon (emblem, static_cast<float> (headerBounds.getX() + 31),
             static_cast<float> (headerBounds.getY() + 52), 21.0f, 6, pi * 0.5f, 1.0f);
    g.strokePath (emblem, juce::PathStrokeType (1.6f));
    for (int i = 0; i < 3; ++i)
        g.drawLine (static_cast<float> (headerBounds.getX() + 19),
                    static_cast<float> (headerBounds.getY() + 45 + i * 7),
                    static_cast<float> (headerBounds.getX() + 43),
                    static_cast<float> (headerBounds.getY() + 45 + i * 7), 1.1f);
    title (g, "TEXTURE LIBRARY  /  100 FACTORY STARTING POINTS",
           { presets.getX(), presets.getBottom() + 3, presets.getWidth(), 17 },
           9.0f, ui::subdued, false);
    g.setColour (ui::line); g.drawLine (static_cast<float> (margin.getX()),
                                       static_cast<float> (headerBounds.getBottom() + 2),
                                       static_cast<float> (margin.getRight()),
                                       static_cast<float> (headerBounds.getBottom() + 2), 1.0f);
    ui::drawPanel (g, leftBounds);
    ui::drawPanel (g, rightBounds);
    if (macroPage) ui::drawPanel (g, footerAll);
    else
    {
        ui::drawPanel (g, footerFusion);
        ui::drawPanel (g, footerMotion);
        ui::drawPanel (g, footerOutput);
    }
    title (g, "MATERIAL ENGINES", { leftBounds.getX() + 14, leftBounds.getY() + 15, leftBounds.getWidth() - 27, 21 },
           10.0f, ui::secondary, true);
    title (g, "EDITING LAYER " + juce::String (selectedLayer == 0 ? "A" : "B"),
           { leftBounds.getX() + 14, leftBounds.getY() + 41, leftBounds.getWidth() - 28, 20 },
           10.0f, ui::subdued);
    const int current = activeMaterial (selectedLayer);
    for (int i = 0; i < 6; ++i)
    {
        if (i != current) continue;
        const auto b = materialButtons[static_cast<size_t> (i)].getBounds();
        g.setColour (ui::accentFor (i));
        g.fillRoundedRectangle (juce::Rectangle<float> (static_cast<float> (b.getX()),
                          static_cast<float> (b.getY() + 8), 3.0f,
                          static_cast<float> (b.getHeight() - 16)), 1.3f);
    }
    g.setColour (ui::engaged); g.fillEllipse (static_cast<float> (leftBounds.getX() + 16),
                              static_cast<float> (leftBounds.getBottom() - 37), 4.0f, 4.0f);
    title (g, "SIX LIVE PROCESSORS",
           { leftBounds.getX() + 27, leftBounds.getBottom() - 43, leftBounds.getWidth() - 35, 20 },
           9.0f, ui::subdued, true);

    title (g, "MATERIAL PROPERTIES", { rightBounds.getX() + 19, rightBounds.getY() + 61,
                                      rightBounds.getWidth() - 38, 18 },
           10.0f, ui::secondary, true);
    title (g, ui::name[static_cast<size_t> (current)],
           { rightBounds.getX() + 19, rightBounds.getY() + 81, rightBounds.getWidth() - 38, 35 },
           26.0f, ui::accentFor (current));
    title (g, ui::description[static_cast<size_t> (current)],
           { rightBounds.getX() + 19, rightBounds.getY() + 117, rightBounds.getWidth() - 38, 20 },
           11.0f, ui::secondary);
    title (g, "SOURCE-DRIVEN  /  12 LIVE PARAMETERS",
           { rightBounds.getX() + 18, rightBounds.getBottom() - 27, rightBounds.getWidth() - 36, 19 },
           9.0f, ui::subdued, true, juce::Justification::centred);
    g.setColour (ui::line);
    g.drawLine (static_cast<float> (rightBounds.getX() + 17), static_cast<float> (rightBounds.getBottom() - 33),
                static_cast<float> (rightBounds.getRight() - 17), static_cast<float> (rightBounds.getBottom() - 33), 1.0f);

    if (macroPage)
        title (g, "EIGHT PERFORMANCE MACROS", { footerAll.getX() + 17, footerAll.getY() + 11,
                                                footerAll.getWidth() - 34, 24 }, 11.0f, ui::secondary, true);
    else
    {
        title (g, "DUAL FUSION", { footerFusion.getX() + 17, footerFusion.getY() + 13, 210, 20 },
               11.0f, ui::secondary, true);
        title (g, "MOVEMENT", { footerMotion.getX() + 17, footerMotion.getY() + 13, 200, 20 },
               11.0f, ui::secondary, true);
        title (g, "SIGNAL & OUTPUT", { footerOutput.getX() + 17, footerOutput.getY() + 13, 200, 20 },
               11.0f, ui::secondary, true);
        const auto da = activeMaterial (0), db = activeMaterial (1);
        title (g, "A / " + juce::String (ui::name[static_cast<size_t> (da)]) +
                  "     +     B / " + ui::name[static_cast<size_t> (db)],
               { footerFusion.getX() + 18, footerFusion.getY() + 36, footerFusion.getWidth() - 36, 18 },
               10.0f, ui::secondary);
    }
}
void OrganixAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    headerBounds = area.removeFromTop (94);
    area.removeFromTop (14);
    const int footH = juce::jlimit (160, 190, juce::roundToInt (getHeight() * 0.19f));
    footerAll = area.removeFromBottom (footH);
    area.removeFromBottom (14);
    const int leftW = juce::jlimit (165, 194, juce::roundToInt (getWidth() * 0.116f));
    const int rightW = juce::jlimit (350, 424, juce::roundToInt (getWidth() * 0.277f));
    leftBounds = area.removeFromLeft (leftW);
    area.removeFromLeft (14);
    rightBounds = area.removeFromRight (rightW);
    area.removeFromRight (14);
    centerBounds = area;
    visualizer.setBounds (centerBounds);

    auto headActions = headerBounds;
    macroToggle.setBounds (headActions.removeFromRight (85).reduced (4, 25));
    engaged.setBounds (headActions.removeFromRight (120).reduced (5, 25));
    headActions.removeFromRight (10);
    const int presetWidth = juce::jmin (390, juce::jmax (210, headActions.getWidth() - 310));
    auto controlsRect = headActions.removeFromRight (presetWidth);
    next.setBounds (controlsRect.removeFromRight (35).reduced (2, 25));
    previous.setBounds (controlsRect.removeFromLeft (35).reduced (2, 25));
    presets.setBounds (controlsRect.reduced (3, 25));

    const auto leftPad = leftBounds.reduced (12, 8);
    const int buttonsY = leftBounds.getY() + 72;
    const int available = juce::jmax (270, leftBounds.getBottom() - buttonsY - 50);
    const int buttonH = juce::jmax (38, (available - 5 * 7) / 6);
    for (int i = 0; i < 6; ++i)
        materialButtons[static_cast<size_t> (i)].setBounds (
            leftPad.getX(), buttonsY + i * (buttonH + 7), leftPad.getWidth(), buttonH);

    const int topY = rightBounds.getY() + 12;
    const int tabW = (rightBounds.getWidth() - 47) / 2;
    tabA.setBounds (rightBounds.getX() + 15, topY, tabW, 32);
    tabB.setBounds (rightBounds.getX() + 21 + tabW, topY, tabW, 32);
    enableB.setBounds (rightBounds.getRight() - 74, rightBounds.getY() + 62, 57, 20);

    auto grid = rightBounds.reduced (15, 8).withTrimmedTop (139).withTrimmedBottom (31);
    for (int row = 0; row < 4; ++row)
    {
        auto nextRow = grid.removeFromTop (grid.getHeight() / (4 - row));
        for (int col = 0; col < 3; ++col)
        {
            const auto cell = nextRow.removeFromLeft (nextRow.getWidth() / (3 - col));
            layoutDial (properties[static_cast<size_t> (row * 3 + col)], cell);
        }
    }

    if (macroPage)
    {
        const int stepW = (footerAll.getWidth() - 42) / 8;
        for (int i = 0; i < 8; ++i)
        {
            auto& d = macros[static_cast<size_t> (i)];
            d.slider.setVisible (true);
            d.label.setVisible (true);
            layoutDial (d, { footerAll.getX() + 20 + i * stepW, footerAll.getY() + 39,
                             stepW, footerAll.getHeight() - 48 });
        }
        for (auto& d : bottom) { d.slider.setVisible (false); d.label.setVisible (false); }
        routing.setVisible (false);
    }
    else
    {
        for (auto& d : macros) { d.slider.setVisible (false); d.label.setVisible (false); }
        const int w = footerAll.getWidth();
        const int gap = 14;
        const int fusionW = juce::roundToInt (w * 0.345f);
        const int movementW = juce::roundToInt (w * 0.285f);
        footerFusion = { footerAll.getX(), footerAll.getY(), fusionW, footerAll.getHeight() };
        footerMotion = { footerFusion.getRight() + gap, footerAll.getY(), movementW - gap, footerAll.getHeight() };
        footerOutput = { footerMotion.getRight() + gap, footerAll.getY(),
                         footerAll.getRight() - (footerMotion.getRight() + gap), footerAll.getHeight() };
        routing.setVisible (true);
        routing.setBounds (footerFusion.getRight() - 174, footerFusion.getY() + 10, 158, 28);
        for (int i = 0; i < 7; ++i)
        {
            auto& d = bottom[static_cast<size_t> (i)];
            d.slider.setVisible (true);
            d.label.setVisible (true);
            const auto region = i < 3 ? footerFusion : i < 5 ? footerMotion : footerOutput;
            const int count = i < 3 ? 3 : 2;
            const int offset = i < 3 ? i : i < 5 ? i - 3 : i - 5;
            const int cellW = (region.getWidth() - 24) / count;
            layoutDial (d, { region.getX() + 12 + offset * cellW,
                             region.getY() + 57, cellW, region.getHeight() - 62 });
        }
    }
}
void OrganixAudioProcessorEditor::timerCallback()
{
    refreshPreset();
    const int a = activeMaterial (0), b = activeMaterial (1);
    const int signature = a + 6 * b + 36 * selectedLayer +
                          (vParam (audio, "enableB") > 0.5f ? 72 : 0) +
                          (vParam (audio, "bypass") > 0.5f ? 144 : 0);
    if (signature != lastPreset)
    {
        lastPreset = signature;
        updateMaterialState();
    }
}
