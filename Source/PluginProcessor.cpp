#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace {
constexpr std::array<float,6> eqFrequencies { 60.f,150.f,400.f,1000.f,3000.f,8000.f };
float wave(int type, double p) {
    const float t = (float) p;
    switch (type) {
        case 0: return std::sin(juce::MathConstants<float>::twoPi * t);
        case 1: return 2.0f*t-1.0f;
        case 2: return t<0.5f ? 1.0f:-1.0f;
        case 3: return 1.0f-4.0f*std::abs(t-0.5f);
        case 4: return t<0.25f ? 1.0f : -1.0f;
        default: return 0.0f;
    }
}
}

juce::AudioProcessorValueTreeState::ParameterLayout VOGPolySynthProAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    for (int i=1; i<=6; ++i) {
        const auto prefix = "osc" + juce::String(i);
        p.push_back(std::make_unique<juce::AudioParameterBool>(prefix+"on", "OSC " + juce::String(i) + " On", true));
        p.push_back(std::make_unique<juce::AudioParameterChoice>(prefix+"wave", "OSC " + juce::String(i) + " Wave", juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse" }, (i-1)%5));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(prefix+"level", "OSC " + juce::String(i) + " Level", 0.0f, 1.0f, i==1 ? 0.55f : 0.20f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(prefix+"tune", "OSC " + juce::String(i) + " Tune", -24.0f, 24.0f, 0.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(prefix+"detune", "OSC " + juce::String(i) + " Detune", -100.0f, 100.0f, 0.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(prefix+"pan", "OSC " + juce::String(i) + " Pan", -1.0f, 1.0f, 0.0f));
    }
    p.push_back(std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.001f, 5.0f, 0.02f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", 0.01f, 5.0f, 0.25f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 0.7f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("release", "Release", 0.01f, 8.0f, 0.4f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("master", "Master Volume", 0.0f, 1.0f, 0.65f));
    for(int i=0;i<6;++i) p.push_back(std::make_unique<juce::AudioParameterFloat>("eq"+juce::String(i), "EQ " + juce::String((int)eqFrequencies[(size_t)i]) + " Hz", -18.f, 18.f, 0.f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("division", "Rhythmic Division", juce::StringArray { "1/2", "1/3", "1/4", "1/8", "1/16", "1/32" }, 3));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("tempo", "Tempo", 25.f, 240.f, 120.f));
    return { p.begin(),p.end() };
}

VOGPolySynthProAudioProcessor::VOGPolySynthProAudioProcessor()
: juce::AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),
  state(*this,nullptr,"PARAMETERS",createParameters()) {}

void VOGPolySynthProAudioProcessor::prepareToPlay(double sr, int) {
    sampleRate=sr;
    for(auto& v:voices) { v.amp.setSampleRate(sr); v.note=-1; }
    for(auto& channel:eq) for(auto& f:channel) f.reset();
    // Three-minute stereo capture, preallocated outside the audio callback.
    const int capacity = (int)std::ceil(sr*180.0);
    recordingBuffer.setSize(2,capacity,false,true,false);
    recordingBuffer.clear();
    recording.store(false);
    recordedFrames.store(0);
}

void VOGPolySynthProAudioProcessor::handleMidi(const juce::MidiMessage& m) {
    if(m.isNoteOn()) {
        Voice* selected=nullptr;
        for(auto& v:voices) if(v.note<0 || !v.amp.isActive()) { selected=&v;break; }
        if(!selected) selected=&*std::min_element(voices.begin(),voices.end(),[](auto& a,auto& b){return a.age<b.age;});
        selected->note=m.getNoteNumber(); selected->velocity=m.getFloatVelocity(); selected->down=true;
        selected->phase.fill(0.0); selected->age=juce::Time::getMillisecondCounterHiRes();
        selected->amp.setParameters({param("attack"),param("decay"),param("sustain"),param("release")});
        selected->amp.reset(); selected->amp.noteOn();
    } else if(m.isNoteOff()) {
        for(auto& v:voices) if(v.note==m.getNoteNumber()) { v.down=false; if(!pedal && !hold.load()) v.amp.noteOff(); }
    } else if(m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff();
    else if(m.isPitchWheel()) pitchWheel=(m.getPitchWheelValue()-8192)/8192.0;
    else if(m.isController()) {
        if(m.getControllerNumber()==1) modWheel=m.getControllerValue()/127.f;
        if(m.getControllerNumber()==64) { const bool next=m.getControllerValue()>=64; if(pedal && !next && !hold.load()) for(auto& v:voices) if(!v.down) v.amp.noteOff(); pedal=next; }
    }
}

void VOGPolySynthProAudioProcessor::allNotesOff() {
    for(auto& v:voices) { v.amp.reset(); v.note=-1;v.down=false; }
    pedal=false;
}

float VOGPolySynthProAudioProcessor::renderVoice(Voice& v) {
    if(v.note<0 || !v.amp.isActive()) return 0.f;
    const float envelope=v.amp.getNextSample();
    double out=0.0;
    for(int i=1;i<=6;++i) {
        const auto key="osc"+juce::String(i);
        if(param(key+"on")<0.5f) continue;
        const double semitone=(double)v.note-69.0+param(key+"tune")+param(key+"detune")/100.0+pitchWheel*2.0;
        const double frequency=440.0*std::pow(2.0,semitone/12.0);
        auto& phase=v.phase[(size_t)(i-1)];
        phase+=frequency/sampleRate;
        phase-=std::floor(phase);
        out+=wave((int)param(key+"wave"),phase)*param(key+"level")/6.0;
    }
    if(!v.amp.isActive()) v.note=-1;
    return (float)out*envelope*v.velocity;
}

void VOGPolySynthProAudioProcessor::updateEQ() {
    for(size_t i=0;i<6;++i) {
        auto coefficients=juce::IIRCoefficients::makePeakFilter(sampleRate,eqFrequencies[i],0.9,juce::Decibels::decibelsToGain(param("eq"+juce::String((int)i))));
        for(auto& channel:eq) channel[i].setCoefficients(coefficients);
    }
}

void VOGPolySynthProAudioProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    updateEQ();
    auto iterator=midi.cbegin();
    auto end=midi.cend();
    const int count=audio.getNumSamples();
    for(int s=0;s<count;++s) {
        while(iterator!=end && (*iterator).samplePosition<=s) { handleMidi((*iterator).getMessage());++iterator; }
        float sample=0.f;
        for(auto& v:voices) sample+=renderVoice(v);
        sample*=param("master");
        for(int ch=0;ch<juce::jmin(2,audio.getNumChannels());++ch) {
            float result=sample;
            for(auto& f:eq[(size_t)ch]) result=f.processSingleSampleRaw(result);
            audio.setSample(ch,s,std::tanh(result));
        }
    }
    if(recording.load()) {
        const int start=recordedFrames.load();
        const int length=juce::jmin(count,recordingBuffer.getNumSamples()-start);
        if(length>0) {
            const juce::ScopedLock lock(recordingLock);
            for(int ch=0;ch<2;++ch) recordingBuffer.copyFrom(ch,start,audio,juce::jmin(ch,audio.getNumChannels()-1),0,length);
            recordedFrames.store(start+length);
        }
        if(length<count) recording.store(false);
    }
}

void VOGPolySynthProAudioProcessor::setRecording(bool enabled) {
    if(!enabled) { recording.store(false);return; }
    const juce::ScopedLock lock(recordingLock);
    recording.store(false);
    recordedFrames.store(0);
    recording.store(true);
}

bool VOGPolySynthProAudioProcessor::saveRecording(const juce::File& file) {
    recording.store(false);
    const juce::ScopedLock lock(recordingLock);
    const int n=recordedFrames.load();
    if(n<=0) return false;
    auto stream=std::unique_ptr<juce::FileOutputStream>(file.createOutputStream());
    if(!stream) return false;
    juce::WavAudioFormat wav;
    auto writer=std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(stream.get(),sampleRate,2,24,{},0));
    if(!writer) return false;
    stream.release();
    return writer->writeFromAudioSampleBuffer(recordingBuffer,0,n);
}

void VOGPolySynthProAudioProcessor::savePatch(const juce::File& file) {
    if(auto xml=state.copyState().createXml()) file.replaceWithText(xml->toString());
}
bool VOGPolySynthProAudioProcessor::loadPatch(const juce::File& file) {
    auto xml=juce::parseXML(file);
    if(!xml || !xml->hasTagName(state.state.getType())) return false;
    state.replaceState(juce::ValueTree::fromXml(*xml));return true;
}
void VOGPolySynthProAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if(auto xml=state.copyState().createXml()) copyXmlToBinary(*xml,dest);
}
void VOGPolySynthProAudioProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor*
VOGPolySynthProAudioProcessor::createEditor()
{
    return new VOGPolySynthProAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() 
{ 
    return new VOGPolySynthProAudioProcessor(); 
}
