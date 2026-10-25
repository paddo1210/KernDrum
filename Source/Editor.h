#pragma once
#include "Processor.h"
class KernDrumEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit KernDrumEditor(KernDrumProcessor&);
 ~KernDrumEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 struct Dial:juce::Component {
  juce::Slider slider;juce::Label label;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
  Dial();void resized()override;
  void bind(KernDrumProcessor&,const juce::String&,const juce::String&);
 };
 struct Grid:juce::Component {
  KernDrumEditor& editor;int dragVoice=-1,dragStep=-1;float startVelocity=0;int startY=0;
  explicit Grid(KernDrumEditor& e):editor(e){}
  void paint(juce::Graphics&)override;
  void mouseDown(const juce::MouseEvent&)override;
  void mouseDrag(const juce::MouseEvent&)override;
  void mouseUp(const juce::MouseEvent&)override;
  juce::Rectangle<float> cell(int v,int s)const;
 } grid;
 KernDrumProcessor& processor;
 juce::LookAndFeel_V4 look;
 juce::Label title,subtitle,voiceTitle,status,help;
 std::array<juce::TextButton,8> voiceButtons,triggerButtons;
 std::array<juce::ToggleButton,5> toggles;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,5> toggleAttachments;
 std::array<Dial,9> dials;
 Dial master,swing;
 juce::ToggleButton sequencer{"Sequencer"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sequencerAttachment;
 juce::ComboBox pattern,kit;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patternAttachment;
 juce::TextButton copy{"Copy to next"},clear{"Clear pattern"},reset{"Load kit"};
 int selected=0;
 void selectVoice(int);
 void timerCallback()override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KernDrumEditor)
};
