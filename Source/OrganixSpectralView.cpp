#include "OrganixSpectralView.h"

namespace organix
{
namespace
{
constexpr float pi=3.14159265358979323846f;
constexpr float tau=2.0f*pi;
constexpr float epsilon=1.0e-9f;
}
SpectralViewAnalyzer::SpectralViewAnalyzer() noexcept
{
    prepare(48000.0);
}
void SpectralViewAnalyzer::prepare(double fs) noexcept
{
    rate=std::isfinite(fs)?std::clamp(fs,8000.0,384000.0):48000.0;
    fastCoefficient=1.0f-std::exp(-1.0f/static_cast<float>(rate*0.0015));
    slowCoefficient=1.0f-std::exp(-1.0f/static_cast<float>(rate*0.035));
    for(int i=0;i<fftSize;++i)
        hann[static_cast<size_t>(i)]=0.5f-
            0.5f*std::cos(tau*static_cast<float>(i)/(fftSize-1));
    for(int i=0;i<fftBins;++i)
    {
        const float theta=-tau*static_cast<float>(i)/fftSize;
        twiddles[static_cast<size_t>(i)]={std::cos(theta),std::sin(theta)};
    }
    const float nyquist=static_cast<float>(rate*0.5);
    const float minHz=30.0f;
    const float maxHz=std::min(20000.0f,nyquist);
    const float logarithmicRange=std::log(std::max(minHz+1.0f,maxHz)/minHz);
    for(int k=0;k<fftBins;++k)
    {
        const float hz=static_cast<float>(k*rate/fftSize);
        const float normalized=hz<=minHz?0.0f:
            std::log(hz/minHz)/logarithmicRange;
        binBand[static_cast<size_t>(k)]=static_cast<unsigned char>(
            std::clamp(static_cast<int>(normalized*(displayBands-1)),0,displayBands-1));
    }
    reset();
}
void SpectralViewAnalyzer::reset() noexcept
{
    for(size_t i=0;i<states.size();++i)
    {
        states[i]={};
        auto& out=readouts[i];
        for(auto& value:out.bands)value.store(0.0f,std::memory_order_relaxed);
        out.energy.store(0.0f,std::memory_order_relaxed);
        out.transient.store(0.0f,std::memory_order_relaxed);
        out.centroid.store(0.0f,std::memory_order_relaxed);
        out.brightness.store(0.0f,std::memory_order_relaxed);
        out.motion.store(0.0f,std::memory_order_relaxed);
    }
}
void SpectralViewAnalyzer::transform(State& s,Readout& out) noexcept
{
    for(int i=0;i<fftSize;++i)
    {
        const int idx=(s.writeIndex+i)&(fftSize-1);
        s.fft[static_cast<size_t>(i)]=
            {s.ring[static_cast<size_t>(idx)]*hann[static_cast<size_t>(i)],0.0f};
    }
    int j=0;
    for(int i=1;i<fftSize;++i)
    {
        int bit=fftSize>>1;
        for(;j&bit;bit>>=1)j^=bit;
        j^=bit;
        if(i<j)std::swap(s.fft[static_cast<size_t>(i)],
                         s.fft[static_cast<size_t>(j)]);
    }
    for(int length=2;length<=fftSize;length<<=1)
    {
        const int half=length>>1,step=fftSize/length;
        for(int block=0;block<fftSize;block+=length)
            for(int k=0;k<half;++k)
            {
                const auto value=s.fft[static_cast<size_t>(block+k+half)]*
                                  twiddles[static_cast<size_t>(k*step)];
                const auto base=s.fft[static_cast<size_t>(block+k)];
                s.fft[static_cast<size_t>(block+k)]=base+value;
                s.fft[static_cast<size_t>(block+k+half)]=base-value;
            }
    }
    std::array<float,displayBands> magnitudes {};
    float magnitudeTotal=0.0f,centroidSum=0.0f;
    float highEnergy=0.0f,totalEnergy=0.0f,flux=0.0f;
    const float nyquist=static_cast<float>(rate*0.5);
    for(int k=1;k<fftBins;++k)
    {
        const float freq=static_cast<float>(k*rate/fftSize);
        const float power=std::norm(s.fft[static_cast<size_t>(k)]);
        const float magnitude=std::sqrt(std::max(0.0f,power));
        const auto bandIndex=binBand[static_cast<size_t>(k)];
        magnitudes[static_cast<size_t>(bandIndex)]=
            std::max(magnitudes[static_cast<size_t>(bandIndex)],magnitude);
        centroidSum+=freq*magnitude;
        magnitudeTotal+=magnitude;
        totalEnergy+=power;
        if(freq>=3500.0f)highEnergy+=power;
        flux+=std::max(0.0f,magnitude-s.priorMagnitude[static_cast<size_t>(k)]);
        s.priorMagnitude[static_cast<size_t>(k)]=magnitude;
    }
    const float gate=clamp01(s.recentEnergy*8.0f);
    const float brightness=totalEnergy>epsilon
        ?clamp01(std::sqrt(highEnergy/totalEnergy))*gate:0.0f;
    const float centroid=magnitudeTotal>epsilon
        ?clamp01(centroidSum/(magnitudeTotal*nyquist))*gate:0.0f;
    const float motion=magnitudeTotal>epsilon
        ?clamp01(flux*2.0f/magnitudeTotal)*gate:0.0f;
    s.recentBrightness+=(brightness-s.recentBrightness)*0.22f;
    s.recentCentroid+=(centroid-s.recentCentroid)*0.22f;
    s.recentMotion+=(motion-s.recentMotion)*0.22f;
    for(int i=0;i<displayBands;++i)
    {
        // The measured amplitude is Hann-coherent-gain corrected. Plot
        // magnitude in a 78 dB display window, not arbitrary animation.
        const float amplitude=magnitudes[static_cast<size_t>(i)]*4.0f/fftSize;
        const float dB=20.0f*std::log10(std::max(1.0e-7f,amplitude));
        const float level=clamp01((dB+78.0f)/78.0f)*gate;
        float& previous=s.filteredBands[static_cast<size_t>(i)];
        previous+=(level-previous)*(level>previous?0.40f:0.18f);
        out.bands[static_cast<size_t>(i)].store(previous,std::memory_order_relaxed);
    }
    out.centroid.store(s.recentCentroid,std::memory_order_relaxed);
    out.brightness.store(s.recentBrightness,std::memory_order_relaxed);
    out.motion.store(s.recentMotion,std::memory_order_relaxed);
}
void SpectralViewAnalyzer::analyze(
    Stream stream,const float* left,const float* right,
    int samples,int channels) noexcept
{
    if(left==nullptr||samples<=0||channels<=0)return;
    const size_t idx=static_cast<size_t>(stream);
    auto& s=states[idx];
    auto& o=readouts[idx];
    float peakAttack=0.0f;
    double meanSquares=0.0;
    for(int i=0;i<samples;++i)
    {
        const float l=finite(left[i]);
        const float r=channels>=2&&right!=nullptr?finite(right[i]):l;
        const float mono=0.5f*(l+r);
        const float absolute=std::abs(mono);
        meanSquares+=0.5*(static_cast<double>(l)*l+static_cast<double>(r)*r);
        s.fastEnvelope+=(absolute-s.fastEnvelope)*fastCoefficient;
        s.slowEnvelope+=(absolute-s.slowEnvelope)*slowCoefficient;
        peakAttack=std::max(peakAttack,clamp01(
            (s.fastEnvelope-s.slowEnvelope)/(0.055f+s.slowEnvelope*1.4f)));
        s.ring[static_cast<size_t>(s.writeIndex)]=mono;
        s.writeIndex=(s.writeIndex+1)&(fftSize-1);
        s.validSamples=std::min(fftSize,s.validSamples+1);
        if(++s.hopCounter>=hopSize)
        {
            s.hopCounter=0;
            if(s.validSamples>=fftSize)transform(s,o);
        }
    }
    const float rms=std::sqrt(static_cast<float>(meanSquares/samples));
    s.recentEnergy+=(clamp01(rms*2.4f)-s.recentEnergy)*0.32f;
    s.recentTransient+=(peakAttack-s.recentTransient)*
        (peakAttack>s.recentTransient?0.42f:0.16f);
    o.energy.store(s.recentEnergy,std::memory_order_relaxed);
    o.transient.store(s.recentTransient,std::memory_order_relaxed);
}
float SpectralViewAnalyzer::band(Stream stream,int index) const noexcept
{
    if(index<0||index>=displayBands)return 0.0f;
    return readouts[static_cast<size_t>(stream)]
        .bands[static_cast<size_t>(index)].load(std::memory_order_relaxed);
}
float SpectralViewAnalyzer::transient(Stream stream) const noexcept
{
    return readouts[static_cast<size_t>(stream)].transient.load(std::memory_order_relaxed);
}
float SpectralViewAnalyzer::energy(Stream stream) const noexcept
{
    return readouts[static_cast<size_t>(stream)].energy.load(std::memory_order_relaxed);
}
float SpectralViewAnalyzer::centroid(Stream stream) const noexcept
{
    return readouts[static_cast<size_t>(stream)].centroid.load(std::memory_order_relaxed);
}
float SpectralViewAnalyzer::brightness(Stream stream) const noexcept
{
    return readouts[static_cast<size_t>(stream)].brightness.load(std::memory_order_relaxed);
}
float SpectralViewAnalyzer::motion(Stream stream) const noexcept
{
    return readouts[static_cast<size_t>(stream)].motion.load(std::memory_order_relaxed);
}
} // namespace organix
