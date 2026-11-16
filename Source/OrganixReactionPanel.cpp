#include "OrganixReactionPanel.h"
#include "MaterialReactions.h"
#include <algorithm>
#include <cmath>

namespace ui = organix::ui;
namespace
{
float readParameter (OrganixAudioProcessor& processor, const char* id) noexcept
{
    if (auto* value = processor.parameters.getRawParameterValue (id))
        return value->load (std::memory_order_relaxed);
    return 0.0f;
}
void setLabel (juce::Label& label)
{
    label.setFont (juce::Font (juce::FontOptions (10.5f)));
    label.setColour (juce::Label::textColourId, ui::secondary);
    label.setJustificationType (juce::Justification::centredLeft);
}
void drawText (juce::Graphics& g, juce::String s, juce::Rectangle<int> rect,
               float size, juce::Colour colour, bool heavy = false,
               juce::Justification justification = juce::Justification::centredLeft)
{
    g.setColour (colour);
    g.setFont (juce::Font (juce::FontOptions (
        size, heavy ? juce::Font::bold : juce::Font::plain)));
    g.drawFittedText (s, rect, justification, 1, 0.8f);
}
}

OrganixReactionPanel::OrganixReactionPanel (OrganixAudioProcessor& p) : audio (p)
{
    addAndMakeVisible (power);
    power.onClick = [this]
    {
        if (auto* parameter = audio.parameters.getParameter ("reactionEnable"))
        {
            const bool next = readParameter (audio, "reactionEnable") < 0.5f;
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (next ? 1.0f : 0.0f));
            parameter->endChangeGesture();
        }
        updateLabels();
        repaint();
    };

    direction.setColour (juce::ComboBox::backgroundColourId, ui::inset);
    direction.setColour (juce::ComboBox::textColourId, ui::text);
    direction.setColour (juce::ComboBox::outlineColourId, ui::border);
    addAndMakeVisible (direction);

    for (auto* slider : { &amount, &response })
    {
        slider->setSliderStyle (juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 65, 25);
        slider->setColour (juce::Slider::trackColourId, ui::accentFor (0));
        slider->setColour (juce::Slider::thumbColourId, ui::accentFor (0));
        slider->setColour (juce::Slider::textBoxTextColourId, ui::text);
        slider->setColour (juce::Slider::textBoxBackgroundColourId, ui::inset);
        slider->setColour (juce::Slider::textBoxOutlineColourId, ui::border);
        slider->textFromValueFunction = [] (double value)
        {
            return juce::String (juce::roundToInt (value * 100.0)) + "%";
        };
        slider->valueFromTextFunction = [] (const juce::String& txt)
        {
            return txt.getDoubleValue() / 100.0;
        };
        addAndMakeVisible (*slider);
    }
    amount.setDoubleClickReturnValue (true, 0.60);
    response.setDoubleClickReturnValue (true, 0.40);
    for (auto* label : { &directionLabel, &amountLabel, &responseLabel })
    {
        setLabel (*label);
        addAndMakeVisible (*label);
    }

    setFrench (true);
    directionLink = std::make_unique<ComboAttachment> (
        audio.parameters, "reactionDirection", direction);
    amountLink = std::make_unique<SliderAttachment> (
        audio.parameters, "reactionAmount", amount);
    responseLink = std::make_unique<SliderAttachment> (
        audio.parameters, "reactionResponse", response);
    startTimerHz (20);
}
OrganixReactionPanel::~OrganixReactionPanel()
{
    stopTimer();
    directionLink.reset();
    amountLink.reset();
    responseLink.reset();
}
void OrganixReactionPanel::setFrench (bool value)
{
    french = value;
    const int previous = direction.getSelectedId();
    direction.clear (juce::dontSendNotification);
    direction.addItem (ui::word (french, "A VERS B", "A TO B"), 1);
    direction.addItem (ui::word (french, "B VERS A", "B TO A"), 2);
    direction.addItem (ui::word (french, "MUTUELLE", "MUTUAL"), 3);
    direction.setSelectedId (previous > 0 ? previous : 3, juce::dontSendNotification);
    directionLabel.setText (
        ui::word (french, "SENS DES INTERACTIONS", "REACTION DIRECTION"),
        juce::dontSendNotification);
    amountLabel.setText (
        ui::word (french, "INTENSITÉ DES RÉACTIONS", "REACTION INTENSITY"),
        juce::dontSendNotification);
    responseLabel.setText (
        ui::word (french, "TEMPS DE RÉPONSE", "RESPONSE TIME"),
        juce::dontSendNotification);
    updateLabels();
    repaint();
}
void OrganixReactionPanel::updateLabels()
{
    const bool enabled = readParameter (audio, "reactionEnable") > 0.5f;
    power.setButtonText (enabled ?
        ui::word (french, "RÉACTIONS ACTIVES", "REACTIONS ON") :
        ui::word (french, "ACTIVER LES RÉACTIONS", "ENABLE REACTIONS"));
    power.setColour (juce::TextButton::buttonColourId,
                     enabled ? juce::Colour (0xff15433f) : ui::inset);
    power.setColour (juce::TextButton::textColourOffId,
                     enabled ? ui::engaged : ui::secondary);
}
void OrganixReactionPanel::timerCallback()
{
    if (! isShowing()) return;
    const bool enabled = readParameter (audio, "reactionEnable") > 0.5f;
    const float targetAB = enabled
        ? audio.reactionActivityAB.load (std::memory_order_relaxed) : 0.0f;
    const float targetBA = enabled
        ? audio.reactionActivityBA.load (std::memory_order_relaxed) : 0.0f;
    const auto approach = [] (float old, float target)
    {
        const float alpha = target > old ? 0.28f : 0.11f;
        return old + (std::clamp (target, 0.0f, 1.0f) - old) * alpha;
    };
    activityAB = approach (activityAB, targetAB);
    activityBA = approach (activityBA, targetBA);
    const int signature = juce::jlimit (0, 5,
        static_cast<int> (readParameter (audio, "materialA"))) +
        6 * juce::jlimit (0, 5, static_cast<int> (readParameter (audio, "materialB"))) +
        36 * (enabled ? 1 : 0);
    if (signature != previousSignature)
    {
        previousSignature = signature;
        updateLabels();
    }
    repaint();
}
void OrganixReactionPanel::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (ui::panel);
    g.fillRoundedRectangle (area, 7.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (area, 7.0f, 1.0f);
    drawText (g, "MATERIAL REACTIONS", {13, 11, getWidth() - 26, 23},
              14.0f, ui::text, true);
    drawText (g, ui::word (french,
              "DEUX MATIÈRES, UN COMPORTEMENT VIVANT",
              "TWO MATERIALS, ONE LIVING RESPONSE"),
              {13, 34, getWidth() - 26, 19}, 9.7f, ui::secondary);

    // Dedicated cards make the source and destination unambiguous.
    const int aId = juce::jlimit (0, 5,
        static_cast<int> (readParameter (audio, "materialA")));
    const int bId = juce::jlimit (0, 5,
        static_cast<int> (readParameter (audio, "materialB")));
    const auto a = static_cast<organix::Material> (aId);
    const auto b = static_cast<organix::Material> (bId);
    const auto ab = organix::MaterialReactions::description (a, b);
    const auto ba = organix::MaterialReactions::description (b, a);
    const bool enabled = readParameter (audio, "reactionEnable") > 0.5f;
    const bool activeB = readParameter (audio, "enableB") > 0.5f;
    const int mode = juce::jlimit (0, 2,
        static_cast<int> (readParameter (audio, "reactionDirection")));
    const int rowsY[] = {288, 350};
    for (int index = 0; index < 2; ++index)
    {
        const int y = rowsY[index];
        const auto color = ui::accentFor (index == 0 ? aId : bId);
        const float level = index == 0 ? activityAB : activityBA;
        const bool enabledDirection = enabled && activeB &&
                                      (mode == 2 || mode == index);
        g.setColour (ui::inset);
        g.fillRoundedRectangle (10.0f, static_cast<float> (y),
                                static_cast<float> (getWidth() - 20), 58.0f, 5.0f);
        g.setColour (color.withAlpha (enabledDirection ? 0.9f : 0.28f));
        g.fillRoundedRectangle (17.0f, static_cast<float> (y + 11),
                                3.0f, 33.0f, 1.5f);
        drawText (g, index == 0 ? "A > B" : "B > A",
                  {28, y + 6, 74, 18}, 12.5f, color, true);
        const auto& pair = index == 0 ? ab : ba;
        drawText (g, juce::String::fromUTF8 (french ? pair.french : pair.english),
                  {98, y + 6, getWidth() - 114, 18}, 11.3f,
                  enabledDirection ? ui::text : ui::subdued, true);
        g.setColour (ui::line);
        g.fillRoundedRectangle (29.0f, static_cast<float> (y + 35),
                                static_cast<float> (getWidth() - 59), 7.0f, 3.0f);
        if (enabledDirection)
        {
            g.setColour (color);
            g.fillRoundedRectangle (29.0f, static_cast<float> (y + 35),
                static_cast<float> (getWidth() - 59) * level, 7.0f, 3.0f);
        }
    }
    drawText (g, ui::word (french, "SANS BOUCLE INSTANTANÉE / DSP TEMPS RÉEL",
                                "NO ZERO-DELAY LOOP / REAL-TIME DSP"),
              {12, getHeight() - 22, getWidth() - 24, 15}, 9.2f,
              ui::subdued, false, juce::Justification::centred);
}
void OrganixReactionPanel::resized()
{
    const int pad = 14, width = getWidth() - pad * 2;
    power.setBounds (pad, 64, width, 30);
    directionLabel.setBounds (pad, 105, width, 18);
    direction.setBounds (pad, 125, width, 29);
    amountLabel.setBounds (pad, 171, width, 18);
    amount.setBounds (pad, 191, width, 31);
    responseLabel.setBounds (pad, 230, width, 18);
    response.setBounds (pad, 250, width, 31);
}
