#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrganixModMatrix.h"
#include "OrganixI18n.h"
#include "OrganixUITheme.h"
#include <array>
#include <functional>
#include <memory>

class OrganixModMatrixView final : public juce::Component, private juce::Timer
{
public:
    explicit OrganixModMatrixView (OrganixAudioProcessor&);
    ~OrganixModMatrixView() override;
    void setFrench (bool value);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void()> onClose;

private:
    using ComboAttachment=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    void timerCallback() override;
    void bindSlot (int index);
    void refreshState();
    void changeToggle (const juce::String& parameter, bool enabled);
    void applyDemo();
    float read (const juce::String& parameter) const noexcept;
    static juce::String slotStem (int index);
    static void styleLabel (juce::Label& label);
    static void styleCombo (juce::ComboBox& combo);
    static void styleSlider (juce::Slider& slider);
    static void drawText (juce::Graphics& g, const juce::String& string,
                          juce::Rectangle<int> rect, float size,
                          juce::Colour colour, bool strong=false,
                          juce::Justification alignment=juce::Justification::centredLeft);
    OrganixAudioProcessor& audio;
    bool french=true;
    int selected=0;
    juce::TextButton masterToggle {"MATRICE OFF"}, tempoToggle {"LIBRE"},
                     slotToggle {"LIAISON OFF"}, demoButton {"EXEMPLE"},
                     closeButton {"RETOUR"};
    juce::ComboBox source, target, polarity, division;
    juce::Label sourceLabel,targetLabel,polarityLabel,depthLabel,masterLabel,
                envelopeLabel;
    juce::Slider slotDepth, masterDepth, randomRate;
    std::array<juce::Slider,4> lfoRates;
    std::array<juce::Slider,4> envelopeSliders;
    std::unique_ptr<ComboAttachment> sourceLink,targetLink,polarityLink,divisionLink;
    std::unique_ptr<SliderAttachment> depthLink,masterLink,randomLink;
    std::array<std::unique_ptr<SliderAttachment>,4> lfoLinks,envLinks;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixModMatrixView)
};
