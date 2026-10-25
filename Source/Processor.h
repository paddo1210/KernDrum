#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
static_assert(std::atomic<float>::is_always_lock_free && std::atomic<double>::is_always_lock_free && std::atomic<bool>::is_always_lock_free,
              "KernDrum requires lock-free audio-thread atomics (64-bit target)");

class KernDrumProcessor final : public juce::AudioProcessor {
public:
 KernDrumProcessor();
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 static juce::String voiceID(int v,int c);
 static juce::String stepID(int p,int v,int s);
 juce::AudioProcessorValueTreeState state;
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "KernDrum"; }
 bool acceptsMidi() const override { return true; }
 bool producesMidi() const override { return false; }
 double getTailLengthSeconds() const override { return 4; }
 int getNumPrograms() override { return 1; }
 int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override { return "User Kit"; }
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 void setValue(const juce::String&,float);
 void audition(int v) { if (juce::isPositiveAndBelow(v,8)) previews[v].store(true); }
 void copyPattern(int from,int to);
 void clearPattern(int);
 void loadKit(int);
 std::atomic<int> currentStep{-1};
 std::atomic<float> meterL{0},meterR{0};
 std::atomic<double> displayedBpm{120};
 std::atomic<bool> hostPlaying{false};
private:
 kern::Engine engine;
 std::array<std::array<std::atomic<float>*,kern::numControls>,8> controls{};
 std::array<std::array<std::array<std::atomic<float>*,16>,8>,4> cells{};
 std::array<std::atomic<bool>,8> previews{};
 std::atomic<float> *master=nullptr,*swing=nullptr,*pattern=nullptr,*enabled=nullptr;
 double rate=48000;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KernDrumProcessor)
};
