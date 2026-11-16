#pragma once
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class MatterView final : public juce::Component, private juce::Timer
{
public:
    explicit MatterView (OrganixAudioProcessor&);
    ~MatterView() override = default;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    OrganixAudioProcessor& processor;
    float animation = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatterView)
};

class OrganixAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OrganixAudioProcessorEditor (OrganixAudioProcessor&);
    ~OrganixAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Dial
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };
    void timerCallback() override;
    void setupDial (Dial&, const char* parameter, const juce::String& label, juce::Colour accent);
    void setupCombo (juce::ComboBox&, const char* parameter);
    static void initLabel (juce::Label&, const juce::String&, float, juce::Colour, bool bold);

    OrganixAudioProcessor& audio;
    juce::LookAndFeel_V4 lookAndFeel;
    MatterView visualizer;
    juce::Label heading, tagline, sectionA, sectionB, macrosTitle, materialLabelA, materialLabelB;
    juce::Label status, meters;
    juce::ComboBox presetList, materialA, materialB, routing;
    juce::TextButton previous {"<"}, next {">"};
    juce::ToggleButton enableB {"MATERIAL B ON"}, bypass {"BYPASS"};
    std::unique_ptr<ComboAttachment> attachA, attachB, attachRoute;
    std::unique_ptr<ButtonAttachment> attachEnabled, attachBypass;
    std::array<Dial, 8> engines;
    std::array<Dial, 8> macros;
    std::array<Dial, 5> global;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixAudioProcessorEditor)
};
