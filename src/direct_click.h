#pragma once
#include <string>
#include <cstdlib>
#include <cstdint>
struct DirectClick {
 enum class Result { Wait, NativeDouble };
 std::wstring path; int x=0,y=0,downX=0,downY=0; unsigned long long time=0,deadline=0;
 bool pressed=false,second=false,released=false,rawPending=false;int rawX=0,rawY=0;unsigned long long rawTime=0;
 void reset(){path.clear();time=deadline=0;pressed=second=released=rawPending=false;}
 // Shell identity is resolved on release. A slow or transient DOWN lookup
 // must not discard an otherwise valid click; keep its original timestamp.
 void begin(int px,int py,unsigned long long now){rawX=px;rawY=py;rawTime=now;rawPending=true;pressed=true;deadline=0;}
 Result finish(const std::wstring&p,int px,int py,unsigned long long now,unsigned delay,int dx,int dy){
  if(!rawPending){reset();return Result::NativeDouble;}
  rawPending=false;press(p,rawX,rawY,rawTime,delay,dx,dy);return release(p,px,py,now,delay,dx,dy);
 }
 bool due(unsigned long long now)const{return !pressed&&!path.empty()&&deadline&&now>=deadline;}
 void press(const std::wstring&p,int px,int py,unsigned long long now,unsigned delay,int dx,int dy){
  // Windows recognises a double click on DOWN, even when the second UP is late.
  second=released&&path==p&&(uint32_t)(now-time)<=delay&&px>=downX-dx/2&&px<downX+(dx+1)/2&&py>=downY-dy/2&&py<downY+(dy+1)/2;
  pressed=true;released=false;deadline=0;
  if(!second){path=p;x=downX=px;y=downY=py;time=now;}
 }
 Result release(const std::wstring&p,int px,int py,unsigned long long now,unsigned delay,int,int){
  if(!pressed||path!=p){reset();return Result::NativeDouble;}
  pressed=false;
  if(second){reset();return Result::NativeDouble;}
  released=true;x=px;y=py;deadline=now+delay;return Result::Wait;
 }
};
