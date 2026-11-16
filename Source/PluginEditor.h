#pragma once
#include "PluginProcessor.h"
#include "OrganixUITheme.h"
#include <array>
#include <memory>

// Native JUCE editor, no browser view or static screenshot is used.
class MatterView final : public juce::Component, private juce::Timer
{
public:
    explicit MatterView (OrganixAudioProcessor&);
    ~MatterView() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setLayer (int layerIndex) { layer = juce::jlimit (0, 1, layerIndex); repaint(); }
    int getLayer() const noexcept { return layer; }

private:
    void timerCallback() override { repaint(); }
    void setXY (juce::Point<float> p);
    OrganixAudioProcessor& audio;
    int layer = 0;
    float clock = 0.0f;
    juce::RangedAudioParameter* gestureX = nullptr;
    juce::RangedAudioParameter* gestureY = nullptr;
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

    struct Dial
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void timerCallback() override;
    void setupDial (Dial&, const juce::String& id, const juce::String& title,
                    juce::Colour colour, bool percent = true);
    void bindProperties();
    void selectLayer (int next);
    void selectMaterial (int materialIndex);
    void updateMaterialState();
    void refreshPreset();
    void setAudioToggle (const char* parameterId, bool enabled);
    void layoutDial (Dial& dial, juce::Rectangle<int> area);
    int activeMaterial (int layerIndex) const;
    static void drawText (juce::Graphics&, const juce::String&, juce::Rectangle<int>,
                          float size, juce::Colour colour, bool bold = false,
                          juce::Justification = juce::Justification::centredLeft);

    OrganixAudioProcessor& audio;
    organix::ui::LookAndFeel lookAndFeel;
    MatterView visualizer;
    int selectedLayer = 0;
    bool macroPage = false;
    int lastPreset = -1;

    juce::ComboBox presets, routing;
    juce::TextButton previous { "<" }, next { ">" };
    juce::TextButton engaged { "ENGAGED" }, macroToggle { "MACROS" };
    juce::TextButton tabA { "LAYER A" }, tabB { "LAYER B" };
    juce::TextButton enableB { "B ON" };
    std::array<juce::TextButton, 6> materialButtons;
    std::unique_ptr<ComboAttachment> routingAttachment;
    std::array<Dial, 12> properties;
    std::array<Dial, 7> bottom;
    std::array<Dial, 8> macros;

    juce::Rectangle<int> headerBounds, leftBounds, centerBounds, rightBounds;
    juce::Rectangle<int> footerFusion, footerMotion, footerOutput;
    juce::Rectangle<int> footerAll;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixAudioProcessorEditor)
};
