#pragma once
#include "PluginEditor.h"
#include "OrganixViewport.h"

// Real host-facing editor. The existing Organix JUCE interface is a child,
// uniformly scaled as a whole (including clickable components and overlays).
// No scroll bar, no global JUCE scale change, no display-dependent DSP state.
class OrganixViewportEditor final : public juce::AudioProcessorEditor
{
public:
    explicit OrganixViewportEditor (OrganixAudioProcessor& processor)
        : juce::AudioProcessorEditor (&processor), content (processor)
    {
        setOpaque (true);
        setResizable (false, false);

        // userArea already excludes the operating system's menu bar/dock.
        // Reserve extra room for the DAW's own floating plugin title bar.
        const auto* display =
            juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        const auto userArea = display != nullptr
            ? display->userArea
            : juce::Rectangle<int> (0, 0, 1280, 800);
        const auto viewport = organix::layout::chooseViewport (
            userArea.getWidth(), userArea.getHeight());

        addAndMakeVisible (content);
        content.setBounds (0, 0, viewport.contentWidth, viewport.contentHeight);
        content.setTransform (juce::AffineTransform::scale (viewport.scale));

        // The host receives actual scaled dimensions, not the oversized
        // logical design bounds. Never impose a scrollbar or resize grip.
        setSize (viewport.windowWidth, viewport.windowHeight);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (organix::ui::background);
    }

    void resized() override
    {
        content.setTopLeftPosition (0, 0);
    }

private:
    OrganixAudioProcessorEditor content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixViewportEditor)
};
