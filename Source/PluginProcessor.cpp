#include "PluginProcessor.h"
#include "FactorySoundDesigns.h"
#include "OrganixViewportEditor.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace
{
float clamp01 (float x) { return juce::jlimit (0.0f, 1.0f, x); }
}

OrganixAudioProcessor::OrganixAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "ORGANIX_STATE", makeLayout())
{
    initialiseMatrixParameterPointers();
    initialiseMidiLearn();
    loadFactoryPreset (0);
    // The processor owns MIDI CC dispatch independently of the editor.
    startTimerHz (60);
}

juce::AudioProcessorValueTreeState::ParameterLayout OrganixAudioProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using Choice = juce::AudioParameterChoice;
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    auto pid = [] (const char* name) { return juce::ParameterID (name, 1); };
    layout.add (std::make_unique<Choice> (pid ("materialA"), "Material A", juce::StringArray {"LIQUID", "CRYSTAL", "PLASMA", "VAPOR", "METAL", "LIGHT"}, 0));
    layout.add (std::make_unique<Choice> (pid ("materialB"), "Material B", juce::StringArray {"LIQUID", "CRYSTAL", "PLASMA", "VAPOR", "METAL", "LIGHT"}, 1));
    layout.add (std::make_unique<Bool> (pid ("enableB"), "Enable Material B", true));
    layout.add (std::make_unique<Choice> (pid ("routing"), "Fusion Routing", juce::StringArray {"PARALLEL", "SERIAL A > B", "SERIAL B > A", "CROSS-FUSION"}, 0));
    layout.add (std::make_unique<Bool> (pid ("bypass"), "Global Bypass", false));
    auto add = [&] (const char* id, const char* name, float low, float high, float def) {
        layout.add (std::make_unique<Float> (pid (id), name, juce::NormalisableRange<float> (low, high, 0.001f), def));
    };
    add ("input", "Input Gain dB", -24.0f, 24.0f, 0.0f);
    add ("output", "Output Gain dB", -24.0f, 12.0f, 0.0f);
    add ("drywet", "Dry Wet", 0.0f, 1.0f, 0.65f);
    layout.add (std::make_unique<Choice> (
        pid ("spectralCompare"), "Spectral Display Input Output or Overlay",
        juce::StringArray {"INPUT", "OUTPUT", "BEFORE/AFTER"}, 2));
    add ("spectralTransient", "Spectral Transient Shape", -1.0f, 1.0f, 0.0f);
    // Cross-material modulation is opt-in for old projects and factory patches.
    layout.add (std::make_unique<Bool> (pid ("reactionEnable"), "Material Reactions Enabled", false));
    add ("reactionAmount", "Material Reactions Amount", 0.0f, 1.0f, 0.60f);
    layout.add (std::make_unique<Choice> (
        pid ("reactionDirection"), "Material Reaction Direction",
        juce::StringArray {"A TO B", "B TO A", "MUTUAL"}, 2));
    add ("reactionResponse", "Material Reactions Response", 0.0f, 1.0f, 0.40f);
    layout.add (std::make_unique<Bool> (pid ("morphEnable"), "Four Scene Morph Enabled", false));
    add ("morphX", "Four Scene Morph X", 0.0f, 1.0f, 0.0f);
    add ("morphY", "Four Scene Morph Y", 0.0f, 1.0f, 0.0f);
    // Standalone and DAW-recallable trajectory design, default disabled.
    layout.add (std::make_unique<Bool> (pid ("trajectoryEnable"), "Trajectory Playback", false));
    layout.add (std::make_unique<Choice> (pid ("trajectoryShape"),
        "Trajectory Shape", juce::StringArray {"CIRCLE", "SPIRAL", "ZIGZAG", "FREEHAND"}, 0));
    add ("trajectorySpeed", "Trajectory Speed", 0.05f, 6.0f, 0.25f);
    layout.add (std::make_unique<Bool> (pid ("trajectoryFree"), "Trajectory Free Rate", true));
    layout.add (std::make_unique<Choice> (pid ("trajectoryDivision"),
        "Trajectory Beat Division",
        juce::StringArray {"1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T"}, 2));
    layout.add (std::make_unique<Bool> (pid ("trajectoryReverse"), "Trajectory Reverse", false));
    add ("trajectoryDepth", "Trajectory XY Depth", 0.0f, 1.0f, 1.0f);
    // Professional 32-slot modulation matrix: 5 host-automatable controls
    // for EVERY slot (on, source, destination, signed depth, polarity).
    layout.add (std::make_unique<Bool> (pid ("matrixEnable"),
                                        "Modulation Matrix Enabled", false));
    add ("matrixMasterDepth", "Modulation Master Depth", 0.0f, 1.0f, 0.8f);
    layout.add (std::make_unique<Bool> (pid ("matrixTempoSync"),
                                        "Matrix LFO Tempo Sync", false));
    layout.add (std::make_unique<Choice> (
        pid ("matrixBeatDivision"), "Matrix LFO Beat Division",
        juce::StringArray {"1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T"}, 2));
    for (int i=0; i<4; ++i)
    {
        const auto id = juce::String ("matrixLfo") + juce::String (i+1) + "Rate";
        layout.add (std::make_unique<Float> (
            juce::ParameterID (id, 1),
            "Matrix LFO " + juce::String (i+1) + " Hz/Beat Multiplier",
            juce::NormalisableRange<float> (0.02f,12.0f,0.001f),
            i==0 ? 0.17f : i==1 ? 0.31f : i==2 ? 0.5f : 0.9f));
    }
    for (int i=0; i<2; ++i)
    {
        const auto prefix = juce::String ("matrixEnv") + juce::String(i+1);
        layout.add (std::make_unique<Float> (
            juce::ParameterID (prefix+"Attack",1),
            "Matrix Envelope " + juce::String(i+1) + " Attack (s)",
            juce::NormalisableRange<float>(0.001f,0.5f,0.001f),
            i==0 ? 0.015f : 0.060f));
        layout.add (std::make_unique<Float> (
            juce::ParameterID (prefix+"Decay",1),
            "Matrix Envelope " + juce::String(i+1) + " Decay (s)",
            juce::NormalisableRange<float>(0.005f,3.0f,0.001f),
            i==0 ? 0.32f : 0.8f));
    }
    add ("matrixRandomRate", "Matrix Random Rate", 0.05f,10.0f,1.0f);
    juce::StringArray matrixSources, matrixDestinations;
    for (int i=0; i<organix::MatrixSettings::sourceCount; ++i)
        matrixSources.add (juce::String::fromUTF8(
            organix::ModulationMatrix::sourceName (i,false)));
    for (int i=0; i<organix::MatrixSettings::destinationCount; ++i)
        matrixDestinations.add (juce::String::fromUTF8(
            organix::ModulationMatrix::destinationName (i,false)));
    for (int i=1; i<=organix::MatrixSettings::slotCount; ++i)
    {
        const auto slotName = juce::String ("MM ") + juce::String(i).paddedLeft('0',2);
        const auto stem = juce::String ("mm") + juce::String(i).paddedLeft('0',2);
        layout.add (std::make_unique<Bool> (
            juce::ParameterID(stem+"Enable",1),slotName+" Enabled",false));
        layout.add (std::make_unique<Choice> (
            juce::ParameterID(stem+"Source",1),slotName+" Source",matrixSources,0));
        layout.add (std::make_unique<Choice> (
            juce::ParameterID(stem+"Target",1),slotName+" Destination",matrixDestinations,0));
        layout.add (std::make_unique<Float> (
            juce::ParameterID(stem+"Depth",1),slotName+" Depth",
            juce::NormalisableRange<float>(-1.0f,1.0f,0.001f),0.0f));
        layout.add (std::make_unique<Choice> (
            juce::ParameterID(stem+"Polarity",1),slotName+" Polarity",
            juce::StringArray {"UNIPOLAR","BIPOLAR"},0));
    }
    // Matter DNA: analysis is opt-in to preserve all v0.5 sessions.
    layout.add (std::make_unique<Bool> (pid ("dnaEnable"), "Enable Matter DNA", false));
    add ("dnaDepth", "Matter DNA Depth", 0.0f, 1.0f, 0.50f);
    add ("dnaSensitivity", "Matter DNA Sensitivity", 0.0f, 1.0f, 0.50f);
    add ("dnaCrystalAttack", "DNA Transients to Crystal", 0.0f, 1.0f, 0.80f);
    add ("dnaVaporAir", "DNA Breath to Vapor", 0.0f, 1.0f, 0.80f);
    add ("dnaEnergyMotion", "DNA Energy to Motion", 0.0f, 1.0f, 0.40f);
    add ("dnaSpectralTone", "DNA Spectrum to Tone", 0.0f, 1.0f, 0.25f);
    // Four real controls absent from the earlier reference UI.
    add ("width", "Stereo Width", 0.0f, 2.0f, 1.0f);
    layout.add (std::make_unique<Bool> (pid ("motionFree"), "Motion Free Running", true));
    add ("motionRate", "Motion Rate Hz", 0.05f, 12.0f, 0.25f);
    layout.add (std::make_unique<Choice> (
        pid ("motionTrajectory"), "Movement Trajectory",
        juce::StringArray {"MANUAL", "SINE", "TRIANGLE", "PULSE", "RANDOM HOLD"}, 0));
    layout.add (std::make_unique<Choice> (
        pid ("motionDivision"), "Motion Beat Division",
        juce::StringArray {"1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T"}, 2));
    add ("aAmount", "Material A Amount", 0.0f, 1.0f, 0.55f);
    add ("aMotion", "Material A Motion", 0.0f, 1.0f, 0.45f);
    add ("aTone", "Material A Tone", 0.0f, 1.0f, 0.50f);
    add ("aFeedback", "Material A Feedback", 0.0f, 0.90f, 0.2f);
    add ("bAmount", "Material B Amount", 0.0f, 1.0f, 0.55f);
    add ("bMotion", "Material B Motion", 0.0f, 1.0f, 0.45f);
    add ("bTone", "Material B Tone", 0.0f, 1.0f, 0.50f);
    add ("bFeedback", "Material B Feedback", 0.0f, 0.90f, 0.2f);
    // 12 editable, persistent material controls per layer, independent of the selected engine.
    // The engine maps these normalized values to its material-specific characteristics.
    for (int i = 1; i <= 12; ++i)
    {
        const auto suffix = juce::String (i).paddedLeft ('0', 2);
        for (const auto* layer : { "aShape", "bShape" })
        {
            const auto id = juce::String (layer) + suffix;
            const auto prefix = juce::String (layer)[0] == 'a' ? "Material A Property " : "Material B Property ";
            layout.add (std::make_unique<Float> (juce::ParameterID (id, 1),
                                                  juce::String (prefix) + suffix,
                                                  juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
                                                  0.5f));
        }
    }
    add ("fusion", "Cross Fusion", 0.0f, 1.0f, 0.3f);
    add ("balance", "Material Balance", 0.0f, 1.0f, 0.5f);
    constexpr const char* ids[] = {"macroTransform", "macroMotion", "macroEnergy", "macroDensity", "macroSpace", "macroHarmonics", "macroEvolution", "macroFusion"};
    constexpr const char* names[] = {"Transform", "Motion", "Energy", "Density", "Space", "Harmonics", "Evolution", "Fusion"};
    for (int i = 0; i < 8; ++i) add (ids[i], names[i], 0.0f, 1.0f, 0.5f);
    return layout;
}

