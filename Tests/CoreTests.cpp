#include "Engine.h"
#include <iostream>
#include <cstdlib>
#include <limits>
static void check(bool yes,const char* what){if(!yes){std::cerr<<"FAIL: "<<what<<'\n';std::exit(1);}}
int main(){
 for(double sr:{44100.,48000.,96000.}) {
  kern::Engine e;e.prepare(sr);
  float silent=0;for(int n=0;n<1000;++n){auto x=e.render(.8f);silent+=std::abs(x[0])+std::abs(x[1]);}check(silent==0,"initial silence");
  for(int v=0;v<8;++v){
   e.prepare(sr);e.trigger(v,.9f);double energy=0;
   for(int n=0;n<int(sr*4);++n){auto x=e.render(.8f);for(auto y:x){check(std::isfinite(y)&&std::abs(y)<=1,"finite bounded output");energy+=y*y;}}
   check(energy>.01,"every factory voice produces sound");
   check(std::abs(e.drum[v].previous)<.0001,"voice tail decays");
  }
 }
 for(double swing:{0.,.25,.5,1.})for(double bpm:{60.,123.,240.}){
  const double inc=bpm/(60*48000.);int count=0;int64_t last=-1;
  for(int n=0;n<int(std::ceil(4/inc));++n){double q=n*inc;auto now=kern::absoluteStep(q,swing);if(now!=kern::absoluteStep(q-inc,swing)){check(now>last,"monotonic steps");last=now;++count;double ideal=(now/2)*.5+(now%2?(.25+swing*.125):0);check(q-ideal>=-1e-10 && q-ideal<=inc+1e-10,"sample accurate swung boundary");}}
  check(count==16,"16 triggers per four beats");
 }
 check(kern::wrappedStep(-1)==15,"negative pre-roll wraps");
 // Reconstructing PPQ at block boundaries must not duplicate/drop steps.
 for(int block:{32,127,512,2048}){
  const double inc=123./(60*48000.);int count=0;int64_t last=-1;
  for(int start=0;start<200000;start+=block){const double beat=start*inc;for(int i=0;i<block && start+i<200000;++i){const double q=beat+i*inc;auto now=kern::absoluteStep(q,.61);if(now!=kern::absoluteStep(q-inc,.61)){check(now>last,"no repeated event across block boundaries");last=now;++count;}}}
  check(count==35,"block-size-independent trigger count");
 }
 {kern::Engine e;e.prepare(48000);auto patch=kern::factory[0];patch[kern::mute]=1;e.drum[0].setPatch(patch,true);e.trigger(0,1);double energy=0;for(int n=0;n<48000;++n){auto x=e.render(1);energy+=x[0]*x[0]+x[1]*x[1];}check(energy==0,"muted voice is silent");}
 {kern::Engine e;e.prepare(48000);auto patch=kern::factory[1];patch[kern::solo]=1;e.drum[1].setPatch(patch,true);e.trigger(0,1);double energy=0;for(int n=0;n<48000;++n){auto x=e.render(1);energy+=x[0]*x[0]+x[1]*x[1];}check(energy==0,"solo excludes other voices");}
 kern::Engine quiet,loud;quiet.prepare(48000);loud.prepare(48000);quiet.trigger(0,.2f);loud.trigger(0,1);double a=0,b=0;
 for(int n=0;n<10000;++n){auto x=quiet.render(.8f),y=loud.render(.8f);a+=x[0]*x[0];b+=y[0]*y[0];}check(b>a*2,"velocity affects energy");
 std::cout<<"PASS: factory voices, tails, finite stereo, velocity, swing timing at 3 tempos, pre-roll\n";
}
