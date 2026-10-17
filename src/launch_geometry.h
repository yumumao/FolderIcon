#pragma once
#include <algorithm>
#include <cstdint>
namespace LaunchGeometry {
struct Rect {int left,top,right,bottom;};
inline Rect fit(Rect window,Rect source,Rect target){
 const int width=std::max(1,std::min(window.right-window.left,target.right-target.left));
 const int height=std::max(1,std::min(window.bottom-window.top,target.bottom-target.top));
 auto axis=[](int p,int begin,int end,int size,int destBegin,int destEnd,int destSize){
  const int space=std::max(0,end-begin-size),available=std::max(0,destEnd-destBegin-destSize);
  const int offset=std::max(0,std::min(p-begin,space));
  return destBegin+(space?int((int64_t(offset)*available+space/2)/space):0);
 };
 const int x=axis(window.left,source.left,source.right,window.right-window.left,target.left,target.right,width);
 const int y=axis(window.top,source.top,source.bottom,window.bottom-window.top,target.top,target.bottom,height);
 return {x,y,x+width,y+height};
}
}
