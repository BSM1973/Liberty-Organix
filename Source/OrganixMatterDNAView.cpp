#include "OrganixMatterDNAView.h"
#include <algorithm>

namespace ui = organix::ui;
namespace
{
float readParameter (OrganixAudioProcessor& processor, const char* id)
{
    if (const auto* value = processor.parameters.getRawParameterValue (id))
        return value->load (std::memory_order_relaxed);
    return 0.0f;
}
}
OrganixMatterDNAView::OrganixMatterDNAView (OrganixAudioProcessor& processor)
    : audio (processor)
{
    constexpr const char* ids[] = {
        "dnaDepth", "dnaSensitivity", "dnaCrystalAttack",
        "dnaVaporAir", "dnaEnergyMotion", "dnaSpectralTone"
    };
    for (int i = 0; i < 6; ++i)
    {
        auto& c = controls[static_cast<std::size_t> (i)];
        c.knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        c.knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);
        c.knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        c.knob.setColour (juce::Slider::rotarySliderFillColourId,
                          i == 2 ? ui::accentFor (1) :
                          i == 3 ? ui::accentFor (3) : ui::accentFor (0));
        c.knob.setDoubleClickReturnValue (true, i == 2 || i == 3 ? 0.8 : 0.5);
        c.knob.setMouseDragSensitivity (180);
        c.name.setJustificationType (juce::Justification::centred);
        c.name.setFont (juce::Font (juce::FontOptions (9.0f)));
        c.name.setColour (juce::Label::textColourId, ui::secondary);
        c.name.setInterceptsMouseClicks (false, false);
        c.value.setJustificationType (juce::Justification::centred);
        c.value.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        c.value.setColour (juce::Label::textColourId, i == 2 ? ui::accentFor (1) :
                          i == 3 ? ui::accentFor (3) : ui::accentFor (0));
        c.value.setEditable (true, true, false);
        c.value.onTextChange = [&c]
        {
            c.knob.setValue (juce::jlimit (0.0, 1.0, c.value.getText().getDoubleValue() / 100.0),
                             juce::sendNotificationSync);
        };
        c.knob.onValueChange = [&c]
        {
            c.value.setText (juce::String (juce::roundToInt (c.knob.getValue() * 100.0)) + "%",
                             juce::dontSendNotification);
        };
        addAndMakeVisible (c.knob);
        addAndMakeVisible (c.name);
        addAndMakeVisible (c.value);
        c.knob.setComponentID (juce::String ("midi:") + ids[i]);
        c.link = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audio.parameters, ids[i], c.knob);
        c.knob.onValueChange();
    }
    enableButton.onClick = [this]
    {
        if (auto* param = audio.parameters.getParameter ("dnaEnable"))
        {
            const bool newValue = readParameter (audio, "dnaEnable") <= 0.5f;
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (newValue ? 1.0f : 0.0f));
            param->endChangeGesture();
        }
        refreshEnableState();
    };
    addAndMakeVisible (enableButton);
    enableButton.setComponentID ("midi:dnaEnable");
    setFrench (true);
    startTimerHz (30);
}
OrganixMatterDNAView::~OrganixMatterDNAView()
{
    stopTimer();
    for (auto& c : controls) c.link.reset();
}
void OrganixMatterDNAView::setFrench (bool value)
{
    french = value;
    constexpr const char* fr[] = {
        "PROFONDEUR", "SENSIBILITÉ", "ATTAQUE CRISTAL",
        "SOUFFLE VAPEUR", "ÉNERGIE / MVT", "SPECTRE / TON"
    };
    constexpr const char* en[] = {
        "DEPTH", "SENSITIVITY", "CRYSTAL ATTACK",
        "VAPOR AIR", "ENERGY / MOTION", "SPECTRUM / TONE"
    };
    for (int i = 0; i < 6; ++i)
    {
        auto& c = controls[static_cast<std::size_t> (i)];
        c.name.setText (juce::String::fromUTF8 (french ? fr[i] : en[i]), juce::dontSendNotification);
        c.knob.setTooltip (c.name.getText());
    }
    refreshEnableState();
    repaint();
}
void OrganixMatterDNAView::refreshEnableState()
{
    const bool nowEnabled = readParameter (audio, "dnaEnable") > 0.5f;
    if (enabled != nowEnabled)
    {
        enabled = nowEnabled;
        repaint();
    }
    enableButton.setButtonText (
        nowEnabled ? ui::word (french, "ADN ACTIF", "DNA ON")
                   : ui::word (french, "ACTIVER ADN", "ENABLE DNA"));
    enableButton.setColour (juce::TextButton::buttonColourId,
                            nowEnabled ? juce::Colour (0xff103b36) : ui::surface);
    enableButton.setColour (juce::TextButton::textColourOffId,
                            nowEnabled ? ui::engaged : ui::secondary);
}
void OrganixMatterDNAView::timerCallback()
{
    if (! isShowing()) return;
    refreshEnableState();
    const float raw[] = {
        audio.dnaEnergy.load (std::memory_order_relaxed),
        audio.dnaTransient.load (std::memory_order_relaxed),
        audio.dnaAir.load (std::memory_order_relaxed),
        audio.dnaCentroid.load (std::memory_order_relaxed),
        audio.dnaMotion.load (std::memory_order_relaxed)
    };
    for (int i = 0; i < 5; ++i)
    {
        const float target = juce::jlimit (0.0f, 1.0f, raw[i]);
        auto& value = meterSmooth[static_cast<std::size_t> (i)];
        value += (target - value) * (target > value ? 0.28f : 0.14f);
    }
    repaint();
}
void OrganixMatterDNAView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.6f);
    g.setColour (ui::panel);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    const int width = getWidth();
    const int meterX = juce::roundToInt (width * 0.605f);
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawFittedText ("MATTER DNA", {16, 10, 190, 20},
                      juce::Justification::centredLeft, 1);
    g.setColour (ui::secondary);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawFittedText (
        ui::word (french, "ANALYSE DU SIGNAL / MODULATION DES MATÉRIAUX",
                          "INPUT ANALYSIS / MATERIAL MODULATION"),
        {146, 10, juce::jmax (160, meterX - 365), 20},
        juce::Justification::centredLeft, 1, 0.8f);

    g.setColour (ui::line);
    g.drawLine (static_cast<float> (meterX - 9), 39.0f,
                static_cast<float> (meterX - 9), static_cast<float> (getHeight() - 13), 1.0f);
    g.setColour (ui::subdued);
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawFittedText (ui::word (french, "ANALYSE AUDIO EN DIRECT", "LIVE AUDIO ANALYSIS"),
                      {meterX + 10, 12, width - meterX - 154, 19},
                      juce::Justification::centredLeft, 1);

    constexpr const char* labelsFR[] = { "ÉNERGIE", "TRANSITOIRES", "AIR / SOUFFLE", "CENTROÏDE", "MOUVEMENT" };
    constexpr const char* labelsEN[] = { "ENERGY", "TRANSIENTS", "AIR / BREATH", "CENTROID", "MOTION" };
    for (int i = 0; i < 5; ++i)
    {
        const int y = 41 + i * 23;
        const auto colour = i == 1 ? ui::accentFor (1) :
                            i == 2 ? ui::accentFor (3) : ui::accentFor (0);
        g.setColour (ui::secondary);
        g.setFont (juce::Font (juce::FontOptions (9.5f)));
        g.drawFittedText (juce::String::fromUTF8 (french ? labelsFR[i] : labelsEN[i]),
                          {meterX + 10, y, 114, 19}, juce::Justification::centredLeft, 1);
        const int barX = meterX + 129;
        const int barW = juce::jmax (40, width - barX - 43);
        g.setColour (ui::inset);
        g.fillRoundedRectangle (static_cast<float> (barX), static_cast<float> (y + 6),
                                static_cast<float> (barW), 7.0f, 3.0f);
        g.setColour (colour.withAlpha (0.9f));
        g.fillRoundedRectangle (static_cast<float> (barX), static_cast<float> (y + 6),
                                static_cast<float> (barW) *
                                    meterSmooth[static_cast<std::size_t> (i)],
                                7.0f, 3.0f);
        g.setColour (colour);
        g.drawFittedText (juce::String (juce::roundToInt (
                            meterSmooth[static_cast<std::size_t> (i)] * 100.0f)),
                          {width - 41, y, 34, 19}, juce::Justification::centredRight, 1);
    }
}
void OrganixMatterDNAView::resized()
{
    const int meterX = juce::roundToInt (getWidth() * 0.605f);
    const int leftW = meterX - 21;
    const int perDial = juce::jmax (73, (leftW - 22) / 6);
    for (int i = 0; i < 6; ++i)
    {
        auto& c = controls[static_cast<std::size_t> (i)];
        const int x = 11 + i * perDial;
        const int diameter = juce::jmin (64, perDial - 8);
        c.knob.setBounds (x + (perDial - diameter) / 2, 42, diameter, diameter);
        c.name.setBounds (x + 1, 110, perDial - 2, 20);
        c.value.setBounds (x + 1, 134, perDial - 2, 19);
    }
    enableButton.setBounds (getWidth() - 131, 9, 119, 26);
}
