#include "PluginEditor.h"

VOGPolySynthProAudioProcessorEditor::VOGPolySynthProAudioProcessorEditor(VOGPolySynthProAudioProcessor& p)
: juce::AudioProcessorEditor(&p),processor(p)
{
    setSize(1540,840);
    for(int i=1;i<=6;++i) {
        const int x=20+(i-1)*252;
        const auto prefix="osc"+juce::String(i);
        addToggle(prefix+"on",x+12,139);
        addCombo(prefix+"wave",{"Sine","Saw","Square","Triangle","Pulse"},x+89,140,142);
        addKnob(prefix+"level","LEVEL",x+10,190);
        addKnob(prefix+"tune","TUNE",x+89,190);
        addKnob(prefix+"detune","DETUNE",x+169,190);
        addKnob(prefix+"pan","PAN",x+89,280);
    }
    const juce::StringArray adsrNames { "attack","decay","sustain","release" };
    for(int i=0;i<4;++i) addKnob(adsrNames[i],adsrNames[i].toUpperCase(),30+i*91,420,74);
    for(int i=0;i<6;++i) addKnob("eq"+juce::String(i),juce::StringArray{"60 Hz","150 Hz","400 Hz","1 kHz","3 kHz","8 kHz"}[i],420+i*96,420,74);
    addKnob("master","MASTER",1040,420,100);
    addKnob("tempo","TEMPO",1158,420,80);
    addCombo("division",{"1/2","1/3","1/4","1/8","1/16","1/32"},1275,460,140);
    for(auto* button:{&savePatch,&loadPatch,&record,&saveWav,&panic}) { addAndMakeVisible(button);button->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff12365b)); }
    addAndMakeVisible(hold);
    addAndMakeVisible(status);
    status.setColour(juce::Label::textColourId,juce::Colours::lightblue);
    status.setText("Ready • USB MIDI input is routed through your DAW",juce::dontSendNotification);
    addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(36,96);
    keyboard.setKeyWidth(24.f);
    hold.onClick=[this] { processor.setHold(hold.getToggleState()); };
    record.onClick=[this] { processor.setRecording(!processor.isRecording()); record.setButtonText(processor.isRecording()?"STOP RECORD":"RECORD"); };
    saveWav.onClick=[this] { chooseFile(true,"Save WAV recording","*.wav",[this](const juce::File& f) { status.setText(processor.saveRecording(f)?"Saved WAV: "+f.getFileName():"No recording available",juce::dontSendNotification); }); };
    savePatch.onClick=[this] { chooseFile(true,"Save named patch","*.vogpatch",[this](const juce::File& f) { processor.savePatch(f);status.setText("Saved patch: "+f.getFileName(),juce::dontSendNotification); }); };
    loadPatch.onClick=[this] { chooseFile(false,"Load patch","*.vogpatch",[this](const juce::File& f) { status.setText(processor.loadPatch(f)?"Loaded patch: "+f.getFileName():"Invalid patch file",juce::dontSendNotification); }); };
    panic.onClick=[this] { processor.allNotesOff();status.setText("All notes stopped",juce::dontSendNotification); };
    startTimerHz(5);
}

