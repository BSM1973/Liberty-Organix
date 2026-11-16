#include "PluginEditor.h"
#include "OrganixUILayout.h"
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
void MatterView::timerCallback()
{
    // All animation work stays on the GUI message thread. No buffer or
    // allocation is introduced in processBlock().
    if (! isShowing()) return;
    const int id = juce::jlimit (0, 5, static_cast<int> (
        vParam (audio, layer == 0 ? "materialA" : "materialB")));
    if (priorMaterial != id)
    {
        priorMaterial = id;
        materialReveal = 0.0f;
    }
    const float rms = juce::jlimit (0.0f, 1.0f,
        audio.outputRms.load (std::memory_order_relaxed) * 3.8f);
    const float peak = juce::jlimit (0.0f, 1.0f,
        audio.outputPeak.load (std::memory_order_relaxed));
    visualRms += (rms - visualRms) * (rms > visualRms ? 0.24f : 0.12f);
    visualPeak = juce::jmax (peak, visualPeak * 0.87f);
    const float movement = vParam (audio, layer == 0 ? "aMotion" : "bMotion");
    clock += 0.012f + movement * 0.018f;
    if (clock >= 1000.0f) clock -= 1000.0f;
    materialReveal = juce::jmin (1.0f, materialReveal + 0.10f);
    repaint();
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
    // Header, subtitles and the RMS meter area must not trigger XY automation.
    const auto field = getLocalBounds().toFloat().reduced (20.0f, 20.0f)
                            .withTrimmedTop (104.0f).withTrimmedBottom (80.0f);
    if (! field.contains (e.position)) return;
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
    const float energy = visualRms;
    const float peak = visualPeak;
    const float xControl = vParam (audio, layer == 0 ? "aShape03" : "bShape03");
    const float yControl = vParam (audio, layer == 0 ? "aShape08" : "bShape08");
    const auto whole = getLocalBounds();
    title (g, ui::word (french, "CHAMP SONORE INTERACTIF", "INTERACTIVE SOUND FIELD"), whole.withTrimmedLeft (22).withTrimmedTop (14).withHeight (18),
           10.0f, ui::subdued, true);
    title (g, ui::materialLabel (id, french), whole.withTrimmedLeft (22).withTrimmedTop (34).withHeight (34),
           26.0f, accent, false);
    title (g, layer == 0 ? "/ A" : "/ B", whole.withTrimmedLeft (191).withTrimmedTop (42).withHeight (21),
           11.0f, ui::subdued, true);
    const bool secondActive = vParam (audio, "enableB") > 0.5f;
    title (g, secondActive ? ui::word (french, "FUSION AVEC", "FUSED WITH") :
                                    ui::word (french, "SECONDE COUCHE", "SECOND LAYER"), whole.withTrimmedRight (23).withTrimmedTop (26).withHeight (17),
           9.0f, ui::subdued, true, juce::Justification::centredRight);
    title (g, secondActive ? ui::materialLabel (other, french) :
                          ui::word (french, "DÉSACTIVÉE", "DISENGAGED"),
           whole.withTrimmedRight (23).withTrimmedTop (45).withHeight (20),
           11.0f, secondActive ? ui::accentFor (other) : ui::subdued,
           true, juce::Justification::centredRight);
    g.setColour (ui::border); g.drawLine (20.0f, 84.0f, rect.getRight() - 20.0f, 84.0f, 1.0f);
    title (g, ui::word (french, "SURFACE DU MATÉRIAU", "MATERIAL SURFACE"), whole.withTrimmedLeft (24).withTrimmedTop (89).withHeight (24),
           10.0f, ui::subdued);
    title (g, ui::word (french, "DSP TEMPS RÉEL / RÉACTIF AU SON",
                                   "LIVE DSP / AUDIO-REACTIVE"), whole.withTrimmedRight (24).withTrimmedTop (89).withHeight (24),
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

    // Reveal the new engine over ten frames instead of snapping.
    g.beginTransparencyLayer (materialReveal);
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

    g.endTransparencyLayer();
    g.setColour (accent.withAlpha (0.78f));
    g.drawEllipse (cx - 10.0f, cy - 10.0f, 20.0f, 20.0f, 1.0f);
    g.drawLine (cx - 5.0f, cy, cx + 5.0f, cy, 1.0f);
    g.drawLine (cx, cy - 5.0f, cx, cy + 5.0f, 1.0f);
    title (g, ui::word (french, "GLISSER POUR FAÇONNER LE SON / X : ",
                                   "DRAG TO SHAPE SOUND / X: ") +
               ui::property (id, 2, french) + "  /  Y : " + ui::property (id, 7, french),
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
        ui::word (french, "ENTRÉE RMS", "INPUT RMS"),
        ui::word (french, "SORTIE RMS", "OUTPUT RMS"),
        ui::word (french, "CRÊTE SORTIE", "PEAK OUT")
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
                                                bool percent, int unitMode)
{
    dial.percent = percent;
    dial.unitMode = unitMode >= 0 ? unitMode : (percent ? 0 : 1);
    dial.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    dial.slider.setRotaryParameters (pi * 1.25f, pi * 2.75f, true);
    dial.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    const double resetValue = dial.unitMode == 3 ? 1.0
                            : dial.unitMode == 2 ? 0.25
                            : dial.unitMode == 0 ? 0.5 : 0.0;
    dial.slider.setDoubleClickReturnValue (true, resetValue);
    dial.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    dial.slider.setColour (juce::Slider::rotarySliderOutlineColourId, ui::border);
    dial.slider.setMouseDragSensitivity (180);
    dial.slider.setTooltip (label + " - drag to adjust; double-click to reset");

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
    dial.value.setTooltip ("Click to enter an exact value");
    dial.value.onTextChange = [&dial]
    {
        double value = dial.value.getText().getDoubleValue();
        if (dial.unitMode == 0 || dial.unitMode == 3) value /= 100.0;
        dial.slider.setValue (juce::jlimit (dial.slider.getMinimum(),
                                          dial.slider.getMaximum(), value),
                              juce::sendNotificationSync);
    };
    dial.slider.onValueChange = [&dial]
    {
        const double value = dial.slider.getValue();
        juce::String text;
        switch (dial.unitMode)
        {
            case 0:
            case 3: text = juce::String (juce::roundToInt (value * 100.0)) + "%"; break;
            case 2: text = juce::String (value, 2) + " Hz"; break;
            default: text = juce::String (value, 1) + " dB"; break;
        }
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
    const int diameter = juce::jmax (20, std::min ({ 66, content.getWidth() - 8,
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
        dial.label.setText (ui::property (id, i, french), juce::dontSendNotification);
        dial.slider.setColour (juce::Slider::rotarySliderFillColourId, ui::accentFor (id));
        dial.value.setColour (juce::Label::textColourId, ui::accentFor (id));
        dial.attachment = std::make_unique<SliderAttachment> (audio.parameters, parameterId, dial.slider);
        dial.slider.setTooltip (dial.label.getText() + "  -  " +
                ui::word (french, selectedLayer == 0 ? "Couche A" : "Couche B",
                                  selectedLayer == 0 ? "Layer A" : "Layer B") +
                ui::word (french, "  /  automatisable, double-clic : réinitialiser",
                                  "  /  automatable, double-click: reset"));
        dial.slider.onValueChange();
    }
}
void OrganixAudioProcessorEditor::updateMaterialState()
{
    const int a = activeMaterial (0), b = activeMaterial (1);
    const int selected = activeMaterial (selectedLayer);
    const bool enabled = vParam (audio, "enableB") > 0.5f;
    for (int i = 0; i < 6; ++i)
        materialButtons[static_cast<size_t> (i)].setLayerState (
            i == selected, i == a, enabled && i == b);
    tabA.setColour (juce::TextButton::buttonColourId, selectedLayer == 0 ? ui::inset : ui::surface);
    tabB.setColour (juce::TextButton::buttonColourId, selectedLayer == 1 ? ui::inset : ui::surface);
    tabA.setColour (juce::TextButton::textColourOffId, selectedLayer == 0 ? ui::accentFor (a) : ui::secondary);
    tabB.setColour (juce::TextButton::textColourOffId, selectedLayer == 1 ? ui::accentFor (b) : ui::secondary);
    enableB.setButtonText (enabled ? ui::word (french, "B ACTIF", "B ON")
                                  : ui::word (french, "B INACTIF", "B OFF"));
    enableB.setColour (juce::TextButton::textColourOffId, enabled ? ui::engaged : ui::secondary);
    const bool bypassed = vParam (audio, "bypass") > 0.5f;
    engaged.setButtonText (bypassed ? ui::word (french, "DÉSACTIVÉ", "BYPASSED")
                                  : ui::word (french, "ACTIVÉ", "ENGAGED"));
    engaged.setColour (juce::TextButton::buttonColourId, bypassed ? ui::surface : juce::Colour (0xff11352f));
    engaged.setColour (juce::TextButton::textColourOffId, bypassed ? ui::secondary : ui::engaged);
    for (auto& dial : properties)
    {
        dial.slider.setColour (juce::Slider::rotarySliderFillColourId, ui::accentFor (selected));
        dial.value.setColour (juce::Label::textColourId, ui::accentFor (selected));
    }
    for (int i = 0; i < 12; ++i)
        properties[static_cast<size_t> (i)].label.setText (
            ui::property (selected, i, french), juce::dontSendNotification);
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
    const int idx = juce::jlimit (0, 99, audio.currentPreset.load (std::memory_order_relaxed));
    const auto custom = audio.parameters.state.getProperty ("customPresetName").toString();
    if (custom.isNotEmpty())
        currentPresetName = custom;
    else if (lastFactoryPreset != idx || currentPresetName.isEmpty())
        currentPresetName = audio.getProgramName (idx);
    lastFactoryPreset = idx;
    presetDisplay.setButtonText (currentPresetName.isNotEmpty() ? currentPresetName : audio.getProgramName (idx));
}
void OrganixAudioProcessorEditor::openBrowser()
{
    if (browser == nullptr) return;
    browser->setFrench (french);
    browser->setBounds (getLocalBounds());
    browser->setVisible (true);
    browser->toFront (true);
}
void OrganixAudioProcessorEditor::changeLanguage (bool useFrench)
{
    french = useFrench;
    if (uiSettings != nullptr)
    {
        uiSettings->setValue ("language", french ? "fr" : "en");
        uiSettings->saveIfNeeded();
    }
    visualizer.setFrench (french);
    routeSelector.setFrench (french);
    if (browser != nullptr) browser->setFrench (french);
    languageFR.setColour (juce::TextButton::buttonColourId, french ? ui::inset.brighter (0.22f) : ui::surface);
    languageEN.setColour (juce::TextButton::buttonColourId, french ? ui::surface : ui::inset.brighter (0.22f));
    languageFR.setColour (juce::TextButton::textColourOffId, french ? ui::accentFor (0) : ui::secondary);
    languageEN.setColour (juce::TextButton::textColourOffId, french ? ui::secondary : ui::accentFor (0));
    populateMovementChoices();
    refreshMovementState();
    browserButton.setButtonText (ui::word (french, "BIBLIOTHÈQUE", "BROWSER"));
    macroToggle.setButtonText (macroPage ? ui::word (french, "PRINCIPAL", "MAIN")
                                       : ui::word (french, "MACROS", "MACROS"));
    tabA.setButtonText (ui::word (french, "COUCHE A", "LAYER A"));
    tabB.setButtonText (ui::word (french, "COUCHE B", "LAYER B"));
    constexpr const char* frBottom[] = {
        "FUSION", "ÉQUILIBRE", "SEC / EFFET", "MOUVEMENT", "VITESSE",
        "ENTRÉE", "LARGEUR", "SORTIE"
    };
    constexpr const char* enBottom[] = {
        "FUSION", "BALANCE", "DRY / WET", "MOTION", "RATE",
        "INPUT", "WIDTH", "OUTPUT"
    };
    for (int i = 0; i < 8; ++i)
        bottom[static_cast<size_t> (i)].label.setText (
            juce::String::fromUTF8 (french ? frBottom[i] : enBottom[i]), juce::dontSendNotification);
    constexpr const char* frMacros[] = {
        "TRANSFORMER", "MOUVEMENT", "ÉNERGIE", "DENSITÉ",
        "ESPACE", "HARMONIQUES", "ÉVOLUTION", "FUSION"
    };
    constexpr const char* enMacros[] = {
        "TRANSFORM", "MOTION", "ENERGY", "DENSITY",
        "SPACE", "HARMONICS", "EVOLUTION", "FUSION"
    };
    for (int i = 0; i < 8; ++i)
        macros[static_cast<size_t> (i)].label.setText (
            juce::String::fromUTF8 (french ? frMacros[i] : enMacros[i]), juce::dontSendNotification);
    for (auto& card : materialButtons) card.setFrench (french);
    bindProperties();
    updateMaterialState();
    repaint();
}

void OrganixAudioProcessorEditor::populateMovementChoices()
{
    const int lastTrajectory = trajectorySelect.getSelectedId();
    const int lastDivision = divisionSelect.getSelectedId();
    trajectorySelect.clear (juce::dontSendNotification);
    constexpr const char* frenchModes[] = {"MANUEL", "SINUS", "TRIANGLE", "IMPULSION", "ALEATOIRE"};
    constexpr const char* englishModes[] = {"MANUAL", "SINE", "TRIANGLE", "PULSE", "RANDOM HOLD"};
    for (int i = 0; i < 5; ++i)
        trajectorySelect.addItem (french ? frenchModes[i] : englishModes[i], i + 1);
    divisionSelect.clear (juce::dontSendNotification);
    constexpr const char* beatValues[] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T" };
    for (int i = 0; i < 7; ++i) divisionSelect.addItem (beatValues[i], i + 1);
    trajectorySelect.setSelectedId (lastTrajectory > 0 ? lastTrajectory : 1, juce::dontSendNotification);
    divisionSelect.setSelectedId (lastDivision > 0 ? lastDivision : 3, juce::dontSendNotification);
    trajectoryLabel.setText (ui::word (french, "TRAJECTOIRE", "TRAJECTORY"), juce::dontSendNotification);
    divisionLabel.setText (ui::word (french, "DIVISION RYTHMIQUE", "BEAT DIVISION"), juce::dontSendNotification);
}
void OrganixAudioProcessorEditor::refreshMovementState()
{
    const bool isFree = vParam (audio, "motionFree") > 0.5f;
    motionFreeButton.setButtonText (isFree ? ui::word (french, "LIBRE", "FREE") : "SYNC");
    motionFreeButton.setColour (juce::TextButton::buttonColourId,
                                isFree ? ui::inset : juce::Colour (0xff193d4c));
    motionFreeButton.setColour (juce::TextButton::textColourOffId,
                                isFree ? ui::secondary : ui::accentFor (0));
    // The division still reflects its stored value while FREE is active,
    // but visibly disables until host-tempo mode is selected.
    divisionSelect.setEnabled (! isFree);
    divisionLabel.setColour (juce::Label::textColourId, isFree ? ui::subdued : ui::secondary);
    bottom[4].slider.setEnabled (isFree);
    bottom[4].label.setColour (juce::Label::textColourId, isFree ? ui::secondary : ui::subdued);
    bottom[4].value.setColour (juce::Label::textColourId, isFree ? ui::accentFor (3) : ui::subdued);
}

OrganixAudioProcessorEditor::OrganixAudioProcessorEditor (OrganixAudioProcessor& p)
    : AudioProcessorEditor (&p), audio (p), visualizer (p), routeSelector (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (visualizer);

    for (int i = 0; i < 6; ++i)
    {
        auto& button = materialButtons[static_cast<size_t> (i)];
        button.setMaterial (i);
        button.onClick = [this, i] { selectMaterial (i); };
        addAndMakeVisible (button);
    }
    uiSettings = std::make_unique<juce::PropertiesFile> (ui::preferenceOptions());
    french = uiSettings->getValue ("language", "fr") != "en";
    currentPresetName = audio.getProgramName (audio.currentPreset.load());
    lastFactoryPreset = audio.currentPreset.load();
    presetDisplay.onClick = [this] { openBrowser(); };
    browserButton.onClick = [this] { openBrowser(); };
    previous.onClick = [this]
    {
        audio.loadFactoryPreset ((audio.currentPreset.load() + 99) % 100);
        currentPresetName = audio.getProgramName (audio.currentPreset.load());
        lastFactoryPreset = audio.currentPreset.load();
        refreshPreset();
        updateMaterialState();
    };
    next.onClick = [this]
    {
        audio.loadFactoryPreset ((audio.currentPreset.load() + 1) % 100);
        currentPresetName = audio.getProgramName (audio.currentPreset.load());
        lastFactoryPreset = audio.currentPreset.load();
        refreshPreset();
        updateMaterialState();
    };
    for (auto* b : { &presetDisplay, &browserButton, &previous, &next })
        addAndMakeVisible (*b);
    presetDisplay.setColour (juce::TextButton::buttonColourId, ui::inset);
    presetDisplay.setColour (juce::TextButton::textColourOffId, ui::text);
    browserButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff15373e));
    browserButton.setColour (juce::TextButton::textColourOffId, ui::accentFor (0));

    // No middle dots, emoji, arrows or font-dependent glyphs: FR and EN are
    // painted by native JUCE text buttons using universal Latin letters.
    languageFR.onClick = [this] { changeLanguage (true); };
    languageEN.onClick = [this] { changeLanguage (false); };
    languageFR.setTooltip ("Francais");
    languageEN.setTooltip ("English");
    addAndMakeVisible (languageFR);
    addAndMakeVisible (languageEN);

    for (auto* combo : { &trajectorySelect, &divisionSelect })
    {
        combo->setColour (juce::ComboBox::backgroundColourId, ui::inset);
        combo->setColour (juce::ComboBox::textColourId, ui::text);
        combo->setColour (juce::ComboBox::outlineColourId, ui::border);
        addAndMakeVisible (*combo);
    }
    for (auto* label : { &trajectoryLabel, &divisionLabel })
    {
        label->setFont (juce::Font (juce::FontOptions (10.0f)));
        label->setJustificationType (juce::Justification::centredLeft);
        label->setColour (juce::Label::textColourId, ui::secondary);
        addAndMakeVisible (*label);
    }
    trajectoryAttachment = std::make_unique<ComboAttachment> (
        audio.parameters, "motionTrajectory", trajectorySelect);
    divisionAttachment = std::make_unique<ComboAttachment> (
        audio.parameters, "motionDivision", divisionSelect);
    motionFreeButton.onClick = [this]
    {
        setAudioToggle ("motionFree", vParam (audio, "motionFree") < 0.5f);
        refreshMovementState();
    };
    addAndMakeVisible (motionFreeButton);
    populateMovementChoices();

    addAndMakeVisible (routeSelector);
    browser = std::make_unique<OrganixPresetBrowser> (audio, *uiSettings);
    browser->onApplied = [this] (const juce::String& name)
    {
        currentPresetName = name;
        lastFactoryPreset = audio.currentPreset.load();
        refreshPreset();
        bindProperties();
        updateMaterialState();
        if (browser != nullptr) browser->setVisible (false);
    };
    browser->onDismissed = [this]
    {
        if (browser != nullptr) browser->setVisible (false);
    };
    addAndMakeVisible (*browser);
    browser->setVisible (false);

    engaged.onClick = [this] { setAudioToggle ("bypass", vParam (audio, "bypass") < 0.5f); };
    tabA.onClick = [this] { selectLayer (0); };
    tabB.onClick = [this] { selectLayer (1); };
    enableB.onClick = [this] { setAudioToggle ("enableB", vParam (audio, "enableB") < 0.5f); };
    macroToggle.onClick = [this]
    {
        macroPage = ! macroPage;
        macroToggle.setButtonText (macroPage ? ui::word (french, "PRINCIPAL", "MAIN")
                                            : juce::String ("MACROS"));
        macroToggle.setColour (juce::TextButton::buttonColourId,
                               macroPage ? ui::inset.brighter (0.13f) : ui::surface);
        resized();
        repaint();
    };
    for (auto* c : { &engaged, &tabA, &tabB, &enableB, &macroToggle })
        addAndMakeVisible (*c);

    for (int i = 0; i < 12; ++i)
        setupDial (properties[static_cast<size_t> (i)], {}, ui::controls[0][static_cast<size_t> (i)], ui::accentFor (0));

    constexpr const char* bottomIds[] = { "fusion", "balance", "drywet",
                                          "macroMotion", "motionRate", "input", "width", "output" };
    constexpr const char* bottomNames[] = { "FUSION", "BALANCE", "DRY / WET",
                                            "MOTION", "RATE", "INPUT", "WIDTH", "OUTPUT" };
    for (int i = 0; i < 8; ++i)
    {
        const int unit = i == 4 ? 2 : i == 5 || i == 7 ? 1 : i == 6 ? 3 : 0;
        setupDial (bottom[static_cast<size_t> (i)], bottomIds[i], bottomNames[i],
                   i < 3 ? ui::accentFor (1) : i < 5 ? ui::accentFor (3) : ui::accentFor (0),
                   unit == 0 || unit == 3, unit);
    }
    constexpr const char* macroIds[] = { "macroTransform", "macroMotion", "macroEnergy", "macroDensity",
                                         "macroSpace", "macroHarmonics", "macroEvolution", "macroFusion" };
    constexpr const char* macroNames[] = { "TRANSFORM", "MOTION", "ENERGY", "DENSITY",
                                           "SPACE", "HARMONICS", "EVOLUTION", "FUSION" };
    for (int i = 0; i < 8; ++i)
        setupDial (macros[static_cast<size_t> (i)], macroIds[i], macroNames[i],
                   i % 2 == 0 ? ui::accentFor (0) : ui::accentFor (3));
    bindProperties();
    changeLanguage (french);
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
    trajectoryAttachment.reset();
    divisionAttachment.reset();
    if (browser != nullptr) browser.reset();
    if (uiSettings != nullptr) uiSettings->saveIfNeeded();
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
    title (g, ui::word (french, "BIBLIOTHÈQUE  /  100 PRESETS D'USINE + PRESETS PERSONNELS",
                                "LIBRARY  /  100 FACTORY + USER PRESETS"),
           { presetDisplay.getX(), presetDisplay.getBottom() + 3,
             presetDisplay.getWidth() + browserButton.getWidth(), 17 },
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
    title (g, ui::word (french, "MOTEURS DE MATÉRIAUX", "MATERIAL ENGINES"), { leftBounds.getX() + 14, leftBounds.getY() + 15, leftBounds.getWidth() - 27, 21 },
           10.0f, ui::secondary, true);
    title (g, ui::word (french, "ÉDITION COUCHE ", "EDITING LAYER ") + juce::String (selectedLayer == 0 ? "A" : "B"),
           { leftBounds.getX() + 14, leftBounds.getY() + 41, leftBounds.getWidth() - 28, 20 },
           10.0f, ui::subdued);
    const int current = activeMaterial (selectedLayer);
    // The active material stripe, icon and layer badges are drawn by MaterialButton.
    g.setColour (ui::engaged); g.fillEllipse (static_cast<float> (leftBounds.getX() + 16),
                              static_cast<float> (leftBounds.getBottom() - 37), 4.0f, 4.0f);
    title (g, ui::word (french, "SIX MOTEURS ACTIFS", "SIX LIVE PROCESSORS"),
           { leftBounds.getX() + 27, leftBounds.getBottom() - 43, leftBounds.getWidth() - 35, 20 },
           9.0f, ui::subdued, true);

    title (g, ui::word (french, "PROPRIÉTÉS DU MATÉRIAU", "MATERIAL PROPERTIES"), { rightBounds.getX() + 19, rightBounds.getY() + 61,
                                      rightBounds.getWidth() - 38, 18 },
           10.0f, ui::secondary, true);
    title (g, ui::materialLabel (current, french),
           { rightBounds.getX() + 19, rightBounds.getY() + 81, rightBounds.getWidth() - 38, 35 },
           26.0f, ui::accentFor (current));
    title (g, ui::materialDescription (current, french),
           { rightBounds.getX() + 19, rightBounds.getY() + 117, rightBounds.getWidth() - 38, 20 },
           11.0f, ui::secondary);
    title (g, ui::word (french, "12 PARAMÈTRES AUDIO AUTOMATISABLES", "12 AUTOMATABLE AUDIO PARAMETERS"),
           { rightBounds.getX() + 18, rightBounds.getBottom() - 27, rightBounds.getWidth() - 36, 19 },
           9.0f, ui::subdued, true, juce::Justification::centred);
    g.setColour (ui::line);
    g.drawLine (static_cast<float> (rightBounds.getX() + 17), static_cast<float> (rightBounds.getBottom() - 33),
                static_cast<float> (rightBounds.getRight() - 17), static_cast<float> (rightBounds.getBottom() - 33), 1.0f);

    if (macroPage)
        title (g, ui::word (french, "HUIT MACROS DE PERFORMANCE", "EIGHT PERFORMANCE MACROS"), { footerAll.getX() + 17, footerAll.getY() + 11,
                                                footerAll.getWidth() - 34, 24 }, 11.0f, ui::secondary, true);
    else
    {
        title (g, ui::word (french, "FUSION A/B", "DUAL FUSION"),
               { footerFusion.getX() + 17, footerFusion.getY() + 10, 137, 19 },
               11.0f, ui::secondary, true);
        title (g, ui::word (french, "MOUVEMENT", "MOVEMENT"), { footerMotion.getX() + 17, footerMotion.getY() + 13, 200, 20 },
               11.0f, ui::secondary, true);
        title (g, ui::word (french, "SIGNAL & SORTIE", "SIGNAL & OUTPUT"), { footerOutput.getX() + 17, footerOutput.getY() + 13, 200, 20 },
               11.0f, ui::secondary, true);
        const auto da = activeMaterial (0), db = activeMaterial (1);
        // Routing tiles occupy the upper right; A/B labels stay on the left
        // in separate rows even at the minimum 1240px editor width.
        title (g, "A / " + ui::materialLabel (da, french),
               { footerFusion.getX() + 18, footerFusion.getY() + 32, 139, 17 },
               10.0f, ui::accentFor (da), true);
        title (g, "B / " + ui::materialLabel (db, french),
               { footerFusion.getX() + 18, footerFusion.getY() + 49, 139, 17 },
               10.0f, ui::accentFor (db), true);
    }
    // Deliberate visual breathing room below the three operational cards.
    title (g, ui::word (french, "Façonnez le son. Transformez la matière.",
                               "Shape Sound. Transform Matter. Create the Impossible."),
           { 23, footerAll.getBottom() + 6, juce::jmin (600, getWidth() - 46), 17 },
           10.0f, ui::subdued);
    title (g, "LIBERTY MUSIC SOFTWARE   /   CORE EDITION",
           { getWidth() - 375, footerAll.getBottom() + 6, 350, 17 },
           10.0f, ui::subdued, true, juce::Justification::centredRight);
}
void OrganixAudioProcessorEditor::resized()
{
    const auto geometry = organix::layout::calculate (getWidth(), getHeight());
    jassert (geometry.valid (getWidth(), getHeight()));
    const auto toJuce = [] (const organix::layout::Rect& r)
    {
        return juce::Rectangle<int> (r.x, r.y, r.w, r.h);
    };
    headerBounds = toJuce (geometry.header);
    leftBounds = toJuce (geometry.left);
    centerBounds = toJuce (geometry.center);
    rightBounds = toJuce (geometry.right);
    footerAll = toJuce (geometry.footer);
    footerFusion = toJuce (geometry.fusion);
    footerMotion = toJuce (geometry.movement);
    footerOutput = toJuce (geometry.output);
    visualizer.setBounds (centerBounds);

    auto headActions = headerBounds;
    macroToggle.setBounds (headActions.removeFromRight (86).reduced (4, 25));
    engaged.setBounds (headActions.removeFromRight (120).reduced (5, 25));
    auto languageArea = headActions.removeFromRight (127).reduced (8, 25);
    const int langHalf = languageArea.getWidth() / 2;
    languageFR.setBounds (languageArea.getX(), languageArea.getY(),
                          langHalf - 2, languageArea.getHeight());
    languageEN.setBounds (languageArea.getX() + langHalf + 2,
                          languageArea.getY(),
                          languageArea.getWidth() - langHalf - 2,
                          languageArea.getHeight());
    headActions.removeFromRight (8);
    // Fixed toolbar geometry avoids collisions with the 310px Liberty brand.
    auto controlsRect = headActions.removeFromRight (juce::jmin (485, headActions.getWidth() - 315));
    browserButton.setBounds (controlsRect.removeFromRight (111).reduced (3, 25));
    next.setBounds (controlsRect.removeFromRight (36).reduced (2, 25));
    previous.setBounds (controlsRect.removeFromRight (36).reduced (2, 25));
    presetDisplay.setBounds (controlsRect.reduced (3, 25));

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
            d.value.setVisible (true);
            layoutDial (d, { footerAll.getX() + 20 + i * stepW, footerAll.getY() + 39,
                             stepW, footerAll.getHeight() - 48 });
        }
        for (auto& d : bottom)
        {
            d.slider.setVisible (false); d.label.setVisible (false); d.value.setVisible (false);
        }
        routeSelector.setVisible (false);
        for (auto* c : { static_cast<juce::Component*> (&motionFreeButton),
                         static_cast<juce::Component*> (&trajectorySelect),
                         static_cast<juce::Component*> (&divisionSelect),
                         static_cast<juce::Component*> (&trajectoryLabel),
                         static_cast<juce::Component*> (&divisionLabel) })
            c->setVisible (false);
    }
    else
    {
        for (auto& d : macros)
        {
            d.slider.setVisible (false); d.label.setVisible (false); d.value.setVisible (false);
        }
        // All three panel bounds come from the shared, unit-tested geometry model.
        routeSelector.setVisible (true);
        routeSelector.setBounds (footerFusion.getRight() - 252,
                                 footerFusion.getY() + 8, 240, 63);
        for (auto* c : { static_cast<juce::Component*> (&motionFreeButton),
                         static_cast<juce::Component*> (&trajectorySelect),
                         static_cast<juce::Component*> (&divisionSelect),
                         static_cast<juce::Component*> (&trajectoryLabel),
                         static_cast<juce::Component*> (&divisionLabel) })
            c->setVisible (true);

        const int movementChoiceWidth = juce::jlimit (
            140, 170, juce::roundToInt (footerMotion.getWidth() * 0.45f));
        const int movementChoiceX = footerMotion.getRight() - movementChoiceWidth - 12;
        motionFreeButton.setBounds (footerMotion.getRight() - 87,
                                    footerMotion.getY() + 10, 74, 25);
        trajectoryLabel.setBounds (movementChoiceX, footerMotion.getY() + 42,
                                   movementChoiceWidth, 17);
        trajectorySelect.setBounds (movementChoiceX, footerMotion.getY() + 61,
                                    movementChoiceWidth, 29);
        divisionLabel.setBounds (movementChoiceX, footerMotion.getY() + 98,
                                 movementChoiceWidth, 16);
        divisionSelect.setBounds (movementChoiceX, footerMotion.getY() + 116,
                                  movementChoiceWidth, 29);

        for (int i = 0; i < 8; ++i)
        {
            auto& d = bottom[static_cast<size_t> (i)];
            d.slider.setVisible (true);
            d.label.setVisible (true);
            d.value.setVisible (true);
            if (i < 3)
            {
                const int cellW = (footerFusion.getWidth() - 24) / 3;
                layoutDial (d, { footerFusion.getX() + 12 + i * cellW,
                                 footerFusion.getY() + 76,
                                 cellW, footerFusion.getHeight() - 81 });
            }
            else if (i < 5)
            {
                const int left = footerMotion.getX() + 10;
                const int cellW = (movementChoiceX - left - 4) / 2;
                layoutDial (d, { left + (i - 3) * cellW,
                                 footerMotion.getY() + 57,
                                 cellW, footerMotion.getHeight() - 62 });
            }
            else
            {
                const int cellW = (footerOutput.getWidth() - 24) / 3;
                layoutDial (d, { footerOutput.getX() + 12 + (i - 5) * cellW,
                                 footerOutput.getY() + 57,
                                 cellW, footerOutput.getHeight() - 62 });
            }
        }
    }
    if (browser != nullptr) browser->setBounds (getLocalBounds());
}
void OrganixAudioProcessorEditor::timerCallback()
{
    refreshPreset();
    routeSelector.refresh();
    refreshMovementState();
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