void OrganixAudioProcessor::initialiseMatrixParameterPointers()
{
    for (int i=0; i<organix::MatrixSettings::slotCount; ++i)
    {
        const auto stem=juce::String("mm")+juce::String(i+1).paddedLeft('0',2);
        auto& h=matrixParams[static_cast<size_t>(i)];
        h.enabled=parameters.getRawParameterValue(stem+"Enable");
        h.source=parameters.getRawParameterValue(stem+"Source");
        h.target=parameters.getRawParameterValue(stem+"Target");
        h.depth=parameters.getRawParameterValue(stem+"Depth");
        h.bipolar=parameters.getRawParameterValue(stem+"Polarity");
    }
}
organix::MatrixSettings OrganixAudioProcessor::readMatrixSettings() const noexcept
{
    organix::MatrixSettings s;
    auto read = [this] (const char* id) noexcept -> float
    {
        if (auto* p=parameters.getRawParameterValue(id))
            return p->load(std::memory_order_relaxed);
        return 0.0f;
    };
    s.enabled=read("matrixEnable")>0.5f;
    if (!s.enabled) return s;
    s.masterDepth=read("matrixMasterDepth");
    s.tempoSync=read("matrixTempoSync")>0.5f;
    s.division=juce::jlimit(0,6,static_cast<int>(read("matrixBeatDivision")));
    s.rates[0]=read("matrixLfo1Rate");
    s.rates[1]=read("matrixLfo2Rate");
    s.rates[2]=read("matrixLfo3Rate");
    s.rates[3]=read("matrixLfo4Rate");
    s.envAttack[0]=read("matrixEnv1Attack");
    s.envAttack[1]=read("matrixEnv2Attack");
    s.envDecay[0]=read("matrixEnv1Decay");
    s.envDecay[1]=read("matrixEnv2Decay");
    s.randomRate=read("matrixRandomRate");
    for (int i=0;i<organix::MatrixSettings::slotCount;++i)
    {
        const auto& h=matrixParams[static_cast<size_t>(i)];
        auto& v=s.slots[static_cast<size_t>(i)];
        v.enabled=h.enabled!=nullptr && h.enabled->load(std::memory_order_relaxed)>0.5f;
        if(!v.enabled) continue;
        v.source=h.source!=nullptr?static_cast<int>(h.source->load(std::memory_order_relaxed)):0;
        v.destination=h.target!=nullptr?static_cast<int>(h.target->load(std::memory_order_relaxed)):0;
        v.depth=h.depth!=nullptr?h.depth->load(std::memory_order_relaxed):0.0f;
        v.bipolar=h.bipolar!=nullptr&&h.bipolar->load(std::memory_order_relaxed)>0.5f;
    }
    return s;
}

void OrganixAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    engineA.prepare (sampleRate, getTotalNumInputChannels());
    engineB.prepare (sampleRate, getTotalNumInputChannels());
    matterDNA.prepare (sampleRate);
    spectralView.prepare (sampleRate);
    audioCpuPercent.store (0.0f,std::memory_order_relaxed);
    transientShaper.prepare(sampleRate);
    transientDepthSmooth.reset (sampleRate,0.025);
    transientDepthSmooth.setCurrentAndTargetValue (
        raw(parameters,"spectralTransient"));
    modMatrix.prepare (sampleRate);
    morphRender.prepare (sampleRate, getTotalNumInputChannels());
    reactionLink.prepare (sampleRate);
    reactionActivityAB.store (0.0f, std::memory_order_relaxed);
    reactionActivityBA.store (0.0f, std::memory_order_relaxed);
    receivedTrajectoryVersion = 0;
    trajectoryPhase = 0.0f;
    smoothTrajectoryX = 0.5f;
    smoothTrajectoryY = 0.5f;
    receivedVersions.fill (0);
    morphBlendSmooth.reset (sampleRate, 0.085);
    morphXSmooth.reset (sampleRate, 0.080);
    morphYSmooth.reset (sampleRate, 0.080);
    morphBlendSmooth.setCurrentAndTargetValue (0.0f);
    morphXSmooth.setCurrentAndTargetValue (raw (parameters, "morphX"));
    morphYSmooth.setCurrentAndTargetValue (raw (parameters, "morphY"));
    wetSmooth.reset (sampleRate, 0.025);
    bypassSmooth.reset (sampleRate, 0.008);
    outputSmooth.reset (sampleRate, 0.025);
    widthSmooth.reset (sampleRate, 0.020);
    movementPhase = 0.0f;
    movementHeld = 0.0f;
    movementRandom = 0x12ac57efu;
    wetSmooth.setCurrentAndTargetValue (raw (parameters, "drywet"));
    bypassSmooth.setCurrentAndTargetValue (raw (parameters, "bypass") > 0.5f ? 0.0f : 1.0f);
    outputSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw (parameters, "output")));
    widthSmooth.setCurrentAndTargetValue (raw (parameters, "width"));
    setLatencySamples (0); // Current time-domain engines require no fixed algorithmic delay.
}

void OrganixAudioProcessor::releaseResources()
{
    engineA.reset();
    engineB.reset();
    matterDNA.reset();
    spectralView.reset();
    transientShaper.reset();
    audioCpuPercent.store (0.0f,std::memory_order_relaxed);
    modMatrix.reset();
    morphRender.reset();
    reactionLink.reset();
    trajectoryPhase = 0.0f;
    smoothTrajectoryX = smoothTrajectoryY = 0.5f;
}

bool OrganixAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto in = layouts.getMainInputChannelSet();
    auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == out;
}

float OrganixAudioProcessor::raw (juce::AudioProcessorValueTreeState& state, const char* id) noexcept
{
    if (auto* value = state.getRawParameterValue (id)) return value->load();
    return 0.0f;
}

void OrganixAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    handleMidiLearn (midi);
    const auto channels = std::min (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (channels <= 0 || n <= 0) return;
    const auto cpuStart=std::chrono::steady_clock::now();
    // Analyze the true input before any in-place edits by matrix, DNA or DSP.
    spectralView.analyze (organix::SpectralViewAnalyzer::Stream::Before,
                          buffer.getReadPointer(0),
                          channels>1?buffer.getReadPointer(1):nullptr,n,channels);
    transientDepthSmooth.setTargetValue (raw(parameters,"spectralTransient"));
    const auto matrixSettings=readMatrixSettings();
    const bool matrixOn=matrixSettings.enabled;

    const bool activeB = raw (parameters, "enableB") > 0.5f;
    organix::ReactionControls reactions;
    reactions.enabled = raw (parameters, "reactionEnable") > 0.5f;
    reactions.amount = raw (parameters, "reactionAmount");
    reactions.direction = juce::jlimit (0, 2,
        static_cast<int> (raw (parameters, "reactionDirection")));
    reactions.response = raw (parameters, "reactionResponse");
    const int routing = juce::jlimit (0, 3, static_cast<int> (raw (parameters, "routing")));
    const auto aMaterial = static_cast<organix::Material> (juce::jlimit (0, 5, static_cast<int> (raw (parameters, "materialA"))));
    const auto bMaterial = static_cast<organix::Material> (juce::jlimit (0, 5, static_cast<int> (raw (parameters, "materialB"))));
    const float transform = raw (parameters, "macroTransform") - 0.5f;
    const float motion = raw (parameters, "macroMotion") - 0.5f;
    const float energy = raw (parameters, "macroEnergy");
    const float density = raw (parameters, "macroDensity");
    const float space = raw (parameters, "macroSpace") - 0.5f;
    const float harmonics = raw (parameters, "macroHarmonics") - 0.5f;
    const float evolution = raw (parameters, "macroEvolution") - 0.5f;
    const float fusion = clamp01 (raw (parameters, "fusion") + (raw (parameters, "macroFusion") - 0.5f) * 0.6f);
    const float balance = raw (parameters, "balance");
    organix::Parameters a, b;
    a.amount = clamp01 (raw (parameters, "aAmount") + transform * 0.5f);
    a.motion = clamp01 (raw (parameters, "aMotion") + motion * 0.5f + evolution * 0.3f);
    a.tone = clamp01 (raw (parameters, "aTone") + harmonics * 0.6f);
    a.feedback = juce::jlimit (0.0f, 0.88f, raw (parameters, "aFeedback") + space * 0.25f);
    a.energy = energy;
    a.density = density;
    b.amount = clamp01 (raw (parameters, "bAmount") + transform * 0.5f);
    b.motion = clamp01 (raw (parameters, "bMotion") + motion * 0.5f - evolution * 0.3f);
    b.tone = clamp01 (raw (parameters, "bTone") + harmonics * 0.6f);
    b.feedback = juce::jlimit (0.0f, 0.88f, raw (parameters, "bFeedback") + space * 0.25f);
    b.energy = energy;
    b.density = density;
    organix::MatterDNA::Features dnaFeatures;
    const bool dnaIsEnabled = raw (parameters, "dnaEnable") > 0.5f;
    if (dnaIsEnabled || matrixOn)
    {
        const auto* dnaLeft = buffer.getReadPointer (0);
        const auto* dnaRight = channels == 2 ? buffer.getReadPointer (1) : nullptr;
        dnaFeatures = matterDNA.analyzeBlock (dnaLeft, dnaRight, n, channels);
    }
    const auto meter = [] (std::atomic<float>& dst, float value) noexcept
    {
        dst.store (value, std::memory_order_relaxed);
    };
    meter (dnaEnergy, dnaFeatures.energy);
    meter (dnaTransient, dnaFeatures.transient);
    meter (dnaAir, dnaFeatures.air);
    meter (dnaMotion, dnaFeatures.motion);
    meter (dnaCentroid, dnaFeatures.centroid);
    meter (dnaLow, dnaFeatures.low);
    meter (dnaMid, dnaFeatures.mid);
    meter (dnaHigh, dnaFeatures.high);
    constexpr const char* shapeIdsA[] = {
        "aShape01", "aShape02", "aShape03", "aShape04",
        "aShape05", "aShape06", "aShape07", "aShape08",
        "aShape09", "aShape10", "aShape11", "aShape12"
    };
    constexpr const char* shapeIdsB[] = {
        "bShape01", "bShape02", "bShape03", "bShape04",
        "bShape05", "bShape06", "bShape07", "bShape08",
        "bShape09", "bShape10", "bShape11", "bShape12"
    };
    for (size_t i = 0; i < a.shape.size(); ++i)
    {
        a.shape[i] = raw (parameters, shapeIdsA[i]);
        b.shape[i] = raw (parameters, shapeIdsB[i]);
    }
    if (dnaIsEnabled)
    {
        organix::MatterDNAControls dnaControls;
        dnaControls.enabled = true;
        dnaControls.depth = raw (parameters, "dnaDepth");
        dnaControls.sensitivity = raw (parameters, "dnaSensitivity");
        dnaControls.crystalAttack = raw (parameters, "dnaCrystalAttack");
        dnaControls.vaporAir = raw (parameters, "dnaVaporAir");
        dnaControls.energyMotion = raw (parameters, "dnaEnergyMotion");
        dnaControls.spectralTone = raw (parameters, "dnaSpectralTone");
        organix::applyMatterDNA (aMaterial, a, dnaFeatures, dnaControls);
        if (activeB)
            organix::applyMatterDNA (bMaterial, b, dnaFeatures, dnaControls);
    }
    engineA.configure (aMaterial, a);
    if (activeB) engineB.configure (bMaterial, b);
    // The matrix generates offsets from immutable incoming audio only.
    // It never modifies APVTS base parameter values or captured scenes.
    const std::array<float,8> matrixMacros {{
        raw(parameters,"macroTransform"),raw(parameters,"macroMotion"),
        raw(parameters,"macroEnergy"),raw(parameters,"macroDensity"),
        raw(parameters,"macroSpace"),raw(parameters,"macroHarmonics"),
        raw(parameters,"macroEvolution"),raw(parameters,"macroFusion")
    }};
    float matrixFusion=fusion, matrixBalance=balance;

    // Consume coherent scene snapshots from lock-free atomic mailboxes. A
    // mid-write snapshot is skipped this block rather than blocking audio.
    for (int scene = 0; scene < 4; ++scene)
    {
        auto& slot = morphMailbox[static_cast<size_t> (scene)];
        const uint32_t version = slot.version.load (std::memory_order_acquire);
        if ((version & 1u) != 0u || version == receivedVersions[static_cast<size_t> (scene)])
            continue;
        std::array<float, organix::MorphScene::packedCount> packed {};
        for (size_t index = 0; index < packed.size(); ++index)
            packed[index] = slot.data[index].load (std::memory_order_relaxed);
        if (version == slot.version.load (std::memory_order_acquire))
        {
            morphRender.setScene (scene, organix::MorphScene::unpack (packed));
            receivedVersions[static_cast<size_t> (scene)] = version;
        }
    }
    // Consume any new freehand shape at block boundaries. Every mailbox
    // coordinate is atomic, so no ValueTree or UI vector is read by the DSP.
    const uint32_t trajectoryVersion = trajectoryMailbox.version.load (std::memory_order_acquire);
    if ((trajectoryVersion & 1u) == 0u &&
        trajectoryVersion != receivedTrajectoryVersion)
    {
        organix::TrajectoryPath candidate;
        candidate.count = juce::jlimit (0, organix::TrajectoryPath::maxPoints,
                          trajectoryMailbox.count.load (std::memory_order_relaxed));
        for (int j = 0; j < candidate.count; ++j)
        {
            candidate.points[static_cast<size_t> (j)] = {
                trajectoryMailbox.x[static_cast<size_t> (j)].load (std::memory_order_relaxed),
                trajectoryMailbox.y[static_cast<size_t> (j)].load (std::memory_order_relaxed)
            };
        }
        if (trajectoryMailbox.version.load (std::memory_order_acquire) == trajectoryVersion)
        {
            candidate.sanitize();
            audioTrajectory = candidate;
            receivedTrajectoryVersion = trajectoryVersion;
        }
    }
    const bool trajectoryPlaying = raw (parameters, "trajectoryEnable") > 0.5f;
    const bool trajectoryFree = raw (parameters, "trajectoryFree") > 0.5f;
    const bool trajectoryReverse = raw (parameters, "trajectoryReverse") > 0.5f;
    const int trajectoryShape = juce::jlimit (0, 3, static_cast<int> (
        raw (parameters, "trajectoryShape")));
    const int trajectoryDivision = juce::jlimit (0, 6, static_cast<int> (
        raw (parameters, "trajectoryDivision")));
    const float trajectoryDepth = juce::jlimit (0.0f, 1.0f,
        raw (parameters, "trajectoryDepth"));
    double trajectoryBpm = 120.0;
    if (trajectoryPlaying && ! trajectoryFree)
        if (auto* playHead = getPlayHead())
            if (const auto position = playHead->getPosition())
                if (const auto bpm = position->getBpm()) trajectoryBpm = *bpm;
    const float trajectoryHz = organix::TrajectoryDesigner::frequency (
        trajectoryFree, raw (parameters, "trajectorySpeed"),
        static_cast<float> (trajectoryBpm), trajectoryDivision);
    const float trajectoryAlpha = 1.0f - std::exp (-1.0f /
        (0.007f * static_cast<float> (juce::jmax (1.0, getSampleRate()))));
    const float morphXTarget = raw (parameters, "morphX");
    const float morphYTarget = raw (parameters, "morphY");
    const bool morphEnabled = raw (parameters, "morphEnable") > 0.5f || trajectoryPlaying;
    morphBlendSmooth.setTargetValue (morphEnabled ? 1.0f : 0.0f);
    morphXSmooth.setTargetValue (morphXTarget);
    morphYSmooth.setTargetValue (morphYTarget);
    const bool renderMorph = morphEnabled || morphBlendSmooth.getCurrentValue() > 0.00001f;
    organix::MatterDNAControls morphDNA;
    morphDNA.enabled = dnaIsEnabled;
    if (renderMorph)
    {
        if (dnaIsEnabled)
        {
            morphDNA.depth = raw (parameters, "dnaDepth");
            morphDNA.sensitivity = raw (parameters, "dnaSensitivity");
            morphDNA.crystalAttack = raw (parameters, "dnaCrystalAttack");
            morphDNA.vaporAir = raw (parameters, "dnaVaporAir");
            morphDNA.energyMotion = raw (parameters, "dnaEnergyMotion");
            morphDNA.spectralTone = raw (parameters, "dnaSpectralTone");
        }
        const auto currentXY = organix::TrajectoryDesigner::sample (
            trajectoryShape, trajectoryReverse ? 1.0f - trajectoryPhase : trajectoryPhase,
            audioTrajectory);
        const auto configureXY = trajectoryPlaying
            ? organix::TrajectoryDesigner::blend (
                {morphXSmooth.getCurrentValue(), morphYSmooth.getCurrentValue()},
                currentXY, trajectoryDepth)
            : organix::TrajectoryPoint {morphXSmooth.getCurrentValue(),
                                        morphYSmooth.getCurrentValue()};
        if (!matrixOn)
            morphRender.configure (configureXY.x, configureXY.y,
                                   dnaFeatures, morphDNA, reactions);
    }

    const float inputGain = juce::Decibels::decibelsToGain (raw (parameters, "input"));
    wetSmooth.setTargetValue (raw (parameters, "drywet"));
    bypassSmooth.setTargetValue (raw (parameters, "bypass") > 0.5f ? 0.0f : 1.0f);
    outputSmooth.setTargetValue (juce::Decibels::decibelsToGain (raw (parameters, "output")));
    widthSmooth.setTargetValue (raw (parameters, "width"));

    // Sample-accurate, host-tempo-aware movement. Read host tempo once per block.
    const bool freeMotion = raw (parameters, "motionFree") > 0.5f;
    double hostBpm = 120.0;
    if (! freeMotion || (matrixOn && matrixSettings.tempoSync))
        if (auto* playHead = getPlayHead())
            if (const auto position = playHead->getPosition())
                if (const auto bpm = position->getBpm())
                    hostBpm = *bpm;
    const int division = juce::jlimit (0, 6, static_cast<int> (raw (parameters, "motionDivision")));
    const int trajectory = juce::jlimit (0, 4, static_cast<int> (raw (parameters, "motionTrajectory")));
    const float frequency = organix::Movement::frequency (
        freeMotion, raw (parameters, "motionRate"), hostBpm, division);
    const float phaseIncrement = frequency / static_cast<float> (juce::jmax (1.0, getSampleRate()));
    const float movementAmount = raw (parameters, "macroMotion");
    auto* left = buffer.getWritePointer (0);
    auto* right = channels == 2 ? buffer.getWritePointer (1) : nullptr;
    double inputSquare = 0.0, outputSquare = 0.0;
    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        if (matrixOn && (i % 128 == 0))
        {
            // 128-frame control cadence: fully bounded, with an additional
            // 12 ms smoothing per route and JUCE output smoothing. No
            // allocation, locks or APVTS/ValueTree edits in this path.
            const auto offsets = modMatrix.processSegment (
                matrixSettings,left,right,i,std::min(128,n-i),channels,
                static_cast<float>(hostBpm),dnaFeatures,matrixMacros);
            auto modA=a, modB=b;
            matrixFusion=fusion; matrixBalance=balance;
            float modWet=raw(parameters,"drywet");
            float modWidth=raw(parameters,"width");
            offsets.apply(modA,modB,matrixFusion,matrixBalance,modWet,modWidth);
            engineA.configure(aMaterial,modA);
            if(activeB) engineB.configure(bMaterial,modB);
            wetSmooth.setTargetValue(modWet);
            widthSmooth.setTargetValue(modWidth);
            if(renderMorph)
            {
                const auto phase=trajectoryReverse?1.0f-trajectoryPhase:trajectoryPhase;
                const auto p=organix::TrajectoryDesigner::sample(
                    trajectoryShape,phase,audioTrajectory);
                const auto xy=trajectoryPlaying?
                    organix::TrajectoryDesigner::blend (
                        {morphXSmooth.getCurrentValue(),morphYSmooth.getCurrentValue()},
                        p,trajectoryDepth)
                    :organix::TrajectoryPoint{
                        morphXSmooth.getCurrentValue(),morphYSmooth.getCurrentValue()};
                morphRender.configure(xy.x,xy.y,dnaFeatures,morphDNA,reactions,&offsets);
            }
        }
        const float modulation = organix::Movement::wave (trajectory, movementPhase, movementHeld);
        const float wet = organix::Movement::scaleWet (
            wetSmooth.getNextValue(), movementAmount, modulation);
        movementPhase += phaseIncrement;
        if (movementPhase >= 1.0f)
        {
            movementPhase -= std::floor (movementPhase);
            movementRandom = movementRandom * 1664525u + 1013904223u;
            movementHeld = static_cast<float> ((movementRandom >> 8) & 0xffffu)
                           / 32767.5f - 1.0f;
        }

        const float active = bypassSmooth.getNextValue();
        const float transientDepth = transientDepthSmooth.getNextValue();
        const float gain = outputSmooth.getNextValue();
        const float width = widthSmooth.getNextValue();
        const float morphFraction = morphBlendSmooth.getNextValue();
        float morphX = morphXSmooth.getNextValue();
        float morphY = morphYSmooth.getNextValue();
        if (trajectoryPlaying)
        {
            const float phase = trajectoryReverse ? 1.0f - trajectoryPhase : trajectoryPhase;
            const auto p = organix::TrajectoryDesigner::sample (
                trajectoryShape, phase, audioTrajectory);
            smoothTrajectoryX += (p.x - smoothTrajectoryX) * trajectoryAlpha;
            smoothTrajectoryY += (p.y - smoothTrajectoryY) * trajectoryAlpha;
            const auto driven = organix::TrajectoryDesigner::blend (
                {morphX, morphY}, {smoothTrajectoryX, smoothTrajectoryY},
                trajectoryDepth);
            morphX = driven.x;
            morphY = driven.y;
            trajectoryPhase = organix::TrajectoryDesigner::advance (
                trajectoryPhase, trajectoryHz, getSampleRate());
        }
        trajectoryReadX.store (morphX, std::memory_order_relaxed);
        trajectoryReadY.store (morphY, std::memory_order_relaxed);
        float originals[2] = {0.0f, 0.0f};
        float inputs[2] = {0.0f, 0.0f};
        float processed[2] = {0.0f, 0.0f};

        for (int ch = 0; ch < channels; ++ch)
        {
            const float sample = ch == 0 ? left[i] : right[i];
            const float original = std::isfinite (sample) ? sample : 0.0f;
            const float x = original * inputGain;
            float y = x;
            // Skip the old processor when it is entirely masked by morphing.
            if (morphFraction < 0.99999f)
            {
                if (! activeB)
                    y = engineA.process (ch, x);
                else
                    y = reactionLink.process (engineA, engineB,
                        aMaterial, bMaterial, routing, matrixBalance, matrixFusion,
                        ch, x, reactions);
            }
            originals[ch] = original;
            inputs[ch] = x;
            processed[ch] = std::isfinite (y) ? y : 0.0f;
            inputSquare += static_cast<double> (x) * x;
        }

        // M/S width is applied only to the processed signal; dry and bypass
        // remain unchanged. Width 100% is the mathematically transparent default.
        if (channels == 2)
            organix::StereoWidth::process (processed[0], processed[1], width);

        float morphProcessed[2] {};
        float morphWet = wet;
        if (renderMorph && morphFraction > 0.00001f)
        {
            const auto weights = organix::Morph4D::weights (morphX, morphY);
            morphRender.processFrame (inputs, morphProcessed, channels, morphX, morphY);
            if (channels == 2)
                organix::StereoWidth::process (
                    morphProcessed[0], morphProcessed[1], morphRender.widthAt (weights));
            // Crossfade both DSP and saved per-scene wet values, with global
            // movement retained as a modulation source across both modes.
            morphWet = organix::Movement::scaleWet (
                morphRender.wetAt (weights), movementAmount, modulation);
        }

        for (int ch = 0; ch < channels; ++ch)
        {
            const float base = inputs[ch] * (1.0f - wet) + processed[ch] * wet;
            const float morph = inputs[ch] * (1.0f - morphWet) + morphProcessed[ch] * morphWet;
            const float mixed = (base * (1.0f - morphFraction) +
                                 morph * morphFraction) * gain;
            const float out = std::abs (mixed) < 0.96f
                ? mixed : std::tanh (mixed) * 0.96f / std::tanh (0.96f);
            const float effect = std::isfinite(out)
                ?juce::jlimit(-1.0f,1.0f,out):0.0f;
            // Stereo attack is modified in the actual wet DSP path. The
            // host bypass mix remains neutral; parameter 0 is bit-exact.
            const float shaped=transientShaper.process(ch,effect,transientDepth);
            const float destination = originals[ch] * (1.0f - active) + shaped * active;
            if (ch == 0) left[i] = destination;
            else right[i] = destination;
            outputSquare += static_cast<double> (destination) * destination;
            peak = std::max (peak, std::abs (destination));
        }
    }
    const float standardWeight = morphBlendSmooth.getCurrentValue();
    const float activityAB = reactionLink.activityAB() * (1.0f - standardWeight) +
                             (renderMorph ? morphRender.reactionMeterAB() : 0.0f) * standardWeight;
    const float activityBA = reactionLink.activityBA() * (1.0f - standardWeight) +
                             (renderMorph ? morphRender.reactionMeterBA() : 0.0f) * standardWeight;
    reactionActivityAB.store (reactions.enabled ? activityAB : 0.0f, std::memory_order_relaxed);
    reactionActivityBA.store (reactions.enabled ? activityBA : 0.0f, std::memory_order_relaxed);
    if(matrixOn)
    {
        const auto& report=modMatrix.getLatest().activity;
        for(size_t k=0;k<report.size();++k)
            matrixSlotActivity[k].store(report[k],std::memory_order_relaxed);
    }
    else
        for(auto& meterValue:matrixSlotActivity)
            meterValue.store(0.0f,std::memory_order_relaxed);
    const double divisor = static_cast<double> (channels) * n;
    inputRms.store (static_cast<float> (std::sqrt (inputSquare / divisor)), std::memory_order_relaxed);
    outputRms.store (static_cast<float> (std::sqrt (outputSquare / divisor)), std::memory_order_relaxed);
    outputPeak.store (peak, std::memory_order_relaxed);
    for (int ch = channels; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    spectralView.analyze (organix::SpectralViewAnalyzer::Stream::After,
                          buffer.getReadPointer(0),
                          channels>1?buffer.getReadPointer(1):nullptr,n,channels);
    // % of one real-time audio buffer consumed by this processor. This is not
    // a machine-wide CPU percentage, which JUCE cannot measure reliably.
    const auto cpuEnd=std::chrono::steady_clock::now();
    const double elapsed=std::chrono::duration<double>(cpuEnd-cpuStart).count();
    const double budget=static_cast<double>(n)/std::max(1.0,getSampleRate());
    const float actual=juce::jlimit(0.0f,400.0f,
                                   static_cast<float>(100.0*elapsed/budget));
    const float prior=audioCpuPercent.load(std::memory_order_relaxed);
    audioCpuPercent.store(prior+(actual-prior)*(actual>prior?0.20f:0.07f),
                          std::memory_order_relaxed);
}


