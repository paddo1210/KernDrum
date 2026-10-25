#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace kern {
constexpr int voices = 8, steps = 16, patterns = 4;
constexpr double pi = 3.14159265358979323846;
enum Control { pitch, bend, bendTime, decay, mix, cutoff, highpass, drive, level, pan, mute, solo, wave, clap, numControls };
using Patch = std::array<float, numControls>;
inline constexpr std::array<const char*, voices> names { "Kick", "Snare", "Clap", "Closed Hat", "Open Hat", "Low Tom", "High Tom", "Metal" };
inline constexpr std::array<Patch, voices> factory {{
 {48,32,35,330,.02f,3200,0,2,.85f,0,0,0,0,0},
 {180,9,22,170,.75f,4200,1,1,.65f,-.08f,0,0,0,0},
 {280,0,15,120,.98f,1600,1,.8f,.55f,.1f,0,0,0,1},
 {5800,0,5,38,.97f,6800,1,.6f,.4f,-.22f,0,0,1,0},
 {6100,0,5,390,.98f,7200,1,.4f,.35f,.25f,0,0,1,0},
 {100,12,85,350,.05f,1800,0,.7f,.65f,-.3f,0,0,0,0},
 {210,10,55,220,.07f,2600,0,.7f,.6f,.3f,0,0,0,0},
 {790,19,60,240,.28f,4400,1,3,.4f,.12f,0,0,1,0}
}};
inline int64_t absoluteStep(double beat, double swing) noexcept {
 // The host's PPQ and sample-derived PPQ can differ by a few ULP at
 // exact grid boundaries. Bias by far less than one audio sample to avoid
 // counting a boundary twice in consecutive blocks/samples.
 beat += 1e-10;
 const auto pair = static_cast<int64_t>(std::floor(beat / .5));
 const double within = beat - static_cast<double>(pair) * .5;
 return pair * 2 + (within >= .25 + std::clamp(swing, 0.0, 1.0) * .125 ? 1 : 0);
}
inline int wrappedStep(int64_t step) noexcept { return static_cast<int>((step % 16 + 16) % 16); }
struct Voice {
 double sr = 48000, phase = 0;
 float env = 0, pitchEnv = 0, attack = 0, velocity = 0, noiseLow = 0;
 float previous = 0, residual = 0, ampCoeff = .999f, pitchCoeff = .99f, noiseCoeff = .1f;
 float ampTarget=.999f,pitchTarget=.99f,noiseTarget=.1f;
 float attackCoeff = .96f, tailCoeff = .99f;
 int age = 0, pulse = 0;
 uint32_t rng = 0x12345678;
 Patch smooth = factory[0], target = factory[0];
 float smoothing = .004f;
 void prepare(double rate, int seed) noexcept {
  sr = rate; phase = 0; env = pitchEnv = attack = velocity = noiseLow = previous = residual = 0;
  rng = 0x12345678u + static_cast<uint32_t>(seed * 9781);
  smoothing = static_cast<float>(1.0 - std::exp(-1.0 / (.008 * sr)));
  attackCoeff = static_cast<float>(std::exp(-1.0 / (.0006 * sr)));
  tailCoeff = static_cast<float>(std::exp(-1.0 / (.002 * sr)));
 }
 void setPatch(const Patch& p, bool immediate = false) noexcept {
  target = p; if (immediate) smooth = p;
  // Decay reaches -60 dB after the specified time.
  ampTarget = static_cast<float>(std::exp(-6.907755 / (std::max(5.f,p[decay]) * .001 * sr)));
  pitchTarget = static_cast<float>(std::exp(-6.907755 / (std::max(1.f,p[bendTime]) * .001 * sr)));
  noiseTarget = static_cast<float>(1.0 - std::exp(-2 * pi * std::min(double(p[cutoff]), sr * .45) / sr));
  if(immediate){ampCoeff=ampTarget;pitchCoeff=pitchTarget;noiseCoeff=noiseTarget;}
 }
 void trigger(float v) noexcept {
  residual = previous; env = pitchEnv = 1; attack = 0; velocity = std::clamp(v,0.f,1.f);
  phase = 0; age = pulse = 0;
 }
 float render() noexcept {
  for (int c = 0; c < numControls; ++c) smooth[c] += smoothing * (target[c] - smooth[c]);
  ampCoeff+=smoothing*(ampTarget-ampCoeff);pitchCoeff+=smoothing*(pitchTarget-pitchCoeff);noiseCoeff+=smoothing*(noiseTarget-noiseCoeff);
  if (env < 1e-7f && std::abs(residual) < 1e-7f) { previous = 0; return 0; }
  if (target[clap] > .5f && pulse < 2 && age >= static_cast<int>((pulse+1)*.011*sr)) { env = 1; attack = 0; ++pulse; }
  ++age;
  const double hz = std::clamp(double(smooth[pitch]) * std::exp2(smooth[bend] * pitchEnv / 12.0), 10.0, sr * .42);
  phase += hz / sr; phase -= std::floor(phase);
  const float sine=static_cast<float>(std::sin(2*pi*phase)),tri=static_cast<float>(1 - 4 * std::abs(phase-.5));
  const float tone=sine+(tri-sine)*smooth[wave];
  rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
  const float noise = static_cast<float>(rng >> 8) / 8388608.f - 1.f;
  noiseLow += noiseCoeff * (noise - noiseLow);
  const float filtered=noiseLow+(noise-2*noiseLow)*smooth[highpass];
  attack = 1 - (1-attack)*attackCoeff;
  const float dry = ((1-smooth[mix])*tone + smooth[mix]*filtered) * env * attack * velocity;
  const float gain = 1 + smooth[drive]*2;
  const float shaped = std::tanh(dry * gain) / std::sqrt(gain);
  previous = shaped + residual;
  residual *= tailCoeff; env *= ampCoeff; pitchEnv *= pitchCoeff;
  return previous;
 }
};
struct Engine {
 std::array<Voice, voices> drum;
 float meterL = 0, meterR = 0;
 void prepare(double sr) noexcept { for (int v=0;v<voices;++v) { drum[v].prepare(sr,v); drum[v].setPatch(factory[v],true); } }
 void trigger(int v,float velocity) noexcept { if (v>=0 && v<voices) drum[v].trigger(velocity); }
 std::array<float,2> render(float master) noexcept {
  float l=0,r=0; bool anySolo=false;
  for (auto& d:drum) anySolo |= d.target[solo]>.5f;
  for (auto& d:drum) {
   const float x=d.render();
   const bool audible=d.target[mute]<.5f && (!anySolo || d.target[solo]>.5f);
   // Mute/solo gain is also smoothed, so switching does not cut a waveform.
   const float desired=audible ? d.smooth[level] : 0.f;
   auto& g=audibleGain[static_cast<size_t>(&d-drum.data())]; g += d.smoothing*(desired-g);
   const double p=(d.smooth[pan]+1)*pi*.25;
   l+=x*g*static_cast<float>(std::cos(p)); r+=x*g*static_cast<float>(std::sin(p));
  }
  masterGain += drum[0].smoothing*(master-masterGain);
  l=std::tanh(l*masterGain*.7f); r=std::tanh(r*masterGain*.7f);
  meterL=std::max(std::abs(l),meterL*.9995f); meterR=std::max(std::abs(r),meterR*.9995f);
  return {l,r};
 }
 std::array<float,voices> audibleGain{};
 float masterGain=.8f;
};
}
