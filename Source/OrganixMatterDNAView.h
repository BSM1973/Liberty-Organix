#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrganixUITheme.h"
#include "OrganixI18n.h"
#include <array>
#include <memory>

// Real, bilingual Matter DNA laboratory. This component only reads
// atomic analysis meters and uses APVTS attachments for all controls.
class OrganixMatterDNAView final : public juce::Component, private juce::Timer
{
public:
    explicit OrganixMatterDNAView (OrganixAudioProcessor&);
    ~OrganixMatterDNAView() override;
    void setFrench (bool isFrench);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Control
    {
        juce::Slider knob;
        juce::Label name, value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> link;
    };
    void timerCallback() override;
    void refreshEnableState();
    OrganixAudioProcessor& audio;
    bool french = true, enabled = false;
    juce::TextButton enableButton { "ADN OFF" };
    std::array<Control, 6> controls;
    std::array<float, 5> meterSmooth {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixMatterDNAView)
};