void OrganixAudioProcessor::initialiseMidiLearn() noexcept
{
    for (auto& entry : ccParameterIndex) entry.store (-1, std::memory_order_relaxed);
    for (auto& entry : midiDirty) entry.store (false, std::memory_order_relaxed);
    for (auto& entry : midiValues) entry.store (0.0f, std::memory_order_relaxed);
    const auto& parameterList = getParameters();
    midiParameterCount = juce::jmin (midiLearnCapacity, parameterList.size());
    jassert (parameterList.size() <= midiLearnCapacity);
    for (int i = 0; i < midiParameterCount; ++i)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (parameterList[i]);
        auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameterList[i]);
        midiParameters[static_cast<size_t> (i)] = param;
        midiParameterIds[static_cast<size_t> (i)] =
            withId != nullptr ? withId->getParameterID() : juce::String();
    }
}

int OrganixAudioProcessor::midiIndex (const juce::String& id) const noexcept
{
    if (id.isEmpty()) return -1;
    for (int i = 0; i < midiParameterCount; ++i)
        if (midiParameterIds[static_cast<size_t> (i)] == id
            && midiParameters[static_cast<size_t> (i)] != nullptr)
            return i;
    return -1;
}

void OrganixAudioProcessor::startMidiLearn (const juce::String& id) noexcept
{
    midiLearnTarget.store (midiIndex (id), std::memory_order_release);
}

