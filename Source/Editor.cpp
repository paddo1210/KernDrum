#include "Editor.h"
namespace {const juce::Colour bg{0xff10191f},panel{0xff1b2830},accent{0xffdfb36c},ink{0xffe8eeee},dim{0xff95a7ad};}
KernDrumEditor::Dial::Dial(){
 slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,96,21);
 slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 label.setJustificationType(juce::Justification::centred);label.setFont(juce::FontOptions(12.f));
 addAndMakeVisible(slider);addAndMakeVisible(label);
}
void KernDrumEditor::Dial::resized(){auto r=getLocalBounds();label.setBounds(r.removeFromTop(20));slider.setBounds(r);}
void KernDrumEditor::Dial::bind(KernDrumProcessor& p,const juce::String& id,const juce::String& name){
 attachment.reset();label.setText(name,juce::dontSendNotification);slider.setTooltip(name+" — double-click to reset");
 slider.setName(name);attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,slider);
 slider.setDoubleClickReturnValue(true,p.state.getParameter(id)->convertFrom0to1(p.state.getParameter(id)->getDefaultValue()));
}
KernDrumEditor::KernDrumEditor(KernDrumProcessor& p):AudioProcessorEditor(p),grid(*this),processor(p){
 look.setColour(juce::ResizableWindow::backgroundColourId,bg);look.setColour(juce::Slider::rotarySliderFillColourId,accent);
 look.setColour(juce::Slider::thumbColourId,accent);look.setColour(juce::Slider::textBoxTextColourId,ink);
 look.setColour(juce::Label::textColourId,ink);look.setColour(juce::TextButton::buttonColourId,panel);
 look.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff42686c));look.setColour(juce::ComboBox::backgroundColourId,panel);
 look.setColour(juce::ToggleButton::textColourId,ink);look.setColour(juce::ToggleButton::tickColourId,accent);
 setLookAndFeel(&look);
 title.setText("KERN / DRUM",juce::dontSendNotification);title.setFont(juce::FontOptions(30.f,juce::Font::bold));
 subtitle.setText("SYNTHETIC RHYTHM INSTRUMENT  /  01",juce::dontSendNotification);subtitle.setColour(juce::Label::textColourId,dim);
 for(auto* c:std::initializer_list<juce::Component*>{&title,&subtitle,&voiceTitle,&status,&help,&grid,&sequencer,&pattern,&kit,&copy,&clear,&reset,&master,&swing})addAndMakeVisible(c);
 voiceTitle.setFont(juce::FontOptions(20.f,juce::Font::bold));
 for(int v=0;v<8;++v){
  auto& b=voiceButtons[v];b.setButtonText(juce::String(v+1)+"  "+kern::names[v]);b.setClickingTogglesState(false);b.onClick=[this,v]{selectVoice(v);};addAndMakeVisible(b);
  auto& t=triggerButtons[v];t.setButtonText("Hit");t.setTooltip("Audition / MIDI note "+juce::String(36+v));t.onClick=[this,v]{processor.audition(v);};addAndMakeVisible(t);
 }
 for(auto& d:dials)addAndMakeVisible(d);
 for(auto& t:toggles)addAndMakeVisible(t);
 master.bind(p,"master","Output");swing.bind(p,"swing","Swing");
 sequencerAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"enabled",sequencer);
 for(int i=0;i<4;++i)pattern.addItem("Pattern "+juce::String(i+1),i+1);
 pattern.setTooltip("Pattern changes take effect at the next audio block");
 patternAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"pattern",pattern);
 kit.addItem("Core / balanced",1);kit.addItem("Weight / low & long",2);kit.addItem("Spark / tight & bright",3);kit.setSelectedId(1);
 reset.setTooltip("Replaces all eight voice settings; patterns are retained");reset.onClick=[this]{processor.loadKit(kit.getSelectedId()-1);};
 copy.onClick=[this]{int p=juce::roundToInt(processor.state.getRawParameterValue("pattern")->load());processor.copyPattern(p,(p+1)%4);processor.setValue("pattern",float((p+1)%4));};
 clear.onClick=[this]{processor.clearPattern(juce::roundToInt(processor.state.getRawParameterValue("pattern")->load()));};
 help.setText("STEP: click on/off  /  drag up/down: velocity  /  right-click: accent     •     MIDI: notes 36–43, all channels",juce::dontSendNotification);
 help.setFont(juce::FontOptions(12.f));help.setColour(juce::Label::textColourId,dim);
 selectVoice(0);setResizable(true,true);setResizeLimits(960,720,1600,1100);setSize(1120,800);startTimerHz(30);
}
KernDrumEditor::~KernDrumEditor(){stopTimer();setLookAndFeel(nullptr);}
void KernDrumEditor::selectVoice(int v){
 selected=v;voiceTitle.setText(juce::String(kern::names[v])+"  /  SOUND",juce::dontSendNotification);
 for(int n=0;n<8;++n)voiceButtons[n].setToggleState(n==v,juce::dontSendNotification);
 constexpr std::array<int,9> control{kern::pitch,kern::bend,kern::bendTime,kern::decay,kern::mix,kern::cutoff,kern::drive,kern::level,kern::pan};
 constexpr std::array<const char*,9> labels{"Pitch / Hz","Pitch env / st","Pitch time / ms","Decay / ms","Noise mix","Noise filter / Hz","Drive","Level","Pan"};
 for(int n=0;n<9;++n)dials[n].bind(processor,KernDrumProcessor::voiceID(v,control[n]),labels[n]);
 constexpr std::array<int,5> ids{kern::highpass,kern::wave,kern::clap,kern::mute,kern::solo};
 constexpr std::array<const char*,5> texts{"High-pass noise","Triangle tone","Clap pulses","Mute","Solo"};
 for(int n=0;n<5;++n){toggleAttachments[n].reset();toggles[n].setButtonText(texts[n]);toggleAttachments[n]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,KernDrumProcessor::voiceID(v,ids[n]),toggles[n]);}
}
void KernDrumEditor::resized(){
 const int w=getWidth(),h=getHeight();
 title.setBounds(24,16,340,40);subtitle.setBounds(26,57,400,22);
 kit.setBounds(w-500,26,210,28);reset.setBounds(w-280,26,100,28);master.setBounds(w-160,8,130,100);
 const int rowY=118,rowH=40;
 for(int v=0;v<8;++v){voiceButtons[v].setBounds(24,rowY+v*rowH,139,32);triggerButtons[v].setBounds(169,rowY+v*rowH,43,32);}
 grid.setBounds(228,110,w-254,332);
 sequencer.setBounds(24,461,144,30);pattern.setBounds(180,461,148,30);copy.setBounds(340,461,125,30);clear.setBounds(477,461,125,30);status.setBounds(620,459,w-755,36);
 swing.setBounds(w-135,442,110,100);
 voiceTitle.setBounds(24,521,360,30);
 const int dw=(w-48)/9;const int dialHeight=juce::jlimit(90,180,h-630);
 for(int n=0;n<9;++n)dials[n].setBounds(24+n*dw,558,dw-4,dialHeight);
 for(int n=0;n<5;++n)toggles[n].setBounds(28+n*((w-56)/5),h-66,(w-56)/5,25);
 help.setBounds(24,h-34,w-48,25);
}
void KernDrumEditor::paint(juce::Graphics& g){
 g.fillAll(bg);g.setColour(panel);g.fillRoundedRectangle(14.f,103.f,float(getWidth()-28),340.f,12.f);
 g.fillRoundedRectangle(14.f,515.f,float(getWidth()-28),float(getHeight()-558),12.f);
 const float x=float(getWidth()-26);g.setColour(juce::Colour(0xff26383f));g.fillRect(x-160,77.f,160.f,5.f);g.fillRect(x-160,86.f,160.f,5.f);
 g.setColour(accent);g.fillRect(x-160,77.f,160*processor.meterL.load(),5.f);g.fillRect(x-160,86.f,160*processor.meterR.load(),5.f);
}
void KernDrumEditor::timerCallback(){
 status.setText(processor.hostPlaying.load()?juce::String(processor.displayedBpm.load(),1)+" BPM / HOST":"DAW stopped",juce::dontSendNotification);
 grid.repaint();repaint();
}
juce::Rectangle<float> KernDrumEditor::Grid::cell(int v,int s)const{
 const float cw=getWidth()/16.f,rh=getHeight()/8.f;return {s*cw+2,v*rh+8,cw-5,rh-13};
}
void KernDrumEditor::Grid::paint(juce::Graphics& g){
 const int p=juce::roundToInt(editor.processor.state.getRawParameterValue("pattern")->load()),play=editor.processor.currentStep.load();
 for(int v=0;v<8;++v)for(int s=0;s<16;++s){
  const float value=editor.processor.state.getRawParameterValue(KernDrumProcessor::stepID(p,v,s))->load();const auto r=cell(v,s);
  g.setColour(value>0?accent.interpolatedWith(juce::Colour(0xff55616a),1-value):juce::Colour(s%4==0?0xff35454e:0xff27363e));g.fillRoundedRectangle(r,4);
  if(value>0){g.setColour(bg.withAlpha(.5f));g.fillRect(r.getX()+4,r.getBottom()-5,(r.getWidth()-8)*value,2.f);}
  if(s==play){g.setColour(ink);g.drawRoundedRectangle(r,4,1.5f);}
  if(v==0){g.setColour(dim);g.setFont(10.f);g.drawText(juce::String(s+1),int(r.getX()),0,int(r.getWidth()),10,juce::Justification::centred);}
 }
}
void KernDrumEditor::Grid::mouseDown(const juce::MouseEvent& e){
 const int v=juce::jlimit(0,7,e.y*8/getHeight()),s=juce::jlimit(0,15,e.x*16/getWidth());
 editor.selectVoice(v);dragVoice=v;dragStep=s;startY=e.y;
 int p=juce::roundToInt(editor.processor.state.getRawParameterValue("pattern")->load());const auto id=KernDrumProcessor::stepID(p,v,s);
 startVelocity=editor.processor.state.getRawParameterValue(id)->load();
 editor.processor.setValue(id,e.mods.isRightButtonDown()?1.f:(startVelocity>0?0.f:.75f));
}
void KernDrumEditor::Grid::mouseDrag(const juce::MouseEvent& e){
 if(dragVoice<0)return;
 const int p=juce::roundToInt(editor.processor.state.getRawParameterValue("pattern")->load());
 if(e.getDistanceFromDragStart()>3)editor.processor.setValue(KernDrumProcessor::stepID(p,dragVoice,dragStep),juce::jlimit(.01f,1.f,(startVelocity>0?startVelocity:.75f)+(startY-e.y)*.008f));
}
void KernDrumEditor::Grid::mouseUp(const juce::MouseEvent&){dragVoice=dragStep=-1;}
