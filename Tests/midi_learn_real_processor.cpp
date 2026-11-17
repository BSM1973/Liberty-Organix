// Integration validation of real Organix MIDI CC Learn with JUCE APVTS.
#include "../Source/PluginProcessor.h"
#include <array>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
bool approx(float a, float b, float tolerance=0.015f) noexcept
{
    return std::abs(a-b)<=tolerance;
}
bool renderCC(OrganixAudioProcessor& plugin,int channel,int cc,int value)
{
    juce::AudioBuffer<float> audio(2,128);
    audio.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(channel,cc,value),0);
    const auto count=midi.getNumEvents();
    plugin.processBlock(audio,midi);
    plugin.flushMidiLearnUpdates(); // simulate the processor message-thread Timer.
    return count==midi.getNumEvents(); // CC forwarded to the DAW unchanged
}
float normalized(OrganixAudioProcessor& plugin,const char* id)
{
    auto* param=plugin.parameters.getParameter(id);
    if(!param)throw std::runtime_error(std::string("Missing parameter ")+id);
    return param->getValue();
}
int fail(int code,const char* message)
{
    std::cerr<<"FAIL MIDI Learn "<<code<<": "<<message<<"\n";
    return code;
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialize;
    auto processor=std::make_unique<OrganixAudioProcessor>();
    processor->setPlayConfigDetails(2,2,48000.0,128);
    processor->prepareToPlay(48000.0,128);
    if(processor->isMidiLearning())return fail(1,"Should not learn on startup");
    processor->startMidiLearn("drywet");
    if(!processor->isMidiLearning())return fail(2,"No armed CC Learn");
    if(!renderCC(*processor,2,74,100))return fail(3,"Lost incoming MIDI event");
    if(processor->isMidiLearning())return fail(4,"Learn did not stop");
    if(processor->midiLearnMapping("drywet")!="CH 2 / CC 74")
        return fail(5,"Wrong learned channel/CC");
    if(!approx(normalized(*processor,"drywet"),100.0f/127.0f))
        return fail(6,"Learned CC did not reach real APVTS parameter");
    const float saved=normalized(*processor,"drywet");
    // A long SysEx may require heap storage in juce::MidiMessage.
    // MIDI Learn must ignore it using raw MidiBuffer metadata only.
    {
        std::array<juce::uint8,192> payload {};
        for(size_t i=0;i<payload.size();++i)
            payload[i]=static_cast<juce::uint8>(i%128);
        juce::MidiBuffer sysex;
        sysex.addEvent(juce::MidiMessage::createSysExMessage(
                           payload.data(),static_cast<int>(payload.size())),0);
        juce::AudioBuffer<float> empty(2,128);
        empty.clear();
        processor->processBlock(empty,sysex);
        processor->flushMidiLearnUpdates();
        if(sysex.getNumEvents()!=1||!approx(normalized(*processor,"drywet"),saved,1.0e-5f))
            return fail(20,"Long SysEx should be forwarded untouched and not change CC mapping");
    }
    if(!renderCC(*processor,1,74,4))return fail(7,"MIDI event altered");
    if(!approx(normalized(*processor,"drywet"),saved,1.0e-5f))
        return fail(8,"Wrong MIDI channel was accepted");
    renderCC(*processor,2,74,42);
    if(!approx(normalized(*processor,"drywet"),42.0f/127.0f))
        return fail(9,"Subsequent CC failed");

    processor->startMidiLearn("morphX");
    renderCC(*processor,1,21,120);
    if(processor->midiLearnMapping("morphX")!="CH 1 / CC 21")
        return fail(10,"Second learned parameter not mapped");
    if(!approx(normalized(*processor,"morphX"),120.0f/127.0f))
        return fail(11,"Second mapped CC not applied");

    processor->loadFactoryPreset(31);
    if(processor->midiLearnMapping("drywet").isEmpty())
        return fail(12,"Program change erased MIDI Learn mapping");

    juce::MemoryBlock state;
    processor->getStateInformation(state);
    if(state.getSize()<32)return fail(13,"Empty DAW project snapshot");

    auto restored=std::make_unique<OrganixAudioProcessor>();
    restored->setPlayConfigDetails(2,2,48000.0,128);
    restored->prepareToPlay(48000.0,128);
    restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    if(restored->midiLearnMapping("drywet")!="CH 2 / CC 74" ||
       restored->midiLearnMapping("morphX")!="CH 1 / CC 21")
        return fail(14,"DAW session recall lost CC mapping");
    renderCC(*restored,2,74,33);
    if(!approx(normalized(*restored,"drywet"),33.0f/127.0f))
        return fail(15,"Restored mapping failed without editor");
    restored->clearMidiLearn("drywet");
    if(restored->midiLearnMapping("drywet").isNotEmpty())
        return fail(16,"Per-parameter clear failed");
    const float cleared=normalized(*restored,"drywet");
    renderCC(*restored,2,74,109);
    if(!approx(normalized(*restored,"drywet"),cleared,1.0e-5f))
        return fail(17,"Forgotten CC still affects parameter");
    restored->clearAllMidiLearn();
    if(restored->midiLearnMapping("morphX").isNotEmpty())
        return fail(18,"Clear all failed");
    restored->startMidiLearn("output");
    restored->cancelMidiLearn();
    if(restored->isMidiLearning())return fail(19,"Learn cancel failed");
    restored->releaseResources();
    processor->releaseResources();
    std::cout<<"PASS MIDI LEARN: CC channel isolation, value range, APVTS, "
                "DAW state recall, preset change, forget, cancel and no-editor processing\n";
    return 0;
}
