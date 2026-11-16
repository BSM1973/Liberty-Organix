#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
float clamp01 (float x) { return juce::jlimit (0.0f, 1.0f, x); }
}

OrganixAudioProcessor::OrganixAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "ORGANIX_STATE", makeLayout())
{
    loadFactoryPreset (0);
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
    layout.add (std::make_unique<Bool> (pid ("morphEnable"), "Four Scene Morph Enabled", false));
    add ("morphX", "Four Scene Morph X", 0.0f, 1.0f, 0.0f);
    add ("morphY", "Four Scene Morph Y", 0.0f, 1.0f, 0.0f);
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

void OrganixAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    engineA.prepare (sampleRate, getTotalNumInputChannels());
    engineB.prepare (sampleRate, getTotalNumInputChannels());
    matterDNA.prepare (sampleRate);
    morphRender.prepare (sampleRate, getTotalNumInputChannels());
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
    morphRender.reset();
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
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;
    const auto channels = std::min (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (channels <= 0 || n <= 0) return;

    const bool activeB = raw (parameters, "enableB") > 0.5f;
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
    if (dnaIsEnabled)
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
    const float morphXTarget = raw (parameters, "morphX");
    const float morphYTarget = raw (parameters, "morphY");
    const bool morphEnabled = raw (parameters, "morphEnable") > 0.5f;
    morphBlendSmooth.setTargetValue (morphEnabled ? 1.0f : 0.0f);
    morphXSmooth.setTargetValue (morphXTarget);
    morphYSmooth.setTargetValue (morphYTarget);
    const bool renderMorph = morphEnabled || morphBlendSmooth.getCurrentValue() > 0.00001f;
    if (renderMorph)
    {
        organix::MatterDNAControls morphDNA;
        morphDNA.enabled = dnaIsEnabled;
        if (dnaIsEnabled)
        {
            morphDNA.depth = raw (parameters, "dnaDepth");
            morphDNA.sensitivity = raw (parameters, "dnaSensitivity");
            morphDNA.crystalAttack = raw (parameters, "dnaCrystalAttack");
            morphDNA.vaporAir = raw (parameters, "dnaVaporAir");
            morphDNA.energyMotion = raw (parameters, "dnaEnergyMotion");
            morphDNA.spectralTone = raw (parameters, "dnaSpectralTone");
        }
        morphRender.configure (morphXSmooth.getCurrentValue(),
                               morphYSmooth.getCurrentValue(),
                               dnaFeatures, morphDNA);
    }

    const float inputGain = juce::Decibels::decibelsToGain (raw (parameters, "input"));
    wetSmooth.setTargetValue (raw (parameters, "drywet"));
    bypassSmooth.setTargetValue (raw (parameters, "bypass") > 0.5f ? 0.0f : 1.0f);
    outputSmooth.setTargetValue (juce::Decibels::decibelsToGain (raw (parameters, "output")));
    widthSmooth.setTargetValue (raw (parameters, "width"));

    // Sample-accurate, host-tempo-aware movement. Read host tempo once per block.
    const bool freeMotion = raw (parameters, "motionFree") > 0.5f;
    double hostBpm = 120.0;
    if (! freeMotion)
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
        const float gain = outputSmooth.getNextValue();
        const float width = widthSmooth.getNextValue();
        const float morphFraction = morphBlendSmooth.getNextValue();
        const float morphX = morphXSmooth.getNextValue();
        const float morphY = morphYSmooth.getNextValue();
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
                else if (routing == 1)
                    y = engineB.process (ch, engineA.process (ch, x));
                else if (routing == 2)
                    y = engineA.process (ch, engineB.process (ch, x));
                else
                {
                    const float xa = engineA.process (ch, x);
                    const float xb = engineB.process (ch, x);
                    y = (1.0f - balance) * xa + balance * xb;
                    if (routing == 3)
                        y += fusion * 0.27f * std::tanh (3.0f * xa * xb);
                }
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
            const float effect = std::isfinite (out) ? juce::jlimit (-1.0f, 1.0f, out) : 0.0f;
            const float destination = originals[ch] * (1.0f - active) + effect * active;
            if (ch == 0) left[i] = destination;
            else right[i] = destination;
            outputSquare += static_cast<double> (destination) * destination;
            peak = std::max (peak, std::abs (destination));
        }
    }
    const double divisor = static_cast<double> (channels) * n;
    inputRms.store (static_cast<float> (std::sqrt (inputSquare / divisor)), std::memory_order_relaxed);
    outputRms.store (static_cast<float> (std::sqrt (outputSquare / divisor)), std::memory_order_relaxed);
    outputPeak.store (peak, std::memory_order_relaxed);
    for (int ch = channels; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
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
    if (index < 0 || index >= static_cast<int> (organix::factoryPresets.size())) return;
    // Presets are parameterized starting points. Per-preset listening and QA remain pending.
    const auto& preset = organix::factoryPresets[static_cast<size_t> (index)];
    const int number = index % 10;
    const float t = static_cast<float> (number) / 9.0f;
    const bool subtle = preset.category == 10 || preset.category == 7;
    const bool aggressive = preset.category == 8 || preset.category == 4;
    const bool rhythmic = preset.category == 9 || preset.category == 4;
    const bool ambient = preset.category == 1 || preset.category == 7;
    const float base = subtle ? 0.19f : aggressive ? 0.75f : ambient ? 0.60f : 0.46f;
    setParameter ("materialA", static_cast<float> (preset.a));
    setParameter ("materialB", static_cast<float> (preset.b < 0 ? (static_cast<int> (preset.a) + 1) % 6 : preset.b));
    setParameter ("enableB", preset.b < 0 ? 0.0f : 1.0f);
    setParameter ("routing", static_cast<float> (preset.b < 0 ? 0 : rhythmic ? 3 : (index % 3 == 0 ? 1 : 0)));
    setParameter ("aAmount", clamp01 (base + 0.16f * t));
    setParameter ("bAmount", clamp01 (base + 0.14f * (1.0f - t)));
    setParameter ("aMotion", clamp01 ((rhythmic ? 0.70f : ambient ? 0.18f : 0.35f) + 0.24f * t));
    setParameter ("bMotion", clamp01 ((rhythmic ? 0.55f : ambient ? 0.12f : 0.27f) + 0.20f * (1.0f - t)));
    setParameter ("aTone", clamp01 (0.15f + 0.68f * (static_cast<float> ((index * 7) % 17) / 16.0f)));
    setParameter ("bTone", clamp01 (0.2f + 0.6f * (static_cast<float> ((index * 11) % 19) / 18.0f)));
    setParameter ("aFeedback", subtle ? 0.08f : ambient ? 0.50f : 0.20f + 0.20f * t);
    setParameter ("bFeedback", subtle ? 0.10f : ambient ? 0.42f : 0.15f + 0.23f * (1.0f - t));
    setParameter ("balance", 0.32f + 0.33f * t);
    setParameter ("fusion", aggressive ? 0.65f : subtle ? 0.10f : 0.25f);
    setParameter ("drywet", subtle ? 0.30f : aggressive ? 0.80f : ambient ? 0.88f : 0.64f);
    setParameter ("width", 1.0f);
    setParameter ("motionFree", 1.0f);
    setParameter ("motionRate", 0.25f);
    setParameter ("motionTrajectory", 0.0f);
    setParameter ("motionDivision", 2.0f);
    setParameter ("morphEnable", 0.0f);
    setParameter ("morphX", 0.0f);
    setParameter ("morphY", 0.0f);
    // Safe neutral recall: v0.5 factory presets remain unchanged by Matter DNA.
    setParameter ("dnaEnable", 0.0f);
    setParameter ("dnaDepth", 0.5f);
    setParameter ("dnaSensitivity", 0.5f);
    setParameter ("dnaCrystalAttack", 0.8f);
    setParameter ("dnaVaporAir", 0.8f);
    setParameter ("dnaEnergyMotion", 0.4f);
    setParameter ("dnaSpectralTone", 0.25f);
    setParameter ("input", 0.0f);
    setParameter ("output", aggressive ? -4.0f : -1.5f);
    setParameter ("bypass", 0.0f);
    setParameter ("macroTransform", 0.5f);
    setParameter ("macroMotion", 0.5f);
    setParameter ("macroEnergy", aggressive ? 0.76f : 0.42f + t * 0.2f);
    setParameter ("macroDensity", ambient ? 0.75f : 0.3f + 0.4f * t);
    setParameter ("macroSpace", 0.5f);
    setParameter ("macroHarmonics", 0.5f);
    setParameter ("macroEvolution", 0.5f);
    setParameter ("macroFusion", 0.5f);
    // Factory presets v0.1 predate the twelve-property system. Reset new controls to
    // neutral on recall so existing presets sound stable; individual sound design is pending.
    for (int i = 1; i <= 12; ++i)
    {
        const auto suffix = juce::String (i).paddedLeft ('0', 2);
        setParameter ((juce::String ("aShape") + suffix).toRawUTF8(), 0.5f);
        setParameter ((juce::String ("bShape") + suffix).toRawUTF8(), 0.5f);
    }
    // A factory preset clears any displayed user-preset identity in host state.
    parameters.state.removeProperty ("customPresetName", nullptr);
    currentPreset.store (index);
    // New factory patch starts with all four scenes matching its sound.
    for (int i = 0; i < 4; ++i) captureMorphScene (i);
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

juce::AudioProcessorEditor* OrganixAudioProcessor::createEditor() { return new OrganixAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrganixAudioProcessor(); }
