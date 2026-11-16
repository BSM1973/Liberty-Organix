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
    wetSmooth.reset (sampleRate, 0.025);
    bypassSmooth.reset (sampleRate, 0.008);
    outputSmooth.reset (sampleRate, 0.025);
    wetSmooth.setCurrentAndTargetValue (raw (parameters, "drywet"));
    bypassSmooth.setCurrentAndTargetValue (raw (parameters, "bypass") > 0.5f ? 0.0f : 1.0f);
    outputSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw (parameters, "output")));
    setLatencySamples (0); // Current time-domain engines require no fixed algorithmic delay.
}

void OrganixAudioProcessor::releaseResources() { engineA.reset(); engineB.reset(); }

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
    engineA.configure (aMaterial, a);
    if (activeB) engineB.configure (bMaterial, b);

    const float inputGain = juce::Decibels::decibelsToGain (raw (parameters, "input"));
    wetSmooth.setTargetValue (raw (parameters, "drywet"));
    bypassSmooth.setTargetValue (raw (parameters, "bypass") > 0.5f ? 0.0f : 1.0f);
    outputSmooth.setTargetValue (juce::Decibels::decibelsToGain (raw (parameters, "output")));
    auto* left = buffer.getWritePointer (0);
    auto* right = channels == 2 ? buffer.getWritePointer (1) : nullptr;
    double inputSquare = 0.0, outputSquare = 0.0;
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float wet = wetSmooth.getNextValue();
        const float active = bypassSmooth.getNextValue();
        const float gain = outputSmooth.getNextValue();
        for (int ch = 0; ch < channels; ++ch)
        {
            auto& destination = ch == 0 ? left[i] : right[i];
            const float original = std::isfinite (destination) ? destination : 0.0f;
            const float x = original * inputGain;
            float y = x;
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
                // Cross-Fusion adds nonlinear interaction, not only a crossfade.
                if (routing == 3)
                    y += fusion * 0.27f * std::tanh (3.0f * xa * xb);
            }
            const float mixed = (x * (1.0f - wet) + y * wet) * gain;
            // Bounded output safety for experimental sounds (soft-knee tanh ceiling).
            const float out = std::abs (mixed) < 0.96f ? mixed : std::tanh (mixed) * 0.96f / std::tanh (0.96f);
            const float safeEffect = std::isfinite (out) ? juce::jlimit (-1.0f, 1.0f, out) : 0.0f;
            destination = original * (1.0f - active) + safeEffect * active;
            inputSquare += static_cast<double> (x) * x;
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
        }
    }
}

juce::AudioProcessorEditor* OrganixAudioProcessor::createEditor() { return new OrganixAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrganixAudioProcessor(); }
