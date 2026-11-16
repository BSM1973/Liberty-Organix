#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrganixUITheme.h"
#include "OrganixI18n.h"
#include <array>

class OrganixMorphPad final : public juce::Component, private juce::Timer
{
public:
    explicit OrganixMorphPad (OrganixAudioProcessor& processor);
    ~OrganixMorphPad() override;
    void setFrench (bool value) { french = value; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void changeXY (juce::Point<float> point);
    juce::Rectangle<float> field() const;
    OrganixAudioProcessor& audio;
    juce::RangedAudioParameter* xParam = nullptr;
    juce::RangedAudioParameter* yParam = nullptr;
    bool french = true, dragging = false;
    float visualX = 0.0f, visualY = 0.0f;
    std::array<juce::Point<float>, 20> trace {};
    int validTrace = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixMorphPad)
};

class OrganixMorphControls final : public juce::Component, private juce::Timer
{
public:
    explicit OrganixMorphControls (OrganixAudioProcessor& processor);
    ~OrganixMorphControls() override;
    void setFrench (bool value);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void refresh();
    OrganixAudioProcessor& audio;
    bool french = true, enabled = false;
    juce::TextButton enableButton { "MORPH OFF" };
    std::array<juce::TextButton,4> capture {{
        juce::TextButton ("CAPTURE A"), juce::TextButton ("CAPTURE B"),
        juce::TextButton ("CAPTURE C"), juce::TextButton ("CAPTURE D") }};
    std::array<juce::TextButton,4> listen {{
        juce::TextButton ("PLAY A"), juce::TextButton ("PLAY B"),
        juce::TextButton ("PLAY C"), juce::TextButton ("PLAY D") }};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixMorphControls)
};
