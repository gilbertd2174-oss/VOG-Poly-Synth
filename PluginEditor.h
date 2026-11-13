#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <vector>

class VOGPolySynthProAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VOGPolySynthProAudioProcessorEditor(VOGPolySynthProAudioProcessor&);
    ~VOGPolySynthProAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VOGPolySynthProAudioProcessor& processor;
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::Label>> knobLabels;
    std::vector<std::unique_ptr<SliderAttachment>> knobAttachments;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<ButtonAttachment>> toggleAttachments;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    juce::TextButton savePatch { "SAVE PATCH" }, loadPatch { "LOAD PATCH" }, record { "RECORD" }, saveWav { "SAVE WAV" }, panic { "PANIC" };
    juce::ToggleButton hold { "HOLD" };
    juce::Label status;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard { keyboardState,juce::MidiKeyboardComponent::horizontalKeyboard };
    std::unique_ptr<juce::FileChooser> chooser;
    void addKnob(const juce::String& id,const juce::String& label,int x,int y,int size=66);
    void addToggle(const juce::String& id,int x,int y);
    void addCombo(const juce::String& id,const juce::StringArray& items,int x,int y,int w=140);
    void chooseFile(bool save,const juce::String& title,const juce::String& extension,std::function<void(const juce::File&)> callback);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VOGPolySynthProAudioProcessorEditor)
};
