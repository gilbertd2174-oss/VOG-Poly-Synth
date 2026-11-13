#pragma once
#include <JuceHeader.h>
#include <array>

class VOGPolySynthProAudioProcessor : public juce::AudioProcessor
{
public:
    VOGPolySynthProAudioProcessor();
    ~VOGPolySynthProAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "VOG Poly Synth Pro"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Current Patch"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState state;
    void setHold(bool b) { hold.store(b); }
    bool getHold() const { return hold.load(); }
    void setRecording(bool enabled);
    bool isRecording() const { return recording.load(); }
    double recordedSeconds() const { return recordedFrames.load() / juce::jmax(1.0, sampleRate); }
    bool saveRecording(const juce::File&);
    void savePatch(const juce::File&);
    bool loadPatch(const juce::File&);
    void allNotesOff();
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
private:
    struct Voice
    {
        int note = -1;
        float velocity = 0;
        bool down = false;
        std::array<double,6> phase {};
        juce::ADSR amp;
        double age = 0;
    };
    std::array<Voice,16> voices;
    double sampleRate = 44100.0;
    double pitchWheel = 0.0;
    float modWheel = 0.0f;
    bool pedal = false;
    std::atomic<bool> hold { false };
    std::atomic<bool> recording { false };
    std::atomic<int> recordedFrames { 0 };
    juce::AudioBuffer<float> recordingBuffer;
    juce::CriticalSection recordingLock;
    std::array<std::array<juce::IIRFilter,6>,2> eq;
    juce::MidiKeyboardState keyboardState;
    void handleMidi(const juce::MidiMessage&);
    float renderVoice(Voice&);
    void updateEQ();
    float param(const juce::String& id) const { return state.getRawParameterValue(id)->load(); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VOGPolySynthProAudioProcessor)
};
