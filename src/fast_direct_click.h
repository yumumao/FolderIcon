#pragma once
#include <memory>
#include <cstdint>
// The hook records only input facts. Shell identity is resolved outside the hook.
// If Explorer has already received a native double DOWN, that native launch wins.
struct FastDirectClick {
 struct Gesture {uintptr_t window;int x,y;uint32_t time;bool nativeDouble=false,opened=false,cancelled=false;};
 using Token=std::shared_ptr<Gesture>;
 struct Decision {Token gesture;bool convert=false;};
 Token previous;
 void reset(){if(previous)previous->cancelled=true;previous.reset();}
 void cancel(const Token&g){if(g)g->cancelled=true;if(previous==g)previous.reset();}
 Decision press(uintptr_t window,bool eligible,int x,int y,uint32_t time,unsigned delay,int dx,int dy){
  auto g=std::make_shared<Gesture>(Gesture{window,x,y,time});
  if(!eligible){reset();g->cancelled=true;return {g,false};}
  bool pair=previous&&!previous->cancelled&&previous->window==window&&uint32_t(time-previous->time)<=delay&&
    x>=previous->x-dx/2&&x<previous->x+(dx+1)/2&&y>=previous->y-dy/2&&y<previous->y+(dy+1)/2;
  if(pair){
   if(previous->opened){g->cancelled=true;return {g,true};}
   previous->nativeDouble=g->nativeDouble=true;previous.reset();return {g,false};
  }
  reset();previous=g;return {g,false};
 }
 bool claim(const Token&g){if(!g||g->cancelled||g->nativeDouble||g->opened)return false;g->opened=true;return true;}
};
