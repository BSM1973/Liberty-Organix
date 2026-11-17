#include "OrganixMorphView.h"
#include <algorithm>
#include <cmath>

namespace ui = organix::ui;
namespace
{
float current (OrganixAudioProcessor& processor, const char* id) noexcept
{
    if (auto* p = processor.parameters.getRawParameterValue (id))
        return p->load (std::memory_order_relaxed);
    return 0.0f;
}
void label (juce::Graphics& g, juce::String text, juce::Rectangle<int> bounds,
            float size, juce::Colour colour, bool heavy = false,
            juce::Justification justification = juce::Justification::centredLeft)
{
    g.setColour (colour);
    g.setFont (juce::Font (juce::FontOptions (
        size, heavy ? juce::Font::bold : juce::Font::plain)));
    g.drawFittedText (text, bounds, justification, 1, 0.83f);
}
}
OrganixMorphPad::OrganixMorphPad (OrganixAudioProcessor& processor) : audio (processor)
{
    xParam = audio.parameters.getParameter ("morphX");
    yParam = audio.parameters.getParameter ("morphY");
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    visualX = current (audio, "morphX");
    visualY = current (audio, "morphY");
    startTimerHz (30);
}
OrganixMorphPad::~OrganixMorphPad()
{
    stopTimer();
    if (dragging)
    {
        if (xParam) xParam->endChangeGesture();
        if (yParam) yParam->endChangeGesture();
    }
}
juce::Rectangle<float> OrganixMorphPad::field() const
{
    const float width = static_cast<float> (getWidth());
    const float height = static_cast<float> (getHeight());
    return { 80.0f, 140.0f, std::max (50.0f, width - 160.0f),
             std::max (50.0f, height - 235.0f) };
}
void OrganixMorphPad::timerCallback()
{
    if (! isShowing()) return;
    const bool pathPlaying = current (audio, "trajectoryEnable") > 0.5f;
    const float x = pathPlaying ? audio.trajectoryReadX.load (std::memory_order_relaxed)
                                : current (audio, "morphX");
    const float y = pathPlaying ? audio.trajectoryReadY.load (std::memory_order_relaxed)
                                : current (audio, "morphY");
    visualX += (x - visualX) * 0.30f;
    visualY += (y - visualY) * 0.30f;
    for (int i = static_cast<int> (trace.size()) - 1; i > 0; --i)
        trace[static_cast<size_t> (i)] = trace[static_cast<size_t> (i - 1)];
    trace[0] = {visualX, visualY};
    validTrace = std::min (static_cast<int> (trace.size()), validTrace + 1);
    repaint();
}
void OrganixMorphPad::changeXY (juce::Point<float> p)
{
    const auto r = field();
    const float x = juce::jlimit (0.0f, 1.0f, (p.x - r.getX()) / r.getWidth());
    const float y = juce::jlimit (0.0f, 1.0f, (p.y - r.getY()) / r.getHeight());
    if (xParam != nullptr) xParam->setValueNotifyingHost (xParam->convertTo0to1 (x));
    if (yParam != nullptr) yParam->setValueNotifyingHost (yParam->convertTo0to1 (y));
}
void OrganixMorphPad::addDrawPoint (juce::Point<float> point)
{
    auto region = field();
    const organix::TrajectoryPoint normalized {
        juce::jlimit (0.0f, 1.0f, (point.x - region.getX()) / region.getWidth()),
        juce::jlimit (0.0f, 1.0f, (point.y - region.getY()) / region.getHeight())
    };
    if (draft.count > 0)
    {
        const auto last = draft.points[static_cast<size_t> (draft.count - 1)];
        const float dx = normalized.x - last.x, dy = normalized.y - last.y;
        if (dx * dx + dy * dy < 0.00010f) return;
    }
    if (draft.count == organix::TrajectoryPath::maxPoints)
    {
        // Preserve first/last direction but downsample in place. No allocation.
        for (int j = 0; j < organix::TrajectoryPath::maxPoints / 2; ++j)
            draft.points[static_cast<size_t> (j)] =
                draft.points[static_cast<size_t> (2 * j)];
        draft.count = organix::TrajectoryPath::maxPoints / 2;
    }
    draft.points[static_cast<size_t> (draft.count++)] = normalized;
    repaint();
}
void OrganixMorphPad::mouseDown (const juce::MouseEvent& event)
{
    if (! field().expanded (16.0f).contains (event.position)) return;
    dragging = true;
    if (drawingEnabled)
    {
        drafting = true;
        draft = {};
        addDrawPoint (event.position);
        return;
    }
    if (xParam) xParam->beginChangeGesture();
    if (yParam) yParam->beginChangeGesture();
    changeXY (event.position);
}
void OrganixMorphPad::mouseDrag (const juce::MouseEvent& event)
{
    if (dragging)
    {
        if (drawingEnabled) addDrawPoint (event.position);
        else changeXY (event.position);
    }
}
void OrganixMorphPad::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    if (drafting)
    {
        drafting = false;
        if (draft.count >= 2)
        {
            audio.setTrajectoryPath (draft);
            if (auto* shape = audio.parameters.getParameter ("trajectoryShape"))
            {
                shape->beginChangeGesture();
                shape->setValueNotifyingHost (shape->convertTo0to1 (3.0f));
                shape->endChangeGesture();
            }
            if (auto* enable = audio.parameters.getParameter ("trajectoryEnable"))
            {
                enable->beginChangeGesture();
                enable->setValueNotifyingHost (enable->convertTo0to1 (1.0f));
                enable->endChangeGesture();
            }
        }
        repaint();
        return;
    }
    if (xParam) xParam->endChangeGesture();
    if (yParam) yParam->endChangeGesture();
}
void OrganixMorphPad::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (! field().expanded (16.0f).contains (event.position)) return;
    if (! drawingEnabled) changeXY (event.position);
}
void OrganixMorphPad::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (ui::surface);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    label (g, ui::word (french, "MORPHING QUATRE SCÈNES", "FOUR-SCENE MORPHING"),
           {24, 14, getWidth() - 48, 24}, 13.0f, ui::secondary, true);
    label (g, "A / B / C / D", {24, 36, 200, 35}, 27.0f, ui::text, true);
    label (g, ui::word (french,
             "DÉPLACEZ LE POINT POUR MÉLANGER QUATRE IDENTITÉS SONORES",
             "DRAG THE POINT TO BLEND FOUR SONIC IDENTITIES"),
           {24, 74, getWidth() - 48, 20}, 10.0f, ui::subdued);
    const bool trajectoryPlaying = current (audio, "trajectoryEnable") > 0.5f;
    const bool enabled = current (audio, "morphEnable") > 0.5f || trajectoryPlaying;
    label (g, enabled ? ui::word (french, "MORPH ACTIF", "MORPH ACTIVE")
                       : ui::word (french, "MORPH INACTIF", "MORPH OFF"),
           {getWidth() - 198, 42, 174, 21}, 11.0f,
           enabled ? ui::engaged : ui::subdued, true, juce::Justification::centredRight);

    const auto r = field();
    juce::ColourGradient gradient (juce::Colour (0xff152b3b), r.getX(), r.getY(),
                                    juce::Colour (0xff131323), r.getRight(),
                                    r.getBottom(), false);
    gradient.addColour (0.5, juce::Colour (0xff15263b));
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (r.expanded (9.0f), 11.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (r.expanded (9.0f), 11.0f, 1.0f);

    // Mapping is explicit: A top-left, B top-right, C bottom-left, D bottom-right.
    std::array<juce::Point<float>,4> corners {{
        {r.getX(),r.getY()}, {r.getRight(),r.getY()},
        {r.getX(),r.getBottom()}, {r.getRight(),r.getBottom()}
    }};
    for (int k = 1; k < 5; ++k)
    {
        g.setColour (ui::line.withAlpha (0.72f));
        const float x = r.getX() + r.getWidth() * k / 5.0f;
        const float y = r.getY() + r.getHeight() * k / 5.0f;
        g.drawLine (x, r.getY(), x, r.getBottom(), 0.8f);
        g.drawLine (r.getX(), y, r.getRight(), y, 0.8f);
    }
    g.setColour (ui::secondary.withAlpha (0.34f));
    g.drawLine (corners[0].x, corners[0].y, corners[3].x, corners[3].y, 0.9f);
    g.drawLine (corners[1].x, corners[1].y, corners[2].x, corners[2].y, 0.9f);

    // Display the actual trajectory curve, not just a cosmetic preset icon.
    const int shape = juce::jlimit (0, 3,
        static_cast<int> (current (audio, "trajectoryShape")));
    const auto path = drafting ? draft : audio.getTrajectoryPath();
    juce::Path curve;
    if (drawingEnabled && drafting && path.count > 1)
    {
        for (int k = 0; k < path.count; ++k)
        {
            const auto p = path.points[static_cast<size_t> (k)];
            const float xx = r.getX() + r.getWidth() * p.x;
            const float yy = r.getY() + r.getHeight() * p.y;
            if (k == 0) curve.startNewSubPath (xx, yy);
            else curve.lineTo (xx, yy);
        }
    }
    else
    {
        for (int k = 0; k <= 128; ++k)
        {
            const auto p = organix::TrajectoryDesigner::sample (
                shape, static_cast<float> (k) / 128.0f, path);
            const float xx = r.getX() + r.getWidth() * p.x;
            const float yy = r.getY() + r.getHeight() * p.y;
            if (k == 0) curve.startNewSubPath (xx, yy);
            else curve.lineTo (xx, yy);
        }
    }
    g.setColour (ui::accentFor (4).withAlpha (trajectoryPlaying ? 0.65f : 0.35f));
    g.strokePath (curve, juce::PathStrokeType (2.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto weights = organix::Morph4D::weights (visualX, visualY);
    for (int i = 0; i < 4; ++i)
    {
        const auto scene = audio.readMorphScene (i);
        const auto accent = ui::accentFor (scene.materialA);
        const auto& c = corners[static_cast<size_t> (i)];
        g.setColour (accent.withAlpha (0.33f + 0.67f * weights[static_cast<size_t> (i)]));
        g.fillEllipse (c.x - 17.0f, c.y - 17.0f, 34.0f, 34.0f);
        g.setColour (accent);
        g.drawEllipse (c.x - 17.0f, c.y - 17.0f, 34.0f, 34.0f, 1.6f);
        const char sceneName = static_cast<char> ('A' + i);
        label (g, juce::String::charToString (static_cast<juce_wchar> (sceneName)),
               {static_cast<int> (c.x) - 11, static_cast<int> (c.y) - 12, 22, 24},
               14.0f, ui::text, true, juce::Justification::centred);
        const auto text = ui::word (french, "SCÈNE ", "SCENE ") + juce::String::charToString (sceneName)
                        + "  /  " + ui::materialLabel (scene.materialA, french);
        const int textX = i % 2 == 0 ? static_cast<int> (c.x) - 6
                                      : static_cast<int> (c.x) - 190;
        const int textY = i < 2 ? static_cast<int> (c.y) - 45
                                : static_cast<int> (c.y) + 26;
        label (g, text, {textX, textY, 198, 16}, 10.0f, accent, true,
               i % 2 == 0 ? juce::Justification::centredLeft
                           : juce::Justification::centredRight);
    }

    juce::Path trail;
    for (int i = validTrace - 1; i >= 0; --i)
    {
        const auto& t = trace[static_cast<size_t> (i)];
        const float x = r.getX() + t.x * r.getWidth();
        const float y = r.getY() + t.y * r.getHeight();
        if (i == validTrace - 1) trail.startNewSubPath (x,y);
        else trail.lineTo (x,y);
    }
    g.setColour (ui::accentFor (0).withAlpha (0.31f));
    if (validTrace > 1) g.strokePath (trail, juce::PathStrokeType (1.7f));

    const float pointerX = r.getX() + visualX * r.getWidth();
    const float pointerY = r.getY() + visualY * r.getHeight();
    g.setColour (ui::accentFor (0).withAlpha (0.15f));
    g.fillEllipse (pointerX - 22.0f, pointerY - 22.0f, 44.0f, 44.0f);
    g.setColour (ui::accentFor (0));
    g.fillEllipse (pointerX - 7.0f, pointerY - 7.0f, 14.0f, 14.0f);
    g.drawEllipse (pointerX - 14.0f, pointerY - 14.0f, 28.0f, 28.0f, 1.3f);
    g.setColour (ui::secondary);
    label (g, juce::String (juce::roundToInt (visualX * 100.0f)) + "%  /  " +
           juce::String (juce::roundToInt (visualY * 100.0f)) + "%",
           {24, getHeight() - 44, getWidth() - 48, 20}, 12.0f,
           ui::accentFor (0), true, juce::Justification::centred);
    label (g, drawingEnabled
           ? ui::word (french, "DESSIN : GLISSEZ DANS LE CHAMP CENTRAL",
                             "DRAW: DRAG IN THE CENTRAL FIELD")
           : ui::word (french,
             "LA COURBE ANIME LES QUATRE SCÈNES ET LE SON",
             "THE CURVE DRIVES ALL FOUR SCENES AND THE AUDIO"),
           {24, getHeight() - 24, getWidth() - 48, 16}, 9.0f,
           ui::subdued, false, juce::Justification::centred);
}

OrganixMorphControls::OrganixMorphControls (OrganixAudioProcessor& processor)
    : audio (processor)
{
    enableButton.onClick = [this]
    {
        if (auto* p = audio.parameters.getParameter ("morphEnable"))
        {
            p->beginChangeGesture();
            const bool newState = current (audio, "morphEnable") < 0.5f;
            p->setValueNotifyingHost (p->convertTo0to1 (newState ? 1.0f : 0.0f));
            p->endChangeGesture();
        }
        refresh();
    };
    addAndMakeVisible (enableButton);
    enableButton.setComponentID ("midi:morphEnable");
    for (int i = 0; i < 4; ++i)
    {
        capture[static_cast<size_t> (i)].onClick = [this, i]
        {
            audio.captureMorphScene (i);
            repaint();
        };
        listen[static_cast<size_t> (i)].onClick = [this, i]
        {
            audio.goToMorphScene (i);
        };
        capture[static_cast<size_t> (i)].setColour (
            juce::TextButton::buttonColourId, ui::surface);
        listen[static_cast<size_t> (i)].setColour (
            juce::TextButton::buttonColourId, juce::Colour (0xff143640));
        addAndMakeVisible (capture[static_cast<size_t> (i)]);
        addAndMakeVisible (listen[static_cast<size_t> (i)]);
    }
    setFrench (true);
    startTimerHz (10);
}
OrganixMorphControls::~OrganixMorphControls() { stopTimer(); }
void OrganixMorphControls::setFrench (bool value)
{
    french = value;
    for (int i = 0; i < 4; ++i)
    {
        const auto suffix = juce::String::charToString (static_cast<juce_wchar> ('A' + i));
        capture[static_cast<size_t> (i)].setButtonText (
            ui::word (french, "CAPTURER ", "CAPTURE ") + suffix);
        listen[static_cast<size_t> (i)].setButtonText (
            ui::word (french, "ÉCOUTER ", "LISTEN ") + suffix);
        capture[static_cast<size_t> (i)].setTooltip (
            ui::word (french, "Mémoriser tous les paramètres sonores actuels",
                                 "Capture the current sound to this scene"));
        listen[static_cast<size_t> (i)].setTooltip (
            ui::word (french, "Déplacer le morphing XY vers cette scène",
                                 "Move the morph XY position to this scene"));
    }
    refresh();
    repaint();
}
void OrganixMorphControls::refresh()
{
    const bool value = current (audio, "morphEnable") > 0.5f;
    if (value != enabled) enabled = value;
    enableButton.setButtonText (
        enabled ? ui::word (french, "MORPH ACTIF", "MORPH ON")
                : ui::word (french, "ACTIVER MORPH", "ENABLE MORPH"));
    enableButton.setColour (juce::TextButton::buttonColourId,
                            enabled ? juce::Colour (0xff123b36) : ui::surface);
    enableButton.setColour (juce::TextButton::textColourOffId,
                            enabled ? ui::engaged : ui::secondary);
}
void OrganixMorphControls::timerCallback()
{
    if (! isShowing()) return;
    refresh();
    repaint();
}
void OrganixMorphControls::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (ui::panel);
    g.fillRoundedRectangle (full, 7.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (full, 7.0f, 1.0f);
    label (g, ui::word (french, "QUATRE SCÈNES", "FOUR SCENES"),
           {15, 8, 230, 21}, 11.0f, ui::secondary, true);
    label (g, ui::word (french,
           "CAPTURER UN SON, PUIS MÉLANGER LES SCÈNES AU PAD XY",
           "CAPTURE A SOUND, THEN BLEND THE SCENES ON THE XY PAD"),
           {210, 10, getWidth() - 400, 18}, 9.0f, ui::subdued);
    const int areaW = getWidth() - 24;
    const int cellW = areaW / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto scene = audio.readMorphScene (i);
        const int x = 12 + i * cellW, y = 37;
        g.setColour (ui::inset);
        g.fillRoundedRectangle (static_cast<float> (x),
             static_cast<float> (y), static_cast<float> (cellW - 10),
             static_cast<float> (getHeight() - 45), 5.0f);
        const auto accent = ui::accentFor (scene.materialA);
        g.setColour (accent.withAlpha (0.85f));
        g.fillRoundedRectangle (static_cast<float> (x + 7),
                                static_cast<float> (y + 9),
                                3.0f, 20.0f, 1.5f);
        const auto id = juce::String::charToString (static_cast<juce_wchar> ('A' + i));
        label (g, ui::word (french, "SCÈNE ", "SCENE ") + id + "  /  " +
               ui::materialLabel (scene.materialA, french), 
               {x + 18, y + 8, cellW - 37, 21}, 12.0f, accent, true);
    }
}
void OrganixMorphControls::resized()
{
    enableButton.setBounds (getWidth() - 164, 8, 148, 24);
    const int cellW = (getWidth() - 24) / 4;
    for (int i = 0; i < 4; ++i)
    {
        const int x = 12 + i * cellW;
        const int half = (cellW - 24) / 2;
        const int y = getHeight() - 51;
        capture[static_cast<size_t> (i)].setBounds (x + 8, y, half - 2, 30);
        listen[static_cast<size_t> (i)].setBounds (x + 10 + half, y, half - 2, 30);
    }
}
