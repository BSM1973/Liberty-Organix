#pragma once

#include <JuceHeader.h>
#include "MaterialDSP.h"
#include "FactoryPresets.h"
#include "OrganixMovement.h"
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
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> inputRms {0.0f};
    std::atomic<float> outputRms {0.0f};
    std::atomic<float> outputPeak {0.0f};
    std::atomic<int> currentPreset {0};

    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

private:
    organix::MaterialDSP engineA;
    organix::MaterialDSP engineB;
    juce::LinearSmoothedValue<float> wetSmooth, bypassSmooth, outputSmooth;
    float lastMeterLevel = 0.0f;
    float movementPhase = 0.0f;
    float movementHeld = 0.0f;
    std::uint32_t movementRandom = 0x12ac57efu;
    static float raw (juce::AudioProcessorValueTreeState& state, const char* id) noexcept;
    void setParameter (const char* id, float plainValue);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixAudioProcessor)
};
