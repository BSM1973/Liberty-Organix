#include "OrganixTrajectoryControls.h"

namespace ui = organix::ui;
namespace
{
float get (OrganixAudioProcessor& p, const char* name)
{
    if (auto* v = p.parameters.getRawParameterValue (name))
        return v->load (std::memory_order_relaxed);
    return 0.0f;
}
void styleButton (juce::TextButton& b)
{
    b.setColour (juce::TextButton::buttonColourId, ui::inset);
    b.setColour (juce::TextButton::textColourOffId, ui::text);
}
void styleLabel (juce::Label& l)
{
    l.setColour (juce::Label::textColourId, ui::secondary);
    l.setFont (juce::Font (juce::FontOptions (10.5f)));
    l.setJustificationType (juce::Justification::centredLeft);
}
}

OrganixTrajectoryControls::OrganixTrajectoryControls (OrganixAudioProcessor& p) : audio (p)
{
    for (auto* c : { &shape, &beat })
    {
        c->setColour (juce::ComboBox::backgroundColourId, ui::inset);
        c->setColour (juce::ComboBox::textColourId, ui::text);
        c->setColour (juce::ComboBox::outlineColourId, ui::border);
        addAndMakeVisible (*c);
    }
    for (auto* s : { &speed, &depth })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 76, 24);
        s->setColour (juce::Slider::trackColourId, ui::accentFor (0));
        s->setColour (juce::Slider::thumbColourId, ui::accentFor (0));
        s->setColour (juce::Slider::textBoxTextColourId, ui::text);
        s->setColour (juce::Slider::textBoxBackgroundColourId, ui::inset);
        s->setColour (juce::Slider::textBoxOutlineColourId, ui::border);
        addAndMakeVisible (*s);
    }
    speed.textFromValueFunction = [this] (double value)
    {
        return juce::String (value, 2) +
            (get (audio, "trajectoryFree") > 0.5f ? " Hz" : " x");
    };
    speed.valueFromTextFunction = [] (const juce::String& value)
    {
        return value.getDoubleValue();
    };
    depth.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + "%";
    };
    depth.valueFromTextFunction = [] (const juce::String& value)
    {
        return value.getDoubleValue() / 100.0;
    };
    for (auto* c : { &playback, &freeSync, &direction, &draw })
    {
        styleButton (*c);
        addAndMakeVisible (*c);
    }
    for (auto* l : { &shapeTitle, &beatTitle, &speedTitle, &depthTitle, &status })
    {
        styleLabel (*l);
        addAndMakeVisible (*l);
    }
    status.setJustificationType (juce::Justification::centred);
    status.setColour (juce::Label::textColourId, ui::accentFor (0));
    playback.setComponentID ("midi:trajectoryEnable");
    freeSync.setComponentID ("midi:trajectoryFree");
    direction.setComponentID ("midi:trajectoryReverse");
    shape.setComponentID ("midi:trajectoryShape");
    beat.setComponentID ("midi:trajectoryDivision");
    speed.setComponentID ("midi:trajectorySpeed");
    depth.setComponentID ("midi:trajectoryDepth");
    shapeLink = std::make_unique<ComboLink> (audio.parameters, "trajectoryShape", shape);
    beatLink = std::make_unique<ComboLink> (audio.parameters, "trajectoryDivision", beat);
    speedLink = std::make_unique<SliderLink> (audio.parameters, "trajectorySpeed", speed);
    depthLink = std::make_unique<SliderLink> (audio.parameters, "trajectoryDepth", depth);

    playback.onClick = [this]
    {
        setToggle ("trajectoryEnable", get (audio, "trajectoryEnable") <= 0.5f);
        refresh();
    };
    freeSync.onClick = [this]
    {
        setToggle ("trajectoryFree", get (audio, "trajectoryFree") <= 0.5f);
        speed.updateText();
        refresh();
    };
    direction.onClick = [this]
    {
        setToggle ("trajectoryReverse", get (audio, "trajectoryReverse") <= 0.5f);
        refresh();
    };
    draw.onClick = [this]
    {
        drawing = ! drawing;
        if (onDrawingToggled) onDrawingToggled (drawing);
        refresh();
    };
    shape.onChange = [this]
    {
        if (drawing && shape.getSelectedId() != 4)
        {
            drawing = false;
            if (onDrawingToggled) onDrawingToggled (false);
        }
        refresh();
    };
    setFrench (true);
    startTimerHz (12);
}
OrganixTrajectoryControls::~OrganixTrajectoryControls()
{
    stopTimer();
    speedLink.reset();
    depthLink.reset();
    shapeLink.reset();
    beatLink.reset();
}
void OrganixTrajectoryControls::setToggle (const char* name, bool value)
{
    if (auto* param = audio.parameters.getParameter (name))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (value ? 1.0f : 0.0f));
        param->endChangeGesture();
    }
}
void OrganixTrajectoryControls::setFrench (bool value)
{
    french = value;
    const int shapeID = shape.getSelectedId();
    const int divisionID = beat.getSelectedId();
    shape.clear (juce::dontSendNotification);
    const char* fr[] = { "CERCLE", "SPIRALE", "ZIGZAG", "DESSIN LIBRE" };
    const char* en[] = { "CIRCLE", "SPIRAL", "ZIGZAG", "FREEHAND" };
    for (int i = 0; i < 4; ++i)
        shape.addItem (french ? fr[i] : en[i], i + 1);
    shape.setSelectedId (shapeID > 0 ? shapeID : 1, juce::dontSendNotification);
    beat.clear (juce::dontSendNotification);
    constexpr const char* divisions[] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T" };
    for (int i = 0; i < 7; ++i) beat.addItem (divisions[i], i + 1);
    beat.setSelectedId (divisionID > 0 ? divisionID : 3, juce::dontSendNotification);

    shapeTitle.setText (ui::word (french, "FORME DU MOUVEMENT", "MOVEMENT SHAPE"),
                        juce::dontSendNotification);
    beatTitle.setText (ui::word (french, "DIVISION RYTHMIQUE", "BEAT DIVISION"),
                       juce::dontSendNotification);
    speedTitle.setText (ui::word (french, "VITESSE", "SPEED"), juce::dontSendNotification);
    depthTitle.setText (ui::word (french, "PROFONDEUR DU MOUVEMENT", "MOTION DEPTH"),
                        juce::dontSendNotification);
    refresh();
}
void OrganixTrajectoryControls::timerCallback()
{
    if (isShowing()) refresh();
}
void OrganixTrajectoryControls::refresh()
{
    const bool on = get (audio, "trajectoryEnable") > 0.5f;
    const bool free = get (audio, "trajectoryFree") > 0.5f;
    const bool reverse = get (audio, "trajectoryReverse") > 0.5f;
    playback.setButtonText (on ? ui::word (french, "ARRÊTER", "STOP")
                               : ui::word (french, "ACTIVER", "START"));
    freeSync.setButtonText (free ? ui::word (french, "LIBRE", "FREE") : "SYNC");
    direction.setButtonText (reverse ? ui::word (french, "ARRIÈRE", "REVERSE")
                                     : ui::word (french, "AVANT", "FORWARD"));
    draw.setButtonText (drawing ? ui::word (french, "FIN DU DESSIN", "END DRAW")
                                : ui::word (french, "DESSINER", "DRAW"));
    beat.setEnabled (! free);
    beatTitle.setColour (juce::Label::textColourId,
                         free ? ui::subdued : ui::secondary);
    status.setText (drawing
        ? ui::word (french, "GLISSER DANS LE PAD XY", "DRAG IN THE XY PAD")
        : on ? ui::word (french, "TRAJECTOIRE AUDIO ACTIVE", "AUDIO TRAJECTORY ACTIVE")
             : ui::word (french, "TRAJECTOIRE EN ATTENTE", "TRAJECTORY READY"),
        juce::dontSendNotification);
    playback.setColour (juce::TextButton::buttonColourId,
                        on ? juce::Colour (0xff17473d) : ui::inset);
    draw.setColour (juce::TextButton::buttonColourId,
                    drawing ? juce::Colour (0xff174350) : ui::inset);
    speed.updateText();
}
void OrganixTrajectoryControls::paint (juce::Graphics& g)
{
    g.setColour (ui::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 7.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 7.0f, 1.0f);
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    g.drawFittedText ("TRAJECTORY DESIGNER",
                      {15, 14, getWidth() - 30, 24},
                      juce::Justification::centredLeft, 1);
    g.setColour (ui::secondary);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawFittedText (ui::word (french, "DESSINEZ LE MOUVEMENT DU SON",
                                         "DRAW HOW THE SOUND MOVES"),
                      {15, 40, getWidth() - 30, 18},
                      juce::Justification::centredLeft, 1);
    g.setColour (ui::line);
    for (int y : {104, 183, 310})
        g.drawLine (15.0f, static_cast<float> (y),
                    static_cast<float> (getWidth() - 15), static_cast<float> (y), 1.0f);
    g.setColour (ui::subdued);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawFittedText (ui::word (french,
        "Les positions XY modifient le mélange des 4 scènes DSP.",
        "XY position changes the actual four-scene DSP blend."),
        {13, getHeight() - 31, getWidth() - 26, 17},
        juce::Justification::centred, 1);
}
void OrganixTrajectoryControls::resized()
{
    const int w = getWidth(), pad = 16;
    shapeTitle.setBounds (pad, 69, w - 32, 18);
    shape.setBounds (pad, 88, w - 32, 30);
    const int gap = 8, half = (w - 2 * pad - gap) / 2;
    playback.setBounds (pad, 129, half, 30);
    draw.setBounds (pad + half + gap, 129, half, 30);
    freeSync.setBounds (pad, 169, half, 30);
    direction.setBounds (pad + half + gap, 169, half, 30);
    speedTitle.setBounds (pad, 213, w - 32, 19);
    speed.setBounds (pad, 233, w - 32, 30);
    depthTitle.setBounds (pad, 271, w - 32, 18);
    depth.setBounds (pad, 290, w - 32, 30);
    beatTitle.setBounds (pad, 334, w - 32, 19);
    beat.setBounds (pad, 355, w - 32, 31);
    status.setBounds (pad, 405, w - 32, 31);
}