juce::String OrganixAudioProcessor::midiLearnTargetName() const
{
    const int index = midiLearnTarget.load (std::memory_order_acquire);
    if (index < 0 || index >= midiParameterCount) return {};
    return midiParameterIds[static_cast<size_t> (index)];
}

juce::String OrganixAudioProcessor::midiLearnMapping (const juce::String& id) const
{
    const int target = midiIndex (id);
    if (target < 0) return {};
    for (int slot = 0; slot < midiCCSlots; ++slot)
        if (ccParameterIndex[static_cast<size_t> (slot)].load (std::memory_order_acquire) == target)
            return "CH " + juce::String (slot / 128 + 1) + " / CC " + juce::String (slot % 128);
    return {};
}

void OrganixAudioProcessor::clearMidiLearn (const juce::String& id) noexcept
{
    const int target = midiIndex (id);
    if (target < 0) return;
    for (auto& slot : ccParameterIndex)
        if (slot.load (std::memory_order_relaxed) == target)
            slot.store (-1, std::memory_order_release);
    midiDirty[static_cast<size_t> (target)].store (false, std::memory_order_release);
    if (midiLearnTarget.load (std::memory_order_relaxed) == target)
        midiLearnTarget.store (-1, std::memory_order_release);
}

void OrganixAudioProcessor::clearAllMidiLearn() noexcept
{
    midiLearnTarget.store (-1, std::memory_order_release);
    for (auto& slot : ccParameterIndex) slot.store (-1, std::memory_order_release);
    for (auto& dirty : midiDirty) dirty.store (false, std::memory_order_release);
}

void OrganixAudioProcessor::handleMidiLearn (const juce::MidiBuffer& incoming) noexcept
{
    // Lock-free/zero-allocation MIDI message processing; the DAW's MIDI events
    // are left intact. Actual JUCE host notifications occur on Timer thread.
    for (const auto metadata : incoming)
    {
        // Inspect raw three-byte MIDI CC messages without constructing
        // MidiMessage objects. getMessage() can allocate for large SysEx.
        if (metadata.data == nullptr || metadata.numBytes != 3 ||
            (metadata.data[0] & 0xf0u) != 0xb0u) continue;
        const int cc = metadata.data[1];
        const int value = metadata.data[2];
        const int channel = (metadata.data[0] & 0x0fu) + 1;
        if (cc < 1 || cc > 119) continue;
        const int slot = (channel - 1) * 128 + cc;
        const int target = midiLearnTarget.exchange (-1, std::memory_order_acq_rel);
        if (target >= 0 && target < midiParameterCount)
        {
            // A single physical CC moves one parameter; a parameter is allowed
            // to have more than one learned controller across MIDI channels.
            ccParameterIndex[static_cast<size_t> (slot)].store (target, std::memory_order_release);
        }
        const int mapped = ccParameterIndex[static_cast<size_t> (slot)].load (std::memory_order_acquire);
        if (mapped >= 0 && mapped < midiParameterCount)
        {
            midiValues[static_cast<size_t> (mapped)].store (
                value / 127.0f, std::memory_order_relaxed);
            midiDirty[static_cast<size_t> (mapped)].store (true, std::memory_order_release);
        }
    }
}

