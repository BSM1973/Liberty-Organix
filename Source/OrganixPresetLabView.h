#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrganixPresetLabCore.h"
#include "OrganixUITheme.h"
#include "OrganixI18n.h"
#include <array>
#include <functional>
#include <vector>

class OrganixPresetLabView final : public juce::Component, private juce::Timer
{
public:
    explicit OrganixPresetLabView(OrganixAudioProcessor& processor);
    ~OrganixPresetLabView() override;
    void setFrench(bool value);
    void paint(juce::Graphics&) override;
    void resized() override;
    void prepareOpen();
    void syncExternalChange(); // called on direct factory/user preset loads
    std::function<void()> onClose;

private:
    using Snapshot=juce::MemoryBlock;
    static bool same(const Snapshot&,const Snapshot&) noexcept;
    Snapshot capture() const;
    void restore(const Snapshot&);
    void pushUndo(const Snapshot&);
    void flushPending();
    void timerCallback() override;
    void performUndo();
    void performRedo();
    void toggleAB();
    void captureAB(int slot);
    void captureVariation(int index);
    void recallVariation(int index);
    void generateVariation();
    void refreshLabels();

    OrganixAudioProcessor& audio;
    bool french=true,activeA=true,restoring=false,pending=false;
    unsigned locks=0;
    unsigned nextSeed=0x19071973u;
    juce::uint32 lastChangeTime=0;
    Snapshot tracked, pendingState, snapshotA, snapshotB;
    std::array<Snapshot,4> variations;
    std::vector<Snapshot> undo,redo;

    juce::TextButton closeButton,abButton,storeA,storeB,undoButton,redoButton,
                     mutateButton;
    std::array<juce::TextButton,5> lockButtons;
    std::array<juce::TextButton,4> storeVariation,recallVariationButtons;
    juce::Slider intensity;
    juce::Label status;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganixPresetLabView)
};
