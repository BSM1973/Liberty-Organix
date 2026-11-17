#pragma once
#include "PluginProcessor.h"
#include "OrganixUITheme.h"
#include "OrganixMaterialButton.h"
#include "OrganixI18n.h"
#include "OrganixRoutingSelector.h"
#include "OrganixPresetBrowser.h"
#include "OrganixMatterDNAView.h"
#include "OrganixMorphView.h"
#include "OrganixTrajectoryControls.h"
#include "OrganixReactionPanel.h"
#include "OrganixModMatrixView.h"
#include <array>
#include <memory>

// Native JUCE editor, no browser view or static screenshot is used.
class MatterView final : public juce::Component, private juce::Timer
{
public:
    explicit MatterView (OrganixAudioProcessor&);
    ~MatterView() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setLayer (int layerIndex) { layer = juce::jlimit (0, 1, layerIndex); repaint(); }
    int getLayer() const noexcept { return layer; }
    void setFrench (bool value) { french = value; repaint(); }

private:
    void timerCallback() override;
    void setXY (juce::Point<float> p);
    OrganixAudioProcessor& audio;
    int layer = 0;
    bool french = true;
    float clock = 0.0f;
    float visualRms = 0.0f, visualPeak = 0.0f;
    float materialReveal = 1.0f;
    juce::TextButton spectrumMode { "AVANT / APRÈS" };
    juce::Slider transientControl;
    juce::Label transientLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transientAttachment;
    int priorMaterial = -1;
    juce::RangedAudioParameter* gestureX = nullptr;
    juce::RangedAudioParameter* gestureY = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatterView)
};

// Inner interface; the host-facing AudioProcessorEditor is OrganixViewportEditor.
class OrganixAudioProcessorEditor final : public juce::Component, private juce::Timer
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
        juce::Label label, value;
        bool percent = true;
        int unitMode = 0; // 0 percent, 1 dB, 2 Hz, 3 stereo width percent
        std::unique_ptr<SliderAttachment> attachment;
    };

    void timerCallback() override;
    void setupDial (Dial&, const juce::String& id, const juce::String& title,
                    juce::Colour colour, bool percent = true, int unitMode = -1);
    void bindProperties();
    void selectLayer (int next);
    void selectMaterial (int materialIndex);
    void updateMaterialState();
    void refreshPreset();
    void changeLanguage (bool french);
    void populateMovementChoices();
    void refreshMovementState();
    void openBrowser();
    void setAudioToggle (const char* parameterId, bool enabled);
    void layoutDial (Dial& dial, juce::Rectangle<int> area);
    int activeMaterial (int layerIndex) const;
    static void drawText (juce::Graphics&, const juce::String&, juce::Rectangle<int>,
                          float size, juce::Colour colour, bool bold = false,
                          juce::Justification = juce::Justification::centredLeft);

    OrganixAudioProcessor& audio;
    organix::ui::LookAndFeel lookAndFeel;
    MatterView visualizer;
    OrganixMatterDNAView dnaView;
    OrganixMorphPad morphPad;
    OrganixMorphControls morphControls;
    OrganixTrajectoryControls trajectoryControls;
    OrganixReactionPanel reactionPanel;
    OrganixModMatrixView matrixView;
    int selectedLayer = 0;
    bool macroPage = false;
    bool dnaPage = false;
    bool morphPage = false;
    bool reactionPage = false;
    bool matrixPage = false;
    bool french = true;
    int lastPreset = -1;
    int lastFactoryPreset = -1;
    juce::String currentPresetName;
    std::unique_ptr<juce::PropertiesFile> uiSettings;
    std::unique_ptr<OrganixPresetBrowser> browser;

    juce::TextButton languageFR { "FR" }, languageEN { "EN" };
    juce::ComboBox trajectorySelect, divisionSelect;
    juce::Label trajectoryLabel, divisionLabel;
    juce::TextButton motionFreeButton { "LIBRE" };
    std::unique_ptr<ComboAttachment> trajectoryAttachment, divisionAttachment;
    juce::TextButton presetDisplay { "PRESET" }, browserButton { "BROWSER" };
    juce::TextButton previous { "<" }, next { ">" };
    OrganixRoutingSelector routeSelector;
    juce::TextButton engaged { "ENGAGED" }, macroToggle { "MACROS" },
                     dnaToggle { "ADN" }, morphToggle { "MORPH" };
    juce::TextButton tabA { "LAYER A" }, tabB { "LAYER B" },
                     reactionToggle { "REACTIONS" }, matrixToggle { "MATRICE" };
    juce::TextButton enableB { "B ON" };
    std::array<organix::ui::MaterialButton, 6> materialButtons;
    std::array<Dial, 12> properties;
    std::array<Dial, 8> bottom;
    std::array<Dial, 8> macros;

    juce::Rectangle<int> headerBounds, leftBounds, centerBounds, rightBounds;
    juce::Rectangle<int> footerFusion, footerMotion, footerOutput;
    juce::Rectangle<int> footerAll;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixAudioProcessorEditor)
};
