#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour bg {0xff09111f};
const juce::Colour panel {0xff142334};
const juce::Colour border {0xff365068};
const juce::Colour text {0xffe2effa};
const juce::Colour muted {0xff9db6c9};
const juce::Colour cyan {0xff39d9d5};
const juce::Colour gold {0xfff0bf79};
const juce::Colour lavender {0xffb89efa};
const juce::Colour card {0xff101d2c};
const juce::Colour materialColours[] = {
    juce::Colour (0xff39d9d5), juce::Colour (0xffb5eaff),
    juce::Colour (0xffd888fb), juce::Colour (0xffa4c4ff),
    juce::Colour (0xffefc384), juce::Colour (0xffffe5ad)
};
}

MatterView::MatterView (OrganixAudioProcessor& p) : processor (p)
{
    setInterceptsMouseClicks (true, false);
    // Tooltip omitted: MatterView is a plain Component; JUCE tooltip support requires a dedicated client.
    startTimerHz (30); // rendering is entirely isolated from the audio callback
}

void MatterView::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (card);
    g.fillRoundedRectangle (r, 16.0f);
    g.setColour (border);
    g.drawRoundedRectangle (r.reduced (0.6f), 16.0f, 1.0f);
    const int id = juce::jlimit (0, 5, static_cast<int> (processor.parameters.getRawParameterValue ("materialA")->load()));
    const auto colour = materialColours[id];
    const float energy = juce::jlimit (0.0f, 1.0f, processor.outputRms.load (std::memory_order_relaxed) * 3.8f);
    const float peak = juce::jlimit (0.0f, 1.0f, processor.outputPeak.load (std::memory_order_relaxed));
    const float motion = processor.parameters.getRawParameterValue ("aMotion")->load();
    const float tone = processor.parameters.getRawParameterValue ("aTone")->load();
    animation += 0.013f + motion * 0.031f;
    if (animation > 100000.0f) animation = 0.0f;
    const float cx = r.getCentreX();
    const float cy = r.getCentreY() + 7.0f;
    const float radius = juce::jmin (r.getWidth(), r.getHeight()) * (0.17f + 0.15f * energy);

    // Procedural material contours are modulated by measured audio energy and DSP controls.
    for (int band = 0; band < 7; ++band)
    {
        juce::Path path;
        const float bandScale = 0.35f + 0.115f * band;
        for (int j = 0; j <= 150; ++j)
        {
            const float theta = juce::MathConstants<float>::twoPi * (static_cast<float> (j) / 150.0f);
            const float ripple = std::sin (theta * (3.0f + id) + animation * (1.0f + 0.13f * band));
            const float flutter = std::cos (theta * (5.0f + 4.0f * tone) - animation * 0.7f + band);
            const float rr = radius * bandScale * (1.0f + (0.06f + energy * 0.23f) * ripple + 0.07f * flutter);
            const float x = cx + rr * std::cos (theta) * 1.33f;
            const float y = cy + rr * std::sin (theta);
            if (j == 0) path.startNewSubPath (x, y);
            else path.lineTo (x, y);
        }
        g.setColour (colour.withAlpha (0.14f + 0.10f * band));
        g.strokePath (path, juce::PathStrokeType (0.7f + peak * 1.1f));
    }

    // Audio-reactive center; at silence the graphic is intentionally minimal.
    g.setColour (colour.withAlpha (0.12f + 0.32f * energy));
    g.fillEllipse (cx - 21.0f - 35.0f * energy, cy - 21.0f - 35.0f * energy,
                   42.0f + 70.0f * energy, 42.0f + 70.0f * energy);
    g.setColour (text.withAlpha (0.86f));
    g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    const char* names[] = {"LIQUID", "CRYSTAL", "PLASMA", "VAPOR", "METAL", "LIGHT"};
    g.drawFittedText (names[id], getLocalBounds().reduced (18).removeFromTop (32), juce::Justification::centredLeft, 1);
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawFittedText ("LIVING AUDIO MATTER / live DSP response", getLocalBounds().reduced (18).removeFromBottom (26), juce::Justification::centredLeft, 1);
}

