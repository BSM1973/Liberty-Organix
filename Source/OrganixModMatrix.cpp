#include "OrganixModMatrix.h"
#include "OrganixMovement.h"
#include <algorithm>
#include <cmath>

namespace organix
{
namespace
{
constexpr float tau = 6.2831853071795864769f;
float limit (float value, float lo, float hi, float fallback = 0.0f) noexcept
{
    return std::isfinite (value) ? std::clamp (value, lo, hi) : fallback;
}
void applyLayer (Parameters& p, const std::array<float,MatrixOffsets::count>& o,
                 int base) noexcept
{
    p.amount   = limit (p.amount   + o[static_cast<size_t> (base+0)],0.0f,1.0f);
    p.motion   = limit (p.motion   + o[static_cast<size_t> (base+1)],0.0f,1.0f);
    p.tone     = limit (p.tone     + o[static_cast<size_t> (base+2)],0.0f,1.0f);
    p.feedback = limit (p.feedback + o[static_cast<size_t> (base+3)],0.0f,0.88f);
    p.density  = limit (p.density  + o[static_cast<size_t> (base+4)],0.0f,1.0f);
    p.energy   = limit (p.energy   + o[static_cast<size_t> (base+5)],0.0f,1.0f);
    for (int j=0; j<12; ++j)
    {
        const float delta = o[static_cast<size_t> (base + 6 + j)];
        p.shape[static_cast<size_t> (j)] =
            limit (p.shape[static_cast<size_t> (j)] + delta,0.0f,1.0f);
    }
    // Organix v0.9 uses shape01..06 directly. A modulation routed to
    // shape07..12 must still change SOUND, so the advanced controls drive
    // additional restrained offsets of the six primary DSP parameters.
    p.amount   = limit (p.amount   + o[static_cast<size_t> (base+12)]*0.28f,0.0f,1.0f);
    p.motion   = limit (p.motion   + o[static_cast<size_t> (base+13)]*0.28f,0.0f,1.0f);
    p.tone     = limit (p.tone     + o[static_cast<size_t> (base+14)]*0.28f,0.0f,1.0f);
    p.feedback = limit (p.feedback + o[static_cast<size_t> (base+15)]*0.17f,0.0f,0.88f);
    p.density  = limit (p.density  + o[static_cast<size_t> (base+16)]*0.28f,0.0f,1.0f);
    p.energy   = limit (p.energy   + o[static_cast<size_t> (base+17)]*0.28f,0.0f,1.0f);
}
constexpr const char* sourcesEN[] = {
    "OFF","LFO 1","LFO 2","LFO 3","LFO 4","ENVELOPE 1",
    "ENVELOPE 2","INPUT LEVEL","TRANSIENTS","SPECTRAL AIR",
    "SPECTRAL MOTION","RANDOM HOLD","RANDOM GLIDE",
    "MACRO TRANSFORM","MACRO MOTION","MACRO ENERGY","MACRO DENSITY",
    "MACRO SPACE","MACRO HARMONICS","MACRO EVOLUTION","MACRO FUSION"
};
constexpr const char* sourcesFR[] = {
    "AUCUNE","LFO 1","LFO 2","LFO 3","LFO 4","ENVELOPPE 1",
    "ENVELOPPE 2","NIVEAU AUDIO","TRANSITOIRES","SOUFFLE SPECTRAL",
    "MOUVEMENT SPECTRAL","ALÉATOIRE FIXE","ALÉATOIRE GLISSÉ",
    "MACRO TRANSFORMER","MACRO MOUVEMENT","MACRO ÉNERGIE","MACRO DENSITÉ",
    "MACRO ESPACE","MACRO HARMONIQUES","MACRO ÉVOLUTION","MACRO FUSION"
};
constexpr const char* destEN[] = {
    "OFF",
    "A AMOUNT","A MOTION","A TONE","A FEEDBACK","A DENSITY","A ENERGY",
    "A SHAPE 01","A SHAPE 02","A SHAPE 03","A SHAPE 04","A SHAPE 05","A SHAPE 06",
    "A SHAPE 07","A SHAPE 08","A SHAPE 09","A SHAPE 10","A SHAPE 11","A SHAPE 12",
    "B AMOUNT","B MOTION","B TONE","B FEEDBACK","B DENSITY","B ENERGY",
    "B SHAPE 01","B SHAPE 02","B SHAPE 03","B SHAPE 04","B SHAPE 05","B SHAPE 06",
    "B SHAPE 07","B SHAPE 08","B SHAPE 09","B SHAPE 10","B SHAPE 11","B SHAPE 12",
    "FUSION","BALANCE","DRY / WET","STEREO WIDTH"
};
constexpr const char* destFR[] = {
    "AUCUNE",
    "A INTENSITÉ","A MOUVEMENT","A TIMBRE","A RÉINJECTION","A DENSITÉ","A ÉNERGIE",
    "A FORME 01","A FORME 02","A FORME 03","A FORME 04","A FORME 05","A FORME 06",
    "A FORME 07","A FORME 08","A FORME 09","A FORME 10","A FORME 11","A FORME 12",
    "B INTENSITÉ","B MOUVEMENT","B TIMBRE","B RÉINJECTION","B DENSITÉ","B ÉNERGIE",
    "B FORME 01","B FORME 02","B FORME 03","B FORME 04","B FORME 05","B FORME 06",
    "B FORME 07","B FORME 08","B FORME 09","B FORME 10","B FORME 11","B FORME 12",
    "FUSION","ÉQUILIBRE","SEC / EFFET","LARGEUR STÉRÉO"
};
static_assert (std::size (sourcesEN) == MatrixSettings::sourceCount);
static_assert (std::size (destEN) == MatrixSettings::destinationCount);
}
const char* ModulationMatrix::sourceName (int source, bool french) noexcept
{
    source=std::clamp (source,0,MatrixSettings::sourceCount-1);
    return french ? sourcesFR[source] : sourcesEN[source];
}
const char* ModulationMatrix::destinationName (int destination, bool french) noexcept
{
    destination=std::clamp (destination,0,MatrixSettings::destinationCount-1);
    return french ? destFR[destination] : destEN[destination];
}
void MatrixOffsets::apply (Parameters& a, Parameters& b) const noexcept
{
    applyLayer (a,destinations,1);
    applyLayer (b,destinations,19);
}
void MatrixOffsets::apply (Parameters& a, Parameters& b, float& fusion,
                           float& balance, float& wet, float& width) const noexcept
{
    apply (a,b);
    fusion=limit (fusion+destinations[37],0.0f,1.0f);
    balance=limit (balance+destinations[38],0.0f,1.0f);
    wet=limit (wet+destinations[39],0.0f,1.0f);
    width=limit (width+2.0f*destinations[40],0.0f,2.0f);
}
void ModulationMatrix::prepare (double fs) noexcept
{
    rate=limit (static_cast<float> (fs),8000.0f,384000.0f,48000.0f);
    reset();
}
void ModulationMatrix::reset() noexcept
{
    phase.fill (0.0f);
    envelope={};
    fast=slow=rmsSmoothed=randomPhase=held=previousHeld=0.0f;
    rng=0x9E3779B9u;
    sources.fill (0.0f);
    smoothedSlots.fill (0.0f);
    latest={};
}
float ModulationMatrix::nextRandom() noexcept
{
    rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
    return (static_cast<float>((rng>>8)&0xffffu)/32767.5f)-1.0f;
}
MatrixOffsets ModulationMatrix::processSegment (
    const MatrixSettings& settings, const float* left, const float* right,
    int start, int frames, int channels, float bpm,
    MatterDNA::Features spectral, const std::array<float,8>& macros) noexcept
{
    if (!settings.enabled || left==nullptr || channels<=0 || frames<=0)
    {
        if (!settings.enabled) { smoothedSlots.fill (0.0f); latest={}; }
        return latest;
    }
    const float fs=rate;
    const int count=std::max (1,frames);
    // Audio triggers two independent attack/decay envelopes. A refractory
    // window prevents single fast oscillations from retriggering each cycle.
    const float fastCoeff=1.0f-std::exp(-1.0f/(0.003f*fs));
    const float slowCoeff=1.0f-std::exp(-1.0f/(0.060f*fs));
    float sumSquares=0.0f, peakTransient=0.0f;
    for(int e=0;e<2;++e)
    {
        envelope[e].refractory=std::max(0,envelope[e].refractory-count);
    }
    for(int i=0;i<count;++i)
    {
        const float l=limit(left[start+i],-2.0f,2.0f);
        const float r=channels>=2&&right!=nullptr
                      ?limit(right[start+i],-2.0f,2.0f):l;
        const float x=0.5f*(l+r);
        const float absolute=std::abs(x);
        sumSquares+=0.5f*(l*l+r*r);
        fast+=(absolute-fast)*fastCoeff;
        slow+=(absolute-slow)*slowCoeff;
        const float onset=limit((fast-slow)*4.5f,0.0f,1.0f);
        peakTransient=std::max(peakTransient,onset);
        if(onset>0.085f)
        {
            for(auto& env:envelope)
                if(env.refractory<=0)
                {
                    env.rising=true;
                    env.level=0.0f;
                    env.refractory=static_cast<int>(fs*0.095f);
                }
        }
        for(int e=0;e<2;++e)
        {
            auto& env=envelope[e];
            const float attack=limit(settings.envAttack[e],0.001f,0.5f,0.015f);
            const float decay=limit(settings.envDecay[e],0.005f,3.0f,0.35f);
            if(env.rising)
            {
                env.level+= (1.0f-env.level)* std::min(1.0f,1.0f/(fs*attack));
                if(env.level>=0.98f) env.rising=false;
            }
            else
                env.level*=std::max(0.0f,1.0f-1.0f/(fs*decay));
            env.level=limit(env.level,0.0f,1.0f);
        }
    }
    const float rms=std::sqrt (sumSquares/static_cast<float>(count));
    rmsSmoothed+= (limit(rms*2.4f,0.0f,1.0f)-rmsSmoothed)*0.28f;
    // Four independent oscillators, optionally multiplied by a beat division.
    const float validBpm=limit(bpm,20.0f,300.0f,120.0f);
    for(int i=0;i<4;++i)
    {
        const float configured=limit(settings.rates[i],0.02f,12.0f,0.2f);
        const float hz=settings.tempoSync
            ? Movement::frequency(false,configured,validBpm,
                                  std::clamp(settings.division,0,6))*configured
            : configured;
        phase[static_cast<size_t>(i)]+=
            hz*static_cast<float>(count)/fs;
        phase[static_cast<size_t>(i)]-=
            std::floor(phase[static_cast<size_t>(i)]);
        sources[static_cast<size_t>(i+1)] =
            std::sin(tau*phase[static_cast<size_t>(i)]);
    }
    sources[5]=envelope[0].level;
    sources[6]=envelope[1].level;
    sources[7]=limit(rmsSmoothed,0.0f,1.0f);
    sources[8]=limit(std::max(peakTransient,spectral.transient),0.0f,1.0f);
    sources[9]=limit(spectral.air,0.0f,1.0f);
    sources[10]=limit(spectral.motion,0.0f,1.0f);
    const float randomHz=limit(settings.randomRate,0.05f,10.0f,1.0f);
    randomPhase+=randomHz*static_cast<float>(count)/fs;
    if(randomPhase>=1.0f)
    {
        randomPhase-=std::floor(randomPhase);
        previousHeld=held;
        held=nextRandom();
    }
    sources[11]=held;
    sources[12]=previousHeld+(held-previousHeld)*randomPhase;
    for (int m=0; m<8; ++m)
        sources[static_cast<size_t>(13+m)] =
            limit(macros[static_cast<size_t>(m)],0.0f,1.0f,0.5f);
    latest={};
    const float smooth=1.0f-std::exp(
        -static_cast<float>(count)/(fs*0.012f));
    for(int k=0;k<MatrixSettings::slotCount;++k)
    {
        const auto& slot=settings.slots[static_cast<size_t>(k)];
        const bool validSlot=slot.enabled && slot.source>0 &&
                            slot.source<MatrixSettings::sourceCount &&
                            slot.destination>0 &&
                            slot.destination<MatrixSettings::destinationCount;
        float target=0.0f;
        if(validSlot)
        {
            const float src=sources[static_cast<size_t>(slot.source)];
            const bool oscillator=slot.source<=4 || slot.source==11 || slot.source==12;
            const float normalized=slot.bipolar
                ? (oscillator ? src : (2.0f*src-1.0f))
                : (oscillator ? 0.5f*(src+1.0f) : src);
            target=limit(slot.depth,-1.0f,1.0f)*normalized*
                   limit(settings.masterDepth,0.0f,1.0f,0.8f);
        }
        float& smoothValue=smoothedSlots[static_cast<size_t>(k)];
        smoothValue+=(target-smoothValue)*smooth;
        if(!validSlot) continue;
        latest.destinations[static_cast<size_t>(slot.destination)] += smoothValue;
        latest.activity[static_cast<size_t>(k)]=smoothValue;
    }
    for(auto& d:latest.destinations)
        d=limit(d,-1.0f,1.0f);
    return latest;
}
} // namespace organix
