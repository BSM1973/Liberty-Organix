#pragma once

#include <JuceHeader.h>
#include "MaterialDSP.h"
#include "FactoryPresets.h"
#include "OrganixMovement.h"
#include "MatterDNAModulation.h"
#include "OrganixMorph4D.h"
#include <array>
#include <cstdint>
#include <atomic>

class OrganixAudioProcessor final : public juce::AudioProcessor
{
public:
    OrganixAudioProcessor();
    ~OrganixAudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return static_cast<int> (organix::factoryPresets.size()); }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadFactoryPreset (int index);
    // Message-thread capture/recall. Audio reads only atomic published values.
    void captureMorphScene (int index);
    void goToMorphScene (int index);
    organix::MorphScene readMorphScene (int index) const noexcept;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> inputRms {0.0f};
    std::atomic<float> outputRms {0.0f};
    std::atomic<float> outputPeak {0.0f};
    // Read-only UI meters; audio callback is the sole writer.
    std::atomic<float> dnaEnergy {0.0f}, dnaTransient {0.0f};
    std::atomic<float> dnaAir {0.0f}, dnaMotion {0.0f};
    std::atomic<float> dnaCentroid {0.0f}, dnaLow {0.0f}, dnaMid {0.0f}, dnaHigh {0.0f};
    std::atomic<int> currentPreset {0};

    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

private:
    organix::MaterialDSP engineA;
    organix::MaterialDSP engineB;
    organix::MatterDNA matterDNA;
    organix::MorphRender morphRender;
    juce::LinearSmoothedValue<float> morphBlendSmooth, morphXSmooth, morphYSmooth;
    struct SceneMailbox
    {
        std::atomic<uint32_t> version {0};
        std::array<std::atomic<float>, organix::MorphScene::packedCount> data {};
    };
    std::array<SceneMailbox,4> morphMailbox {};
    std::array<uint32_t,4> receivedVersions {};
    void publishMorphScene (int index, organix::MorphScene scene,
                            bool persist);
    organix::MorphScene captureCurrentMorphScene() const noexcept;
    static juce::String encodeMorphScene (const organix::MorphScene& scene);
    static bool decodeMorphScene (const juce::String& text,
                                  organix::MorphScene& scene);
    juce::LinearSmoothedValue<float> wetSmooth, bypassSmooth, outputSmooth, widthSmooth;
    float lastMeterLevel = 0.0f;
    float movementPhase = 0.0f;
    float movementHeld = 0.0f;
    std::uint32_t movementRandom = 0x12ac57efu;
    static float raw (juce::AudioProcessorValueTreeState& state, const char* id) noexcept;
    void setParameter (const char* id, float plainValue);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixAudioProcessor)
};
