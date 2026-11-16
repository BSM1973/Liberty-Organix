#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrganixUITheme.h"
#include "OrganixI18n.h"
#include <memory>

class OrganixReactionPanel final : public juce::Component, private juce::Timer
{
public:
    explicit OrganixReactionPanel (OrganixAudioProcessor& processor);
    ~OrganixReactionPanel() override;
    void setFrench (bool value);
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    void timerCallback() override;
    void updateLabels();
    OrganixAudioProcessor& audio;
    bool french = true;
    int previousSignature = -1;
    juce::TextButton power { "ACTIVER" };
    juce::ComboBox direction;
    juce::Slider amount, response;
    juce::Label directionLabel, amountLabel, responseLabel;
    std::unique_ptr<SliderAttachment> amountLink, responseLink;
    std::unique_ptr<ComboAttachment> directionLink;
    float activityAB = 0.0f, activityBA = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixReactionPanel)
};