void MatterView::mouseDown (const juce::MouseEvent& e) { mouseDrag (e); }
void MatterView::mouseDrag (const juce::MouseEvent& e)
{
    auto set = [&] (const char* id, float value)
    {
        if (auto* param = processor.parameters.getParameter (id))
            param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
    };
    set ("aMotion", static_cast<float> (e.position.x) / static_cast<float> (juce::jmax (1, getWidth())));
    set ("aTone", 1.0f - static_cast<float> (e.position.y) / static_cast<float> (juce::jmax (1, getHeight())));
}

void OrganixAudioProcessorEditor::initLabel (juce::Label& label, const juce::String& title, float size, juce::Colour colour, bool bold)
{
    label.setText (title, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, colour);
    label.setFont (juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)));
    label.setJustificationType (juce::Justification::centredLeft);
}

void OrganixAudioProcessorEditor::setupDial (Dial& dial, const char* parameter, const juce::String& title, juce::Colour accent)
{
    dial.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    dial.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 18);
    dial.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    dial.slider.setColour (juce::Slider::rotarySliderOutlineColourId, border);
    dial.slider.setColour (juce::Slider::textBoxTextColourId, text);
    dial.slider.setColour (juce::Slider::textBoxBackgroundColourId, card);
    dial.slider.setColour (juce::Slider::textBoxOutlineColourId, card);
    dial.label.setText (title, juce::dontSendNotification);
    dial.label.setColour (juce::Label::textColourId, muted);
    dial.label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    dial.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (dial.slider);
    addAndMakeVisible (dial.label);
    dial.attachment = std::make_unique<SliderAttachment> (audio.parameters, parameter, dial.slider);
}

void OrganixAudioProcessorEditor::setupCombo (juce::ComboBox& combo, const char* parameter)
{
    combo.setColour (juce::ComboBox::backgroundColourId, card);
    combo.setColour (juce::ComboBox::textColourId, text);
    combo.setColour (juce::ComboBox::outlineColourId, border);
    combo.setColour (juce::ComboBox::arrowColourId, cyan);
    const juce::StringArray materials {"LIQUID", "CRYSTAL", "PLASMA", "VAPOR", "METAL", "LIGHT"};
    const juce::StringArray routes {"PARALLEL", "SERIAL A > B", "SERIAL B > A", "CROSS-FUSION"};
    const auto& items = juce::String (parameter) == "routing" ? routes : materials;
    for (int i = 0; i < items.size(); ++i) combo.addItem (items[i], i + 1);
    addAndMakeVisible (combo);
}

