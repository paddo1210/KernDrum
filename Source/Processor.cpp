#include "Processor.h"
#include "Editor.h"
namespace {
constexpr std::array<const char*,kern::numControls> keys {"pitch","bend","bendTime","decay","mix","cutoff","highpass","drive","level","pan","mute","solo","wave","clap"};
constexpr std::array<const char*,kern::numControls> titles {"Pitch Hz","Pitch env st","Pitch decay ms","Decay ms","Noise mix","Noise cutoff Hz","High pass","Drive","Level","Pan","Mute","Solo","Triangle","Clap pulses"};
}
juce::String KernDrumProcessor::voiceID(int v,int c) { return "v"+juce::String(v+1)+"_"+keys[c]; }
juce::String KernDrumProcessor::stepID(int p,int v,int s) { return "p"+juce::String(p+1)+"_v"+juce::String(v+1)+"_s"+juce::String(s+1); }
juce::AudioProcessorValueTreeState::ParameterLayout KernDrumProcessor::layout() {
 juce::AudioProcessorValueTreeState::ParameterLayout result;
 auto add=[&](const juce::String& id,const juce::String& title,float lo,float hi,float def,float skew=1.f) {
  juce::NormalisableRange<float> range(lo,hi); range.skew=skew;
  result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},title,range,def));
 };
 for(int v=0;v<8;++v) for(int c=0;c<kern::numControls;++c) {
  const auto id=voiceID(v,c), title=juce::String(kern::names[v])+" "+titles[c];
  if(c==kern::highpass || c==kern::mute || c==kern::solo || c==kern::wave || c==kern::clap)
   result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},title,kern::factory[v][c]>.5f));
  else {
   float lo=0,hi=1,skew=1;
   if(c==kern::pitch) {lo=30;hi=9000;skew=.3f;}
   if(c==kern::bend) {lo=-48;hi=48;}
   if(c==kern::bendTime) {lo=1;hi=500;skew=.4f;}
   if(c==kern::decay) {lo=5;hi=3000;skew=.35f;}
   if(c==kern::cutoff) {lo=80;hi=18000;skew=.35f;}
   if(c==kern::drive) hi=12;
   if(c==kern::pan) lo=-1;
   add(id,title,lo,hi,kern::factory[v][c],skew);
  }
 }
 add("master","Master",0,1,.8f); add("swing","Swing",0,1,0);
 result.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"pattern",1},"Pattern",0,3,0));
 result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"enabled",1},"Sequencer",true));
 for(int p=0;p<4;++p) for(int v=0;v<8;++v) for(int s=0;s<16;++s) {
  float velocity=0;
  if(p==0 && ((v==0 && s%4==0)||(v==1 && s%8==4)||(v==3 && s%2==0))) velocity=v==3?.55f:.85f;
  add(stepID(p,v,s),"Pattern "+juce::String(p+1)+" "+kern::names[v]+" Step "+juce::String(s+1),0,1,velocity);
 }
 return result;
}
KernDrumProcessor::KernDrumProcessor(): AudioProcessor(BusesProperties().withOutput("Stereo",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"KernDrumState",layout()) {
 for(int v=0;v<8;++v) {
  previews[v].store(false);
  for(int c=0;c<kern::numControls;++c) controls[v][c]=state.getRawParameterValue(voiceID(v,c));
 }
 for(int p=0;p<4;++p) for(int v=0;v<8;++v) for(int s=0;s<16;++s) cells[p][v][s]=state.getRawParameterValue(stepID(p,v,s));
 master=state.getRawParameterValue("master"); swing=state.getRawParameterValue("swing"); pattern=state.getRawParameterValue("pattern"); enabled=state.getRawParameterValue("enabled");
}
bool KernDrumProcessor::isBusesLayoutSupported(const BusesLayout& l) const {return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() && l.getMainInputChannelSet().isDisabled();}
void KernDrumProcessor::prepareToPlay(double sr,int) {
 rate=sr; engine.prepare(sr);
 for(int v=0;v<8;++v) {kern::Patch p; for(int c=0;c<kern::numControls;++c)p[c]=controls[v][c]->load();engine.drum[v].setPatch(p,true);}
 currentStep.store(-1);
}
void KernDrumProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {
 juce::ScopedNoDenormals noDenormals;
 buffer.clear(); if(buffer.getNumChannels()<2)return;
 for(int v=0;v<8;++v) {
  kern::Patch p;for(int c=0;c<kern::numControls;++c)p[c]=controls[v][c]->load(std::memory_order_relaxed);
  engine.drum[v].setPatch(p);
  if(previews[v].exchange(false))engine.trigger(v,.85f);
 }
 bool playing=false; double beat=0,bpm=120;
 if(auto* ph=getPlayHead()) if(auto position=ph->getPosition()) {
  playing=position->getIsPlaying();
  if(auto b=position->getBpm())if(std::isfinite(*b) && *b>0)bpm=*b;
  if(auto q=position->getPpqPosition()) {beat=*q;if(!std::isfinite(beat))playing=false;}else playing=false;
 }
 hostPlaying.store(playing);displayedBpm.store(bpm);
 const bool run=playing && enabled->load()>.5f;
 const double increment=bpm/(60*rate), sh=swing->load();
 const int pat=juce::jlimit(0,3,juce::roundToInt(pattern->load()));
 std::array<std::array<float,16>,8> velocities;
 for(int v=0;v<8;++v)for(int s=0;s<16;++s)velocities[v][s]=cells[pat][v][s]->load(std::memory_order_relaxed);
 auto it=midi.cbegin();const auto end=midi.cend();
 auto* left=buffer.getWritePointer(0);auto* right=buffer.getWritePointer(1);
 const float gain=master->load();
 for(int i=0;i<buffer.getNumSamples();++i) {
  while(it!=end && (*it).samplePosition<=i) {
   // Raw MIDI avoids constructing heap-backed SysEx messages in the audio callback.
   const auto event=*it;const auto* data=event.data;
   if(event.numBytes>=3 && (data[0]&0xf0)==0x90 && data[2]>0)engine.trigger(int(data[1])-36,data[2]/127.f);
   if(event.numBytes>=3 && (data[0]&0xf0)==0xb0 && (data[1]==120 || data[1]==123))
    for(auto& d:engine.drum){d.env=0;d.residual=0;}
   ++it;
  }
  if(run) {
   const double q=beat+i*increment;
   const auto now=kern::absoluteStep(q,sh),before=kern::absoluteStep(q-increment,sh);
   const int s=kern::wrappedStep(now);
   if(now!=before)for(int v=0;v<8;++v)if(velocities[v][s]>0)engine.trigger(v,velocities[v][s]);
   currentStep.store(s,std::memory_order_relaxed);
  }
  const auto out=engine.render(gain);left[i]=out[0];right[i]=out[1];
 }
 if(!run)currentStep.store(-1);
 meterL.store(engine.meterL);meterR.store(engine.meterR);midi.clear();
}
void KernDrumProcessor::setValue(const juce::String& id,float value) {
 if(auto* p=state.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();}
}
void KernDrumProcessor::copyPattern(int from,int to) {if(from==to)return;for(int v=0;v<8;++v)for(int s=0;s<16;++s)setValue(stepID(to,v,s),cells[from][v][s]->load());}
void KernDrumProcessor::clearPattern(int p) {for(int v=0;v<8;++v)for(int s=0;s<16;++s)setValue(stepID(p,v,s),0);}
void KernDrumProcessor::loadKit(int kit) {
 for(int v=0;v<8;++v)for(int c=0;c<kern::numControls;++c) {
  float x=kern::factory[v][c];
  if(kit==1){if(c==kern::pitch)x*=.72f;if(c==kern::decay)x*=1.6f;if(c==kern::drive)x+=2;}
  if(kit==2){if(c==kern::pitch)x*=1.35f;if(c==kern::decay)x*=.55f;if(c==kern::mix)x=std::min(1.f,x+.15f);}
  setValue(voiceID(v,c),x);
 }
}
void KernDrumProcessor::getStateInformation(juce::MemoryBlock& dest) {auto tree=state.copyState();tree.setProperty("version",1,nullptr);if(auto xml=tree.createXml())copyXmlToBinary(*xml,dest);}
void KernDrumProcessor::setStateInformation(const void* data,int size) {if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(state.state.getType())){auto tree=juce::ValueTree::fromXml(*xml);if(tree.isValid())state.replaceState(tree);}}
juce::AudioProcessorEditor* KernDrumProcessor::createEditor(){return new KernDrumEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new KernDrumProcessor();}
