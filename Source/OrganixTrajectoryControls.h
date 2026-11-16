#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrganixI18n.h"
#include <functional>
#include <memory>

class OrganixTrajectoryControls final : public juce::Component, private juce::Timer
{
public:
    explicit OrganixTrajectoryControls (OrganixAudioProcessor& processor);
    ~OrganixTrajectoryControls() override;
    void setFrench (bool value);
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void(bool)> onDrawingToggled;

private:
    using ComboLink = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderLink = juce::AudioProcessorValueTreeState::SliderAttachment;
    void timerCallback() override;
    void refresh();
    void setToggle (const char* name, bool value);
    OrganixAudioProcessor& audio;
    bool french = true, drawing = false;
    juce::ComboBox shape, beat;
    juce::Slider speed, depth;
    juce::TextButton playback { "ACTIVER" }, freeSync { "LIBRE" },
                     direction { "AVANT" }, draw { "DESSINER" };
    juce::Label shapeTitle, beatTitle, speedTitle, depthTitle, status;
    std::unique_ptr<ComboLink> shapeLink, beatLink;
    std::unique_ptr<SliderLink> speedLink, depthLink;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixTrajectoryControls)
};