OrganixAudioProcessorEditor::OrganixAudioProcessorEditor (OrganixAudioProcessor& p)
    : AudioProcessorEditor (&p), audio (p), visualizer (p)
{
    lookAndFeel.setColour (juce::PopupMenu::backgroundColourId, panel);
    lookAndFeel.setColour (juce::PopupMenu::textColourId, text);
    lookAndFeel.setColour (juce::TextButton::buttonColourId, panel);
    lookAndFeel.setColour (juce::TextButton::textColourOffId, text);
    lookAndFeel.setColour (juce::ToggleButton::textColourId, text);
    setLookAndFeel (&lookAndFeel);
    initLabel (heading, "ORGANIX", 31.0f, text, true);
    initLabel (tagline, "LIVING AUDIO MATTER  /  LIBERTY MUSIC SOFTWARE", 12.0f, cyan, true);
    initLabel (materialLabelA, "MATERIAL A", 12.0f, cyan, true);
    initLabel (materialLabelB, "MATERIAL B", 12.0f, gold, true);
    initLabel (sectionA, "MATERIAL A PROPERTIES", 13.0f, cyan, true);
    initLabel (sectionB, "MATERIAL B PROPERTIES", 13.0f, gold, true);
    initLabel (macrosTitle, "PERFORMANCE MACROS  /  GLOBAL DSP", 12.0f, text, true);
    initLabel (meters, "IN: -   OUT: -", 12.0f, muted, false);
    initLabel (status, "100 factory starting points / real-time engines", 11.0f, muted, false);
    for (auto* lbl : {&heading, &tagline, &materialLabelA, &materialLabelB, &sectionA, &sectionB, &macrosTitle, &meters, &status}) addAndMakeVisible (*lbl);
    setupCombo (materialA, "materialA");
    setupCombo (materialB, "materialB");
    setupCombo (routing, "routing");
    attachA = std::make_unique<ComboAttachment> (audio.parameters, "materialA", materialA);
    attachB = std::make_unique<ComboAttachment> (audio.parameters, "materialB", materialB);
    attachRoute = std::make_unique<ComboAttachment> (audio.parameters, "routing", routing);
    attachEnabled = std::make_unique<ButtonAttachment> (audio.parameters, "enableB", enableB);
    attachBypass = std::make_unique<ButtonAttachment> (audio.parameters, "bypass", bypass);
    for (auto* b : {&enableB, &bypass}) addAndMakeVisible (*b);
    addAndMakeVisible (visualizer);

    for (int i = 0; i < 100; ++i)
    {
        if (i % 10 == 0) presetList.addSectionHeading (organix::categoryName (organix::factoryPresets[static_cast<size_t> (i)].category));
        presetList.addItem (organix::factoryPresets[static_cast<size_t> (i)].name, i + 1);
    }
    presetList.setColour (juce::ComboBox::backgroundColourId, card);
    presetList.setColour (juce::ComboBox::textColourId, text);
    presetList.setColour (juce::ComboBox::outlineColourId, border);
    presetList.onChange = [this] { if (presetList.getSelectedId() > 0) audio.loadFactoryPreset (presetList.getSelectedId() - 1); };
    previous.onClick = [this] { presetList.setSelectedId (1 + (audio.currentPreset.load() + 99) % 100, juce::sendNotificationSync); };
    next.onClick = [this] { presetList.setSelectedId (1 + (audio.currentPreset.load() + 1) % 100, juce::sendNotificationSync); };
    presetList.setSelectedId (audio.currentPreset.load() + 1, juce::dontSendNotification);
    addAndMakeVisible (presetList); addAndMakeVisible (previous); addAndMakeVisible (next);

    constexpr const char* engineIds[] = {"aAmount", "aMotion", "aTone", "aFeedback", "bAmount", "bMotion", "bTone", "bFeedback"};
    constexpr const char* engineTitles[] = {"AMOUNT", "MOTION", "TONE", "FEEDBACK", "AMOUNT", "MOTION", "TONE", "FEEDBACK"};
    for (int i = 0; i < 8; ++i) setupDial (engines[static_cast<size_t> (i)], engineIds[i], engineTitles[i], i < 4 ? cyan : gold);
    constexpr const char* macroIds[] = {"macroTransform", "macroMotion", "macroEnergy", "macroDensity", "macroSpace", "macroHarmonics", "macroEvolution", "macroFusion"};
    constexpr const char* macroNames[] = {"TRANSFORM", "MOTION", "ENERGY", "DENSITY", "SPACE", "HARMONICS", "EVOLUTION", "FUSION"};
    for (int i = 0; i < 8; ++i) setupDial (macros[static_cast<size_t> (i)], macroIds[i], macroNames[i], i % 2 == 0 ? cyan : lavender);
    constexpr const char* globalIds[] = {"input", "output", "drywet", "fusion", "balance"};
    constexpr const char* globalNames[] = {"INPUT dB", "OUTPUT dB", "DRY / WET", "FUSION", "BALANCE"};
    for (int i = 0; i < 5; ++i) setupDial (global[static_cast<size_t> (i)], globalIds[i], globalNames[i], i < 2 ? gold : cyan);
    setSize (1160, 790);
    setResizable (true, true);
    setResizeLimits (980, 700, 1900, 1080);
    startTimerHz (10);
}

void OrganixAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    auto makePanel = [&] (juce::Rectangle<int> area)
    {
        g.setColour (panel); g.fillRoundedRectangle (area.toFloat(), 12.0f);
        g.setColour (border.withAlpha (0.65f)); g.drawRoundedRectangle (area.toFloat().reduced (0.5f), 12.0f, 1.0f);
    };
    auto content = getLocalBounds().reduced (12);
    auto header = content.removeFromTop (65);
    g.setColour (cyan.withAlpha (0.65f));
    g.drawLine (static_cast<float> (header.getX()), static_cast<float> (header.getBottom()), static_cast<float> (header.getRight()), static_cast<float> (header.getBottom()), 1.5f);
    content.removeFromTop (7);
    content.removeFromTop (95);
    auto footer = content.removeFromBottom (185);
    content.removeFromBottom (9);
    const int left = 219;
    const int right = 343;
    auto leftArea = content.removeFromLeft (left);
    content.removeFromLeft (9);
    auto rightArea = content.removeFromRight (right);
    content.removeFromRight (9);
    makePanel (leftArea);
    makePanel (rightArea);
    makePanel (footer);
}

void OrganixAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    auto header = area.removeFromTop (65);
    heading.setBounds (header.removeFromLeft (210));
    tagline.setBounds (header.removeFromLeft (450));
    bypass.setBounds (header.removeFromRight (105).reduced (5, 10));
    area.removeFromTop (8);
    auto toolbar = area.removeFromTop (90);
    auto presetArea = toolbar.removeFromLeft (400);
    auto topRow = presetArea.removeFromTop (46);
    previous.setBounds (topRow.removeFromLeft (34).reduced (2, 7));
    next.setBounds (topRow.removeFromLeft (34).reduced (2, 7));
    presetList.setBounds (topRow.reduced (5, 7));
    status.setBounds (presetArea.reduced (6, 2));
    const int globalWidth = juce::jmin (100, toolbar.getWidth() / 5);
    for (auto& dial : global)
    {
        auto box = toolbar.removeFromLeft (globalWidth);
        dial.slider.setBounds (box.withTrimmedBottom (17));
        dial.label.setBounds (box.removeFromBottom (17));
    }
    auto footer = area.removeFromBottom (185);
    area.removeFromBottom (9);
    auto left = area.removeFromLeft (219).reduced (14, 13);
    area.removeFromLeft (9);
    auto right = area.removeFromRight (343).reduced (12, 7);
    area.removeFromRight (9);
    visualizer.setBounds (area);

    materialLabelA.setBounds (left.removeFromTop (25));
    materialA.setBounds (left.removeFromTop (34));
    left.removeFromTop (8);
    materialLabelB.setBounds (left.removeFromTop (26));
    materialB.setBounds (left.removeFromTop (34));
    enableB.setBounds (left.removeFromTop (32));
    left.removeFromTop (7);
    routing.setBounds (left.removeFromTop (34));
    left.removeFromTop (9);
    meters.setBounds (left.removeFromTop (28));

    const int halfH = right.getHeight() / 2;
    for (int group = 0; group < 2; ++group)
    {
        auto r = right.removeFromTop (halfH);
        (group == 0 ? sectionA : sectionB).setBounds (r.removeFromTop (26));
        const int rowH = r.getHeight() / 2;
        for (int row = 0; row < 2; ++row)
        {
            auto rowArea = r.removeFromTop (rowH);
            for (int col = 0; col < 2; ++col)
            {
                auto box = rowArea.removeFromLeft (rowArea.getWidth() / (2 - col));
                auto& dial = engines[static_cast<size_t> (group * 4 + row * 2 + col)];
                dial.label.setBounds (box.removeFromBottom (18));
                dial.slider.setBounds (box);
            }
        }
    }
    macrosTitle.setBounds (footer.removeFromTop (27).reduced (14, 2));
    footer.reduce (12, 3);
    for (int i = 0; i < 8; ++i)
    {
        auto box = footer.removeFromLeft (footer.getWidth() / (8 - i));
        auto& dial = macros[static_cast<size_t> (i)];
        dial.label.setBounds (box.removeFromBottom (18));
        dial.slider.setBounds (box);
    }
}

void OrganixAudioProcessorEditor::timerCallback()
{
    const auto db = [] (float value) { return juce::Decibels::gainToDecibels (juce::jmax (0.000001f, value)); };
    meters.setText (juce::String::formatted ("IN %+.1f dB  /  OUT %+.1f dB  /  PK %+.1f dB",
                      db (audio.inputRms.load()), db (audio.outputRms.load()), db (audio.outputPeak.load())), juce::dontSendNotification);
    const int idx = audio.currentPreset.load();
    if (idx >= 0 && idx < 100 && presetList.getSelectedId() != idx + 1)
        presetList.setSelectedId (idx + 1, juce::dontSendNotification);
}

OrganixAudioProcessorEditor::~OrganixAudioProcessorEditor() { setLookAndFeel (nullptr); }
