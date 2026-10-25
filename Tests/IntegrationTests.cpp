#include "../Source/Processor.h"
#include <iostream>
#include <cstdlib>
static void check(bool ok,const char* s){if(!ok){std::cerr<<"FAIL: "<<s<<'\n';std::exit(1);}}
struct Host:juce::AudioPlayHead {
 double beat=0,bpm=120;bool playing=true;
 juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setPpqPosition(beat);p.setBpm(bpm);p.setIsPlaying(playing);return p;}
};
static double energy(const juce::AudioBuffer<float>& b){double e=0;for(int c=0;c<b.getNumChannels();++c)for(int n=0;n<b.getNumSamples();++n){float x=b.getSample(c,n);check(std::isfinite(x),"finite samples");e+=x*x;}return e;}
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 KernDrumProcessor p;Host host;p.setPlayHead(&host);p.setValue("enabled",0);p.prepareToPlay(48000,512);
 juce::AudioBuffer<float> b(2,512);juce::MidiBuffer midi;
 midi.addEvent(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),127);p.processBlock(b,midi);
 for(int n=0;n<127;++n)check(b.getSample(0,n)==0,"MIDI sample offset silence");check(energy(b)>.01,"MIDI triggers first voice");
 for(int v=1;v<8;++v){p.prepareToPlay(48000,512);midi.addEvent(juce::MidiMessage::noteOn(1,36+v,juce::uint8(100)),0);p.processBlock(b,midi);check(energy(b)>.001,"MIDI maps all eight voices");}
 p.setValue("v1_pitch",91);p.setValue("swing",.63f);p.setValue("pattern",2);p.setValue(KernDrumProcessor::stepID(2,7,15),.42f);p.setValue("v3_solo",1);
 juce::MemoryBlock saved;p.getStateInformation(saved);KernDrumProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));
 for(auto* parameter:p.getParameters()) {auto* a=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter);check(a!=nullptr,"identified parameters");check(std::abs(parameter->getValue()-restored.state.getParameter(a->paramID)->getValue())<1e-6,"all automation and pattern values round-trip");}
 p.copyPattern(2,3);check(std::abs(p.state.getRawParameterValue(KernDrumProcessor::stepID(3,7,15))->load()-.42f)<1e-6,"pattern copy");p.clearPattern(3);check(p.state.getRawParameterValue(KernDrumProcessor::stepID(3,7,15))->load()==0,"pattern clear");
 p.setValue("v3_solo",0);p.setValue("pattern",0);p.setValue("enabled",1);p.setValue("swing",0);p.prepareToPlay(48000,512);host.beat=0;p.processBlock(b,midi);check(energy(b)>.001,"host starts sequencer at zero");
 host.beat=2.25;p.processBlock(b,midi);check(p.currentStep.load()==9,"host seek follows PPQ");host.beat=0;p.processBlock(b,midi);check(p.currentStep.load()==0,"host loop returns to step one");
 host.playing=false;p.processBlock(b,midi);check(p.currentStep.load()==-1,"transport stop disables steps");
 // Host automation uses exactly the same normalized parameter path as a DAW.
 p.state.getParameter("v1_pitch")->setValueNotifyingHost(.8f);check(std::abs(p.state.getParameter("v1_pitch")->getValue()-.8f)<1e-6,"normalized host automation");
 p.setStateInformation("bad",3);check(std::isfinite(p.state.getRawParameterValue("v1_pitch")->load()),"bad state safely ignored");
 {std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->setSize(960,720);editor->setSize(1600,1100);check(editor->getWidth()==1600,"editor creates and resizes");
  if(argc>2){p.loadKit(0);editor->setSize(1120,800);editor->setVisible(true);auto image=editor->createComponentSnapshot(editor->getLocalBounds());auto stream=juce::File(juce::String::fromUTF8(argv[2])).createOutputStream();check(stream!=nullptr,"screenshot file opens");stream->setPosition(0);stream->truncate();juce::PNGImageFormat png;check(png.writeImageToStream(image,*stream),"editor screenshot renders");}
 }
 if(argc>1){
  juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> descriptions;
  format.findAllTypesForFile(descriptions,juce::String::fromUTF8(argv[1]));check(descriptions.size()==1,"VST3 bundle scans as one instrument");
  juce::String error;auto plugin=format.createInstanceFromDescription(*descriptions[0],48000,512,error);
  if(!plugin)std::cerr<<error<<'\n';check(plugin!=nullptr,"built VST3 loads into actual plugin host");
  plugin->setPlayHead(&host);plugin->prepareToPlay(48000,512);midi.addEvent(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),32);plugin->processBlock(b,midi);check(energy(b)>.001,"loaded VST3 renders MIDI audio");
  juce::MemoryBlock pluginState;plugin->getStateInformation(pluginState);plugin->setStateInformation(pluginState.getData(),int(pluginState.getSize()));
  {std::unique_ptr<juce::AudioProcessorEditor> ed(plugin->createEditor());check(ed!=nullptr,"loaded VST3 editor opens");}
  plugin->releaseResources();std::cout<<"PASS: built VST3 scans, loads, renders MIDI, saves/restores state and creates editor\n";
 }
 std::cout<<"PASS: MIDI offsets and eight mappings, complete state round-trip, patterns, transport/seek/loop, automation, editor\n";
}
