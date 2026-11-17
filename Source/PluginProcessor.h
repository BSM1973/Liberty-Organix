#pragma once

#include <JuceHeader.h>
#include "MaterialDSP.h"
#include "FactoryPresets.h"
#include "OrganixMovement.h"
#include "MatterDNAModulation.h"
#include "OrganixMorph4D.h"
#include "OrganixTrajectory.h"
#include "MaterialReactions.h"
#include "OrganixModMatrix.h"
#include "OrganixSpectralView.h"
#include "OrganixSpectralTransient.h"
#include <array>
#include <cstdint>
#include <atomic>
#include <vector>

class OrganixAudioProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    OrganixAudioProcessor();
    ~OrganixAudioProcessor() override { stopTimer(); }
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
    void setTrajectoryPath (organix::TrajectoryPath path, bool persist = true);
    organix::TrajectoryPath getTrajectoryPath() const noexcept;
    std::atomic<float> trajectoryReadX {0.5f}, trajectoryReadY {0.5f};
    organix::MorphScene readMorphScene (int index) const noexcept;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> inputRms {0.0f};
    std::atomic<float> outputRms {0.0f};
    std::atomic<float> outputPeak {0.0f};
    // Dual FFT readouts are lock-free for the 30Hz central UI.
    organix::SpectralViewAnalyzer spectralView;
    std::atomic<float> audioCpuPercent {0.0f};
    // Values from the audio engine, observed by the reaction panel only.
    std::atomic<float> reactionActivityAB {0.0f}, reactionActivityBA {0.0f};
    std::array<std::atomic<float>,organix::MatrixSettings::slotCount> matrixSlotActivity {};
    // Read-only UI meters; audio callback is the sole writer.
    std::atomic<float> dnaEnergy {0.0f}, dnaTransient {0.0f};
    std::atomic<float> dnaAir {0.0f}, dnaMotion {0.0f};
    std::atomic<float> dnaCentroid {0.0f}, dnaLow {0.0f}, dnaMid {0.0f}, dnaHigh {0.0f};
    std::atomic<int> currentPreset {0};

    // MIDI CC learn. User interface and state serialization run on the JUCE
    // message thread. The audio callback only touches preallocated atomics.
    // A mapped CC from the learned MIDI channel controls one APVTS parameter.
    void startMidiLearn (const juce::String& parameterId) noexcept;
    void cancelMidiLearn() noexcept { midiLearnTarget.store (-1, std::memory_order_release); }
    bool isMidiLearning() const noexcept { return midiLearnTarget.load (std::memory_order_acquire) >= 0; }
    juce::String midiLearnTargetName() const;
    juce::String midiLearnMapping (const juce::String& parameterId) const;
    void clearMidiLearn (const juce::String& parameterId) noexcept;
    void clearAllMidiLearn() noexcept;
    // Deterministic message-thread test entry. Never invoke from processBlock().
    void flushMidiLearnUpdates();

    static constexpr int midiLearnCapacity = 512;
    static constexpr int midiCCSlots = 16 * 128;

    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

private:
    void timerCallback() override;
    void initialiseMidiLearn() noexcept;
    void handleMidiLearn (const juce::MidiBuffer& incoming) noexcept;
    juce::String serialiseMidiLearn() const;
    void restoreMidiLearn (const juce::String& data);
    int midiIndex (const juce::String& parameterId) const noexcept;
    std::array<std::atomic<int>,midiCCSlots> ccParameterIndex {};
    std::array<std::atomic<float>,midiLearnCapacity> midiValues {};
    std::array<std::atomic<bool>,midiLearnCapacity> midiDirty {};
    std::array<juce::RangedAudioParameter*,midiLearnCapacity> midiParameters {};
    std::array<juce::String,midiLearnCapacity> midiParameterIds {};
    int midiParameterCount = 0;
    std::atomic<int> midiLearnTarget {-1};

    organix::MaterialDSP engineA;
    organix::MaterialDSP engineB;
    organix::MaterialReactionLink reactionLink;
    organix::MatterDNA matterDNA;
    organix::MorphRender morphRender;
    organix::ModulationMatrix modMatrix;
    struct MatrixSlotPointers
    {
        std::atomic<float>* enabled=nullptr;
        std::atomic<float>* source=nullptr;
        std::atomic<float>* target=nullptr;
        std::atomic<float>* depth=nullptr;
        std::atomic<float>* bipolar=nullptr;
    };
    std::array<MatrixSlotPointers,organix::MatrixSettings::slotCount> matrixParams {};
    void initialiseMatrixParameterPointers();
    organix::MatrixSettings readMatrixSettings() const noexcept;
    juce::LinearSmoothedValue<float> morphBlendSmooth, morphXSmooth, morphYSmooth;
    struct SceneMailbox
    {
        std::atomic<uint32_t> version {0};
        std::array<std::atomic<float>, organix::MorphScene::packedCount> data {};
    };
    std::array<SceneMailbox,4> morphMailbox {};
    std::array<uint32_t,4> receivedVersions {};
    struct TrajectoryMailbox
    {
        std::atomic<uint32_t> version {0};
        std::atomic<int> count {0};
        std::array<std::atomic<float>, organix::TrajectoryPath::maxPoints> x {};
        std::array<std::atomic<float>, organix::TrajectoryPath::maxPoints> y {};
    };
    TrajectoryMailbox trajectoryMailbox;
    organix::TrajectoryPath audioTrajectory;
    uint32_t receivedTrajectoryVersion = 0;
    float trajectoryPhase = 0.0f, smoothTrajectoryX = 0.5f, smoothTrajectoryY = 0.5f;
    static juce::String encodeTrajectory (const organix::TrajectoryPath& path);
    static bool decodeTrajectory (const juce::String& string, organix::TrajectoryPath& path);
    void publishMorphScene (int index, organix::MorphScene scene,
                            bool persist);
    organix::MorphScene captureCurrentMorphScene() const noexcept;
    static juce::String encodeMorphScene (const organix::MorphScene& scene);
    static bool decodeMorphScene (const juce::String& text,
                                  organix::MorphScene& scene);
    juce::LinearSmoothedValue<float> wetSmooth, bypassSmooth, outputSmooth, widthSmooth;
    juce::LinearSmoothedValue<float> transientDepthSmooth;
    organix::SpectralTransientShaper transientShaper;
    float lastMeterLevel = 0.0f;
    float movementPhase = 0.0f;
    float movementHeld = 0.0f;
    std::uint32_t movementRandom = 0x12ac57efu;
    static float raw (juce::AudioProcessorValueTreeState& state, const char* id) noexcept;
    void setParameter (const char* id, float plainValue);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixAudioProcessor)
};
