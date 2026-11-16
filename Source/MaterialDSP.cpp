#include "MaterialDSP.h"

namespace organix
{
void MaterialDSP::prepare (double sampleRate, int channels)
{
    fs = std::max (8000.0, sampleRate);
    delayLength = std::max (1024, static_cast<int> (std::ceil (fs * 2.0)));
    for (int c = 0; c < maxChannels; ++c)
    {
        state[c].delay.assign (static_cast<size_t> (delayLength), 0.0f);
        state[c].rng = 0xA328F3D1u ^ static_cast<unsigned> (c * 3571 + channels);
    }
    reset();
    updateModes();
}

void MaterialDSP::reset() noexcept
{
    for (auto& s : state)
    {
        std::fill (s.delay.begin(), s.delay.end(), 0.0f);
        s.write = 0;
        s.lp = s.hpLP = s.smooth = s.lastInput = 0.0f;
        s.phase = s.phase2 = s.feedbackSample = s.env = 0.0f;
        s.lastModeExcitation = 0.0f;
        std::fill (std::begin (s.oscX), std::end (s.oscX), 0.0f);
        std::fill (std::begin (s.oscY), std::end (s.oscY), 0.0f);
        s.samplesUntilGrain = 0;
        for (auto& g : s.grains) g = {};
    }
}

void MaterialDSP::configure (Material m, Parameters params) noexcept
{
    if (material != m)
    {
        material = m;
        // Existing buffers are retained: no large memory clearing in the audio callback.
    }
    p = params;
    p.amount   = clamp (p.amount, 0.0f, 1.0f);
    p.motion   = clamp (p.motion, 0.0f, 1.0f);
    p.tone     = clamp (p.tone, 0.0f, 1.0f);
    p.feedback = clamp (p.feedback, 0.0f, 0.90f);
    p.density  = clamp (p.density, 0.0f, 1.0f);
    p.energy   = clamp (p.energy, 0.0f, 1.0f);
    for (auto& v : p.shape) v = clamp (v, 0.0f, 1.0f);
    // Twelve normalized, host-automatable property controls.
    p.amount   = clamp (p.amount + 0.55f * (p.shape[0] - 0.5f), 0.0f, 1.0f);
    p.motion   = clamp (p.motion + 0.55f * (p.shape[1] - 0.5f), 0.0f, 1.0f);
    p.tone     = clamp (p.tone + 0.55f * (p.shape[2] - 0.5f), 0.0f, 1.0f);
    p.feedback = clamp (p.feedback + 0.30f * (p.shape[3] - 0.5f), 0.0f, 0.90f);
    p.density  = clamp (p.density + 0.55f * (p.shape[4] - 0.5f), 0.0f, 1.0f);
    p.energy   = clamp (p.energy + 0.55f * (p.shape[5] - 0.5f), 0.0f, 1.0f);
    updateModes();
}

void MaterialDSP::updateModes() noexcept
{
    constexpr std::array<float, modes> harmonic {1.0f, 1.50f, 2.01f, 2.99f};
    constexpr std::array<float, modes> metallic {1.0f, 1.4142f, 2.398f, 3.89f};
    const bool isMetal = material == Material::Metal;
    const float fundamental = (isMetal ? 105.0f : 190.0f) * std::pow (2.0f, 2.2f * p.tone);
    const float time = (isMetal ? 0.11f : 0.24f) + (isMetal ? 1.6f : 2.2f) * p.feedback;
    for (int i = 0; i < modes; ++i)
    {
        const float frequency = std::min (static_cast<float> (fs * 0.44), fundamental * (isMetal ? metallic[i] : harmonic[i]));
        const float w = 2.0f * pi * frequency / static_cast<float> (fs);
        modeSin[i] = std::sin (w);
        modeCos[i] = std::cos (w);
        modeDecay[i] = std::exp (-1.0f / (static_cast<float> (fs) * time / (1.0f + 0.42f * i)));
    }
}

float MaterialDSP::readAt (const ChannelState& s, float index) const noexcept
{
    const float length = static_cast<float> (delayLength);
    index = static_cast<float> (std::fmod (static_cast<double> (index), static_cast<double> (delayLength)));
    if (index < 0.0f) index += length;
    if (index >= length) index = 0.0f; // float rounding at long buffer boundaries
    const int a = std::clamp (static_cast<int> (index), 0, delayLength - 1);
    const int b = a + 1 == delayLength ? 0 : a + 1;
    const float frac = index - static_cast<float> (a);
    return s.delay[static_cast<size_t> (a)] * (1.0f - frac) + s.delay[static_cast<size_t> (b)] * frac;
}
float MaterialDSP::readDelay (const ChannelState& s, float samples) const noexcept
{
    return readAt (s, static_cast<float> (s.write) - samples);
}
float MaterialDSP::random01 (ChannelState& s) const noexcept
{
    s.rng ^= s.rng << 13;
    s.rng ^= s.rng >> 17;
    s.rng ^= s.rng << 5;
    return static_cast<float> (s.rng & 0x00FFFFFFu) * (1.0f / 16777216.0f);
}

float MaterialDSP::exciteModes (ChannelState& s, float excitation, bool metallic) noexcept
{
    float y = 0.0f;
    for (int i = 0; i < modes; ++i)
    {
        const float damp = modeDecay[i];
        const float nx = damp * (modeCos[i] * s.oscX[i] - modeSin[i] * s.oscY[i]) + excitation * (0.23f / (1.0f + 0.3f * i));
        const float ny = damp * (modeSin[i] * s.oscX[i] + modeCos[i] * s.oscY[i]);
        s.oscX[i] = nx;
        s.oscY[i] = ny;
        y += ny * (metallic ? (0.28f / (1.0f + i * 0.5f)) : (0.18f / (1.0f + i * 0.6f)));
    }
    return std::tanh (y * (0.5f + 2.0f * p.amount));
}

float MaterialDSP::liquid (ChannelState& s, float x) noexcept
{
    const float rate = 0.09f + 2.8f * p.motion;
    s.phase = wrapPhase (s.phase + 2.0f * pi * rate / static_cast<float> (fs));
    const float wiggle = 0.5f + 0.5f * sine (s.phase);
    const float offsetMs = 10.0f + 15.0f * p.tone + 18.0f * p.amount * wiggle;
    const float a = readDelay (s, offsetMs * static_cast<float> (fs) / 1000.0f);
    const float lpCoeff = 0.01f + 0.16f * p.tone;
    s.lp += lpCoeff * (a - s.lp);
    const float out = s.lp;
    s.delay[static_cast<size_t> (s.write)] = std::tanh (x + out * p.feedback * 0.55f);
    s.write = s.write + 1 == delayLength ? 0 : s.write + 1;
    return x * (1.0f - p.amount * 0.7f) + out * (p.amount * 0.83f);
}

float MaterialDSP::crystal (ChannelState& s, float x) noexcept
{
    const float excitation = (x - s.lastInput) * (0.5f + 0.8f * p.energy) + x * 0.035f;
    s.lastInput = x;
    return x * (1.0f - 0.65f * p.amount) + exciteModes (s, excitation, false);
}

float MaterialDSP::plasma (ChannelState& s, float x) noexcept
{
    const float cutoff = 140.0f + 960.0f * p.tone;
    const float coeff = 1.0f - std::exp (-2.0f * pi * cutoff / static_cast<float> (fs));
    s.lp += coeff * (x - s.lp);
    const float low = s.lp;
    const float high = x - low;
    const float drive = 1.0f + 18.0f * p.amount * (0.4f + p.energy);
    const float crunchy = (std::tanh (low * drive * 0.6f) + std::tanh (high * drive * 1.25f)) * 0.7f;
    s.phase = wrapPhase (s.phase + 2.0f * pi * (17.0f + 270.0f * p.motion) / static_cast<float> (fs));
    const float ring = 1.0f - 0.45f * p.motion + 0.45f * p.motion * sine (s.phase);
    return x * (1.0f - p.amount) + crunchy * ring * p.amount;
}

float MaterialDSP::vapor (ChannelState& s, float x) noexcept
{
    s.delay[static_cast<size_t> (s.write)] = x;
    const int grainLength = static_cast<int> (fs * (0.045 + p.tone * 0.18));
    if (--s.samplesUntilGrain <= 0)
    {
        s.samplesUntilGrain = std::max (48, static_cast<int> (fs * (0.065 - 0.055 * p.density)));
        for (auto& g : s.grains)
        {
            if (! g.active)
            {
                g.active = true;
                g.duration = std::max (64, grainLength);
                g.age = 0;
                g.readPos = static_cast<float> (s.write) - static_cast<float> (fs * (0.11 + 0.65 * random01 (s)));
                g.speed = 0.68f + 0.65f * random01 (s) + 0.15f * p.motion;
                break;
            }
        }
    }
    float cloud = 0.0f;
    int active = 0;
    for (auto& g : s.grains)
    {
        if (! g.active) continue;
        const float progress = static_cast<float> (g.age) / static_cast<float> (g.duration);
        const float window = 0.5f - 0.5f * std::cos (2.0f * pi * progress);
        cloud += readAt (s, g.readPos) * window;
        ++active;
        g.readPos += g.speed;
        if (++g.age >= g.duration) g.active = false;
    }
    s.write = s.write + 1 == delayLength ? 0 : s.write + 1;
    cloud *= active > 0 ? (0.9f / std::sqrt (static_cast<float> (active))) : 0.0f;
    s.lp += (0.008f + 0.12f * p.tone) * (cloud - s.lp);
    return x * (1.0f - p.amount) + (s.lp * 0.55f + cloud * 0.65f) * p.amount;
}

float MaterialDSP::metal (ChannelState& s, float x) noexcept
{
    const float transient = std::max (0.0f, std::abs (x) - s.env);
    s.env += (std::abs (x) - s.env) * 0.004f;
    const float impact = (x - s.lastInput) * (0.35f + p.energy) + transient * (0.35f + p.motion);
    s.lastInput = x;
    return x * (1.0f - p.amount * 0.65f) + exciteModes (s, impact, true);
}

float MaterialDSP::light (ChannelState& s, float x) noexcept
{
    // Harmonic exciter and high-frequency movement. FFT/spectral resynthesis is NOT included in v0.1.
    const float cutoff = 650.0f + 4200.0f * p.tone;
    const float coeff = 1.0f - std::exp (-2.0f * pi * cutoff / static_cast<float> (fs));
    s.hpLP += coeff * (x - s.hpLP);
    const float high = x - s.hpLP;
    const float harmonics = std::tanh (high * (1.0f + 8.0f * p.energy)) - high;
    s.phase = wrapPhase (s.phase + 2.0f * pi * (0.14f + 5.0f * p.motion) / static_cast<float> (fs));
    const float movement = 0.67f + 0.33f * sine (s.phase);
    s.delay[static_cast<size_t> (s.write)] = std::tanh (harmonics + p.feedback * s.feedbackSample);
    const float delayed = readDelay (s, (14.0f + 12.0f * movement) * static_cast<float> (fs) / 1000.0f);
    s.feedbackSample = delayed;
    s.write = s.write + 1 == delayLength ? 0 : s.write + 1;
    return x + p.amount * movement * (harmonics * 0.45f + delayed * 0.33f);
}

float MaterialDSP::process (int channel, float input) noexcept
{
    if (channel < 0 || channel >= maxChannels) return input;
    if (! std::isfinite (input)) input = 0.0f;
    auto& s = state[static_cast<size_t> (channel)];
    // 07–12 drive real, bounded audio-domain transformations. Later versions
    // will refine their *material-specific* algorithms and unit displays.
    const float transient = std::abs (input - s.smooth);
    const float driven = input * (1.0f + 0.45f * (p.shape[6] - 0.5f));
    float y = driven;
    switch (material)
    {
        case Material::Liquid:  y = liquid (s, driven); break;
        case Material::Crystal: y = crystal (s, driven); break;
        case Material::Plasma:  y = plasma (s, driven); break;
        case Material::Vapor:   y = vapor (s, driven); break;
        case Material::Metal:   y = metal (s, driven); break;
        case Material::Light:   y = light (s, driven); break;
    }
    if (! std::isfinite (y)) return 0.0f;
    // These small centered corrections ensure all material property indices
    // remain acoustically active even for the early engines that don't yet
    // consume a matching specialized DSP field.
    s.phase2 = wrapPhase (s.phase2 + 2.0f * pi *
                           (0.15f + 3.0f * p.shape[1]) / static_cast<float> (fs));
    y *= 1.0f + 0.12f * (p.shape[1] - 0.5f) * std::sin (s.phase2);
    y += (p.shape[3] - 0.5f) * 0.09f * (y - input);
    y += (p.shape[4] - 0.5f) * 0.10f * (std::tanh (y * 1.9f) - y);
    y += (p.shape[5] - 0.5f) * 0.11f * (std::tanh (y * 3.2f) - y);
    s.smooth += (y - s.smooth) * 0.095f;
    y += (p.shape[7] - 0.5f) * 0.52f * (y - s.smooth);
    y += (p.shape[8] - 0.5f) * 0.22f * (y - input) * clamp (transient * 3.0f, 0.0f, 1.0f);
    y += (p.shape[9] - 0.5f) * 0.35f * (std::tanh (y * 2.4f) - y);
    const float level = 1.0f + (p.shape[10] - 0.5f) * 0.50f;
    const float stereo = (p.shape[11] - 0.5f) * (channel == 1 ? 0.42f : -0.42f);
    y *= level * (1.0f + stereo);
    return std::isfinite (y) ? clamp (y, -4.0f, 4.0f) : 0.0f;
}
}