void OrganixAudioProcessor::flushMidiLearnUpdates()
{
    // Message thread only: never call setValueNotifyingHost on audio thread.
    for (int index = 0; index < midiParameterCount; ++index)
    {
        if (! midiDirty[static_cast<size_t> (index)].exchange (
                false, std::memory_order_acq_rel)) continue;
        if (auto* param = midiParameters[static_cast<size_t> (index)])
            param->setValueNotifyingHost (
                midiValues[static_cast<size_t> (index)].load (std::memory_order_acquire));
    }
}
void OrganixAudioProcessor::timerCallback() { flushMidiLearnUpdates(); }

juce::String OrganixAudioProcessor::serialiseMidiLearn() const
{
    // Stable parameter IDs, not fragile process-local parameter indices.
    juce::StringArray mappings;
    for (int slot = 0; slot < midiCCSlots; ++slot)
    {
        const int idx = ccParameterIndex[static_cast<size_t> (slot)].load (std::memory_order_acquire);
        if (idx < 0 || idx >= midiParameterCount) continue;
        const auto& id = midiParameterIds[static_cast<size_t> (idx)];
        if (id.isNotEmpty())
            mappings.add (juce::String (slot / 128 + 1) + ":" +
                          juce::String (slot % 128) + ":" + id);
    }
    return mappings.joinIntoString ("|");
}
void OrganixAudioProcessor::restoreMidiLearn (const juce::String& data)
{
    clearAllMidiLearn();
    const auto mappings = juce::StringArray::fromTokens (data, "|", "");
    for (const auto& row : mappings)
    {
        const auto fields = juce::StringArray::fromTokens (row, ":", "");
        if (fields.size() != 3) continue;
        if (! fields[0].containsOnly ("0123456789") ||
            ! fields[1].containsOnly ("0123456789")) continue;
        const int ch = fields[0].getIntValue(), cc = fields[1].getIntValue();
        const int idx = midiIndex (fields[2]);
        if (ch >= 1 && ch <= 16 && cc >= 1 && cc <= 119 && idx >= 0)
            ccParameterIndex[static_cast<size_t> ((ch - 1) * 128 + cc)]
                .store (idx, std::memory_order_release);
    }
}