void VOGPolySynthProAudioProcessorEditor::addKnob(const juce::String& id,const juce::String& name,int x,int y,int size) {
    auto s=std::make_unique<juce::Slider>();
    s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,size+12,17);
    s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff29a7fa));
    s->setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff253b52));
    s->setBounds(x,y,size,size+24);
    addAndMakeVisible(*s);
    auto label=std::make_unique<juce::Label>();
    label->setText(name,juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId,juce::Colours::lightblue);
    label->setBounds(x-5,y+size+20,size+10,20);
    addAndMakeVisible(*label);
    knobAttachments.push_back(std::make_unique<SliderAttachment>(processor.state,id,*s));
    knobs.push_back(std::move(s));knobLabels.push_back(std::move(label));
}
void VOGPolySynthProAudioProcessorEditor::addToggle(const juce::String& id,int x,int y) {
    auto b=std::make_unique<juce::ToggleButton>("ON");
    b->setBounds(x,y,74,32);
    b->setColour(juce::ToggleButton::tickColourId,juce::Colours::deepskyblue);
    addAndMakeVisible(*b);
    toggleAttachments.push_back(std::make_unique<ButtonAttachment>(processor.state,id,*b));
    toggles.push_back(std::move(b));
}
void VOGPolySynthProAudioProcessorEditor::addCombo(const juce::String& id,const juce::StringArray& items,int x,int y,int w) {
    auto c=std::make_unique<juce::ComboBox>();
    c->addItemList(items,1);c->setBounds(x,y,w,30);
    addAndMakeVisible(*c);
    comboAttachments.push_back(std::make_unique<ComboAttachment>(processor.state,id,*c));
    combos.push_back(std::move(c));
}
void VOGPolySynthProAudioProcessorEditor::chooseFile(bool save,const juce::String& title,const juce::String& extension,std::function<void(const juce::File&)> callback) {
    chooser=std::make_unique<juce::FileChooser>(title,juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),extension);
    chooser->launchAsync(save?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles:juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this,callback=std::move(callback)](const juce::FileChooser& fc) {
        auto f=fc.getResult();
        if(f!=juce::File{}) callback(f);
    });
}
void VOGPolySynthProAudioProcessorEditor::timerCallback() {
    if(processor.isRecording()) status.setText("RECORDING WAV • "+juce::String(processor.recordedSeconds(),1)+" / 180 seconds",juce::dontSendNotification);
    else if(record.getButtonText()=="STOP RECORD") { record.setButtonText("RECORD");status.setText("Recording complete • click SAVE WAV",juce::dontSendNotification); }
}
void VOGPolySynthProAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff080e17));
    g.setColour(juce::Colour(0xff643e25));g.fillRect(0,0,12,getHeight());g.fillRect(getWidth()-12,0,12,getHeight());
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff123a61),0,0,juce::Colour(0xff070f1c),0,120,false));g.fillRoundedRectangle(16,12,1508,100,13);
    g.setColour(juce::Colours::white);g.setFont(juce::FontOptions(38.f,juce::Font::bold));g.drawText("VOG POLY SYNTH PRO",34,24,700,50,juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(17.f));g.setColour(juce::Colours::lightskyblue);g.drawText("Davoe Instruments  •  6 OSC  •  VST3 / STANDALONE",36,75,800,26,juce::Justification::centredLeft);
    for(int i=0;i<6;++i) {
        int x=20+i*252;
        g.setColour(juce::Colour(0xff0f2033));g.fillRoundedRectangle(x,120,244,258,10);
        g.setColour(juce::Colour(0xff3378a8));g.drawRoundedRectangle(x,120,244,258,10,1.2f);
        g.setColour(juce::Colours::lightblue);g.setFont(juce::FontOptions(22.f,juce::Font::bold));g.drawText("OSC "+juce::String(i+1),x+10,121,210,28,juce::Justification::centred);
    }
    g.setColour(juce::Colour(0xff10263c));g.fillRoundedRectangle(20,395,1500,175,10);
    g.setColour(juce::Colours::lightblue);g.setFont(juce::FontOptions(18.f,juce::Font::bold));
    g.drawText("AMP ENVELOPE",30,396,320,26,juce::Justification::centredLeft);
    g.drawText("6-BAND EQUALIZER",420,396,570,26,juce::Justification::centredLeft);
    g.drawText("MASTER / TEMPO / DIVISION",1030,396,450,26,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff10263c));g.fillRoundedRectangle(20,585,1500,80,10);
    g.setColour(juce::Colours::lightblue);g.setFont(juce::FontOptions(15.f));g.drawText("PATCH / RECORDING / HOLD",30,586,550,25,juce::Justification::centredLeft);
}
void VOGPolySynthProAudioProcessorEditor::resized() {
    savePatch.setBounds(35,618,145,36);loadPatch.setBounds(190,618,145,36);
    record.setBounds(355,618,145,36);saveWav.setBounds(510,618,145,36);
    hold.setBounds(675,620,110,34);panic.setBounds(795,618,120,36);
    status.setBounds(930,616,560,40);
    keyboard.setBounds(22,680,1496,145);
}