void OrganixAudioProcessor::setParameter (const char* id, float value)
{
    if (auto* parameter = parameters.getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

void OrganixAudioProcessor::loadFactoryPreset (int index)
{
    if (index < 0 || index >= static_cast<int> (organix::factoryPresets.size()))
        return;

    // v1.3: ONE hand-authored voice design for EACH of the historical 100 names.
    // Factory program recall never reads/writes preset lab A/B or undo history.
    // User presets and existing DAW documents restore their stored state as before.
    const auto& preset = organix::factoryPresets[static_cast<size_t> (index)];
    const auto& design = organix::factorySoundDesigns[static_cast<size_t> (index)];
    const auto flags = design.technologies;
    const bool dna = (flags & organix::D) != 0;
    const bool morph = (flags & (organix::M | organix::T)) != 0;
    const bool trajectory = (flags & organix::T) != 0 && morph;
    const bool reactions = (flags & organix::R) != 0;
    const bool matrix = (flags & organix::X) != 0;
    const bool rhythmic = preset.category == 4 || preset.category == 9;
    const bool bass = preset.category == 6;
    const bool mix = preset.category == 10;
    const bool paired = preset.b >= 0 || morph || reactions;
    const int aMaterial = static_cast<int> (preset.a);
    const int bMaterial = preset.b < 0
        ? (aMaterial + (design.scenePalette % 3) + 1) % 6 : preset.b;

    setParameter ("materialA", static_cast<float> (aMaterial));
    setParameter ("materialB", static_cast<float> (bMaterial));
    setParameter ("enableB", paired ? 1.0f : 0.0f);
    setParameter ("routing", paired
        ? static_cast<float> (reactions ? 0 :
            (design.scenePalette == 4 ? 3 :
             (design.scenePalette == 2 ? 1 : 0))) : 0.0f);
    setParameter ("aAmount", design.aAmount);
    setParameter ("aMotion", design.aMotion);
    setParameter ("aTone", design.aTone);
    setParameter ("aFeedback", design.aFeedback);
    setParameter ("bAmount", design.bAmount);
    setParameter ("bMotion", design.bMotion);
    setParameter ("bTone", design.bTone);
    setParameter ("bFeedback", design.bFeedback);
    setParameter ("balance", paired
        ? juce::jlimit (0.20f, 0.80f,
            0.48f + (design.bAmount - design.aAmount) * 0.23f) : 0.50f);
    setParameter ("fusion", juce::jlimit (0.05f, 0.70f,
        0.12f + (design.energy - 0.20f) * 0.44f));
    setParameter ("drywet", design.wet);
    setParameter ("width", design.width);
    setParameter ("input", 0.0f);
    setParameter ("output", design.outputDb);
    setParameter ("bypass", 0.0f);
    setParameter ("spectralCompare", 2.0f);
    setParameter ("spectralTransient", design.transient);
    setParameter ("motionFree", rhythmic ? 0.0f : 1.0f);
    setParameter ("motionRate", juce::jlimit(0.05f,6.0f,design.speed));
    setParameter ("motionDivision", rhythmic ? 3.0f : 2.0f);
    setParameter ("motionTrajectory", rhythmic ? 1.0f : 0.0f);

    // A unique base patch exposes the six meaningful material DSP properties.
    // The remaining 6 are kept near-neutral: Organix maps them as additional
    // internally normalized DSP properties, not dummy decorative controls.
    const float aShapes[12] = {
        design.aTone, design.aMotion, design.density,
        design.energy, design.aAmount,
        juce::jlimit(0.0f,1.0f,design.aFeedback*1.10f),
        design.aTone, design.energy,
        design.aMotion, design.density, design.aAmount, 0.50f
    };
    const float bShapes[12] = {
        design.bTone, design.bMotion, design.density,
        design.energy, design.bAmount,
        juce::jlimit(0.0f,1.0f,design.bFeedback*1.10f),
        design.bTone, design.energy,
        design.bMotion, design.density, design.bAmount, 0.50f
    };
    for (int i=0;i<12;++i)
    {
        const auto suffix=juce::String(i+1).paddedLeft('0',2);
        setParameter((juce::String("aShape")+suffix).toRawUTF8(),aShapes[i]);
        setParameter((juce::String("bShape")+suffix).toRawUTF8(),bShapes[i]);
    }

    // Explicit sonic intentions give the eight macros genuinely varied states.
    // They are never randomized during factory program loading.
    setParameter("macroTransform",juce::jlimit(0.0f,1.0f,
        0.48f+(design.aAmount-design.bAmount)*0.22f));
    setParameter("macroMotion",juce::jlimit(0.0f,1.0f,
        0.48f+(design.aMotion-design.bMotion)*0.30f));
    setParameter("macroEnergy",design.energy);
    setParameter("macroDensity",design.density);
    setParameter("macroSpace",juce::jlimit(0.0f,1.0f,
        0.48f+(design.width-1.0f)*0.28f));
    setParameter("macroHarmonics",juce::jlimit(0.0f,1.0f,
        0.50f+(design.aTone-design.bTone)*0.22f));
    setParameter("macroEvolution",juce::jlimit(0.0f,1.0f,
        0.48f+design.speed*0.14f));
    setParameter("macroFusion",0.50f);

    setParameter("dnaEnable",dna?1.0f:0.0f);
    setParameter("dnaDepth",design.dnaDepth);
    setParameter("dnaSensitivity",juce::jlimit(0.20f,0.76f,
        0.32f+design.energy*0.30f));
    setParameter("dnaCrystalAttack",preset.a==organix::Material::Crystal
        ? 0.73f : 0.39f);
    setParameter("dnaVaporAir",preset.a==organix::Material::Vapor
        ? 0.76f : 0.37f);
    setParameter("dnaEnergyMotion",rhythmic?0.64f:0.35f);
    setParameter("dnaSpectralTone",juce::jlimit(0.10f,0.48f,
        0.20f+design.aTone*0.26f));

    setParameter("reactionEnable",reactions?1.0f:0.0f);
    setParameter("reactionAmount",design.reactionDepth);
    setParameter("reactionDirection",reactions
        ? (design.scenePalette==4?2.0f:0.0f) : 2.0f);
    setParameter("reactionResponse",rhythmic?0.20f:0.46f);

    // Reset the complete 32-route matrix, not just its global Enable.
    // Otherwise recalling a different factory preset could retain old slots.
    setParameter("matrixEnable",0.0f);
    setParameter("matrixMasterDepth",matrix?0.48f:0.80f);
    setParameter("matrixTempoSync",rhythmic?1.0f:0.0f);
    setParameter("matrixBeatDivision",rhythmic?3.0f:2.0f);
    const float lfoRates[]={
        juce::jlimit(0.06f,4.0f,design.speed),
        0.33f,0.54f,0.82f
    };
    for(int i=1;i<=4;++i)
    {
        const auto name=juce::String("matrixLfo")+juce::String(i)+"Rate";
        setParameter(name.toRawUTF8(),lfoRates[i-1]);
    }
    setParameter("matrixEnv1Attack",rhythmic?0.008f:0.030f);
    setParameter("matrixEnv1Decay",rhythmic?0.20f:0.42f);
    setParameter("matrixEnv2Attack",0.060f);
    setParameter("matrixEnv2Decay",0.74f);
    setParameter("matrixRandomRate",juce::jlimit(0.10f,4.0f,
        design.speed*1.7f));
    for(int i=1;i<=organix::MatrixSettings::slotCount;++i)
    {
        const auto stem=juce::String("mm")+juce::String(i).paddedLeft('0',2);
        setParameter((stem+"Enable").toRawUTF8(),0.0f);
        setParameter((stem+"Source").toRawUTF8(),0.0f);
        setParameter((stem+"Target").toRawUTF8(),0.0f);
        setParameter((stem+"Depth").toRawUTF8(),0.0f);
        setParameter((stem+"Polarity").toRawUTF8(),0.0f);
    }
    if(matrix)
    {
        const auto setRoute=[this](int slot,int source,int target,
                                   float depth,bool bipolar)
        {
            const auto stem=juce::String("mm")+juce::String(slot).paddedLeft('0',2);
            setParameter((stem+"Source").toRawUTF8(),static_cast<float>(source));
            setParameter((stem+"Target").toRawUTF8(),static_cast<float>(target));
            setParameter((stem+"Depth").toRawUTF8(),depth);
            setParameter((stem+"Polarity").toRawUTF8(),bipolar?1.0f:0.0f);
            setParameter((stem+"Enable").toRawUTF8(),1.0f);
        };
        setRoute(1,design.modulationSource,design.modulationTarget,
                 design.modulationDepth,true);
        // A restrained, independent second connection improves expressivity
        // without hiding a large unrelated 32-slot random matrix.
        const int secondTarget = bass ? 3 : mix ? 39 :
            (design.modulationTarget<=18?21:3);
        const int secondSource = rhythmic ? 8 : 7;
        setRoute(2,secondSource,secondTarget,
                 (design.modulationDepth>=0.0f?0.12f:-0.12f),false);
        setParameter("matrixEnable",1.0f);
    }

    setParameter("morphEnable",0.0f);
    setParameter("morphX",0.0f);
    setParameter("morphY",0.0f);
    setParameter("trajectoryEnable",0.0f);
    setParameter("trajectoryShape",static_cast<float>(design.trajectoryShape));
    setParameter("trajectorySpeed",design.speed);
    setParameter("trajectoryFree",rhythmic?0.0f:1.0f);
    setParameter("trajectoryDivision",rhythmic?3.0f:2.0f);
    setParameter("trajectoryReverse",design.scenePalette==2?1.0f:0.0f);
    setParameter("trajectoryDepth",trajectory?0.66f:0.0f);

    // Four deliberately contrasting *independent DSP scenes*. Not four
    // identical copies of the same A/B mixture as in older factory banks.
    const auto baseScene=captureCurrentMorphScene();
    for(int i=0;i<4;++i)
    {
        auto scene=baseScene;
        if(morph && i>0)
        {
            const float depth=design.scenePalette==3?0.075f:0.16f;
            const float value=i==1?1.0f:i==2?-1.0f:0.45f;
            scene.a.tone=clamp01(scene.a.tone+value*depth);
            scene.b.tone=clamp01(scene.b.tone-value*depth*0.70f);
            scene.a.motion=clamp01(scene.a.motion+(i==3?0.15f:-0.04f));
            scene.b.motion=clamp01(scene.b.motion+(i==2?0.15f:-0.03f));
            scene.a.amount=clamp01(scene.a.amount+(i==1?-0.05f:0.06f));
            scene.b.amount=clamp01(scene.b.amount+(i==2?-0.07f:0.03f));
            scene.fusion=clamp01(scene.fusion+(i==3?0.12f:-0.04f));
            scene.balance=clamp01(scene.balance+(i==1?-0.14f:
                                            i==2?0.13f:0.04f));
            scene.width=juce::jlimit(0.75f,bass?1.05f:1.40f,
                scene.width+(i==1?0.03f:i==2?-0.07f:0.07f));
            // The five curated palettes determine which sonic materials can
            // change between scenes; subtle and bass programs stay stable.
            if(design.scenePalette==1 && i==2)
                scene.materialB=static_cast<int>(organix::Material::Light);
            if(design.scenePalette==2 && i==3)
                scene.materialB=static_cast<int>(organix::Material::Crystal);
            if(design.scenePalette==4 && i==3)
            {
                scene.materialA=static_cast<int>(organix::Material::Metal);
                scene.materialB=static_cast<int>(organix::Material::Plasma);
                scene.routing=3;
            }
        }
        publishMorphScene(i,scene,true);
    }

    organix::TrajectoryPath path;
    if(trajectory && design.trajectoryShape==3)
    {
        path.count=5;
        path.points[0]={0.12f,0.38f};
        path.points[1]={0.37f,0.81f};
        path.points[2]={0.75f,0.62f};
        path.points[3]={0.82f,0.16f};
        path.points[4]={0.31f,0.24f};
    }
    setTrajectoryPath(path,true);
    // Turn enabling on LAST, after all scene/path/mailbox changes are ready.
    setParameter("morphEnable",morph?1.0f:0.0f);
    setParameter("trajectoryEnable",trajectory?1.0f:0.0f);
    parameters.state.removeProperty("customPresetName",nullptr);
    parameters.state.setProperty("factorySoundDesignVersion",3,nullptr);
    currentPreset.store(index,std::memory_order_relaxed);
}
organix::MorphScene OrganixAudioProcessor::captureCurrentMorphScene() const noexcept
{
    auto read = [this] (const char* name) noexcept
    {
        if (const auto* p = parameters.getRawParameterValue (name))
            return p->load (std::memory_order_relaxed);
        return 0.0f;
    };
    organix::MorphScene scene;
    scene.materialA = juce::jlimit (0, 5, static_cast<int> (read ("materialA")));
    scene.materialB = juce::jlimit (0, 5, static_cast<int> (read ("materialB")));
    scene.routing = juce::jlimit (0, 3, static_cast<int> (read ("routing")));
    scene.enableB = read ("enableB") > 0.5f;
    const float transform = read ("macroTransform") - 0.5f;
    const float motion = read ("macroMotion") - 0.5f;
    const float energy = read ("macroEnergy");
    const float density = read ("macroDensity");
    const float space = read ("macroSpace") - 0.5f;
    const float harmonics = read ("macroHarmonics") - 0.5f;
    const float evolution = read ("macroEvolution") - 0.5f;
    scene.a.amount = clamp01 (read ("aAmount") + transform * 0.5f);
    scene.a.motion = clamp01 (read ("aMotion") + motion * 0.5f + evolution * 0.3f);
    scene.a.tone = clamp01 (read ("aTone") + harmonics * 0.6f);
    scene.a.feedback = juce::jlimit (0.0f, 0.88f, read ("aFeedback") + space * 0.25f);
    scene.a.energy = energy;
    scene.a.density = density;
    scene.b.amount = clamp01 (read ("bAmount") + transform * 0.5f);
    scene.b.motion = clamp01 (read ("bMotion") + motion * 0.5f - evolution * 0.3f);
    scene.b.tone = clamp01 (read ("bTone") + harmonics * 0.6f);
    scene.b.feedback = juce::jlimit (0.0f, 0.88f, read ("bFeedback") + space * 0.25f);
    scene.b.energy = energy;
    scene.b.density = density;
    for (int i = 0; i < 12; ++i)
    {
        const auto suffix = juce::String (i + 1).paddedLeft ('0', 2);
        scene.a.shape[static_cast<size_t> (i)] = read ((juce::String ("aShape") + suffix).toRawUTF8());
        scene.b.shape[static_cast<size_t> (i)] = read ((juce::String ("bShape") + suffix).toRawUTF8());
    }
    scene.fusion = clamp01 (read ("fusion") + (read ("macroFusion") - 0.5f) * 0.6f);
    scene.balance = read ("balance");
    scene.wet = read ("drywet");
    scene.width = read ("width");
    scene.sanitize();
    return scene;
}
juce::String OrganixAudioProcessor::encodeMorphScene (const organix::MorphScene& scene)
{
    const auto values = scene.pack();
    juce::StringArray tokens;
    for (const auto value : values)
        tokens.add (juce::String (value, 9));
    return tokens.joinIntoString (",");
}
bool OrganixAudioProcessor::decodeMorphScene (const juce::String& serial,
                                                organix::MorphScene& destination)
{
    const auto tokens = juce::StringArray::fromTokens (serial, ",", "");
    if (tokens.size() != organix::MorphScene::packedCount) return false;
    std::array<float, organix::MorphScene::packedCount> data {};
    for (int i = 0; i < tokens.size(); ++i)
    {
        const auto text = tokens[i].trim();
        if (text.isEmpty() || ! text.containsOnly ("-+.0123456789eE")) return false;
        const double value = text.getDoubleValue();
        if (! std::isfinite (value)) return false;
        data[static_cast<size_t> (i)] = static_cast<float> (value);
    }
    destination = organix::MorphScene::unpack (data);
    return true;
}
void OrganixAudioProcessor::publishMorphScene (int index,
                                                organix::MorphScene scene, bool persist)
{
    if (index < 0 || index >= 4) return;
    scene.sanitize();
    auto& slot = morphMailbox[static_cast<size_t> (index)];
    const auto data = scene.pack();
    slot.version.fetch_add (1, std::memory_order_acq_rel); // mark write in progress
    for (size_t i = 0; i < data.size(); ++i)
        slot.data[i].store (data[i], std::memory_order_relaxed);
    slot.version.fetch_add (1, std::memory_order_release);
    if (persist)
        parameters.state.setProperty ("morphScene" + juce::String (index),
                                      encodeMorphScene (scene), nullptr);
}
void OrganixAudioProcessor::captureMorphScene (int index)
{
    publishMorphScene (index, captureCurrentMorphScene(), true);
}
organix::MorphScene OrganixAudioProcessor::readMorphScene (int index) const noexcept
{
    if (index < 0 || index >= 4) return {};
    const auto& slot = morphMailbox[static_cast<size_t> (index)];
    const auto versionBefore = slot.version.load (std::memory_order_acquire);
    if (versionBefore & 1u) return {};
    std::array<float, organix::MorphScene::packedCount> values {};
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = slot.data[i].load (std::memory_order_relaxed);
    const auto versionAfter = slot.version.load (std::memory_order_acquire);
    return versionBefore == versionAfter ? organix::MorphScene::unpack (values)
                                         : organix::MorphScene {};
}
juce::String OrganixAudioProcessor::encodeTrajectory (const organix::TrajectoryPath& source)
{
    auto path = source;
    path.sanitize();
    juce::StringArray chunks;
    for (int i = 0; i < path.count; ++i)
    {
        const auto p = path.points[static_cast<size_t> (i)];
        chunks.add (juce::String (p.x, 6) + "," + juce::String (p.y, 6));
    }
    return chunks.joinIntoString (";");
}
bool OrganixAudioProcessor::decodeTrajectory (const juce::String& content,
                                               organix::TrajectoryPath& path)
{
    path = {};
    if (content.trim().isEmpty()) return true;
    const auto points = juce::StringArray::fromTokens (content, ";", "");
    if (points.size() < 2 || points.size() > organix::TrajectoryPath::maxPoints) return false;
    for (const auto& token : points)
    {
        const auto first = token.upToFirstOccurrenceOf (",", false, false).trim();
        const auto second = token.fromFirstOccurrenceOf (",", false, false).trim();
        if (first.isEmpty() || second.isEmpty() ||
            ! first.containsOnly ("-+.0123456789eE") ||
            ! second.containsOnly ("-+.0123456789eE")) return false;
        const auto x = first.getDoubleValue(), y = second.getDoubleValue();
        if (! std::isfinite (x) || ! std::isfinite (y)) return false;
        path.points[static_cast<size_t> (path.count++)] = {
            static_cast<float> (x), static_cast<float> (y)
        };
    }
    path.sanitize();
    return true;
}
void OrganixAudioProcessor::setTrajectoryPath (organix::TrajectoryPath path, bool persist)
{
    path.sanitize();
    auto& slot = trajectoryMailbox;
    slot.version.fetch_add (1, std::memory_order_acq_rel);
    slot.count.store (path.count, std::memory_order_relaxed);
    for (int i = 0; i < path.count; ++i)
    {
        const auto& p = path.points[static_cast<size_t> (i)];
        slot.x[static_cast<size_t> (i)].store (p.x, std::memory_order_relaxed);
        slot.y[static_cast<size_t> (i)].store (p.y, std::memory_order_relaxed);
    }
    slot.version.fetch_add (1, std::memory_order_release);
    if (persist)
        parameters.state.setProperty ("trajectoryPoints", encodeTrajectory (path), nullptr);
}
organix::TrajectoryPath OrganixAudioProcessor::getTrajectoryPath() const noexcept
{
    organix::TrajectoryPath path;
    const auto& slot = trajectoryMailbox;
    const uint32_t v1 = slot.version.load (std::memory_order_acquire);
    if ((v1 & 1u) != 0u) return path;
    path.count = juce::jlimit (0, organix::TrajectoryPath::maxPoints,
                               slot.count.load (std::memory_order_relaxed));
    for (int i = 0; i < path.count; ++i)
    {
        path.points[static_cast<size_t> (i)].x =
            slot.x[static_cast<size_t> (i)].load (std::memory_order_relaxed);
        path.points[static_cast<size_t> (i)].y =
            slot.y[static_cast<size_t> (i)].load (std::memory_order_relaxed);
    }
    if (v1 != slot.version.load (std::memory_order_acquire)) return {};
    path.sanitize();
    return path;
}

void OrganixAudioProcessor::goToMorphScene (int index)
{
    if (index < 0 || index >= 4) return;
    setParameter ("morphX", index % 2 == 1 ? 1.0f : 0.0f);
    setParameter ("morphY", index >= 2 ? 1.0f : 0.0f);
}

void OrganixAudioProcessor::setCurrentProgram (int index) { loadFactoryPreset (index); }
const juce::String OrganixAudioProcessor::getProgramName (int index)
{
    return index >= 0 && index < static_cast<int> (organix::factoryPresets.size())
        ? juce::String (organix::factoryPresets[static_cast<size_t> (index)].name) : juce::String();
}

void OrganixAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto snapshot = parameters.copyState();
    snapshot.setProperty ("factoryPresetIndex", currentPreset.load(), nullptr);
    snapshot.setProperty ("midiLearnMappingsV1", serialiseMidiLearn(), nullptr);
    if (auto xml = snapshot.createXml()) copyXmlToBinary (*xml, dest);
}
void OrganixAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasTagName (parameters.state.getType().toString()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset.store (juce::jlimit (0, 99, static_cast<int> (state.getProperty ("factoryPresetIndex", 0))));
            parameters.replaceState (state);
            restoreMidiLearn (state.getProperty ("midiLearnMappingsV1").toString());
            organix::TrajectoryPath restoredPath;
            if (decodeTrajectory (state.getProperty ("trajectoryPoints").toString(), restoredPath))
                setTrajectoryPath (restoredPath, false);
            else
                setTrajectoryPath ({}, false);
            for (int index = 0; index < 4; ++index)
            {
                organix::MorphScene snapshot;
                if (decodeMorphScene (
                        state.getProperty ("morphScene" + juce::String (index)).toString(),
                        snapshot))
                    publishMorphScene (index, snapshot, false);
                else
                    captureMorphScene (index); // old v0.6 projects
            }
        }
    }
}

juce::AudioProcessorEditor* OrganixAudioProcessor::createEditor() { return new OrganixViewportEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrganixAudioProcessor(); }
