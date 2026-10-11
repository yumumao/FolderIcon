#pragma once
#include <algorithm>
namespace PanelInteraction {
struct Rect {int x,y,w,h;bool contains(int px,int py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
// Reserve room for Back + Sort + Settings + Close even for one tiny icon column.
inline int panelWidth(int gridWidth){return std::max(216,gridWidth);}
inline int actionStep(int width){return std::min(66,std::max(24,width/6));}
inline Rect backRect(int width){return {12,7,std::min(138,std::max(0,width-3*actionStep(width)-24)),34};}
inline bool blank(int x,int y,int width,int height,int radius,int item,bool scrollable){
 if(item>=0||x<0||x>=width||y<50||y>=height)return false;
 if(scrollable&&x>=width-10&&y<height-32)return false;
 int r=std::min(radius,std::min(width,height)/2);if(!r)return true;
 int dx=x<r?r-x:x>=width-r?x-(width-r-1):0;
 int dy=y<r?r-y:y>=height-r?y-(height-r-1):0;
 return dx*dx+dy*dy<=r*r;
}
// The last UP of a double-click may target a newly recreated parent window.
// A fresh DOWN always clears this suppression, so no future single click is lost.
struct ClickGuard {
 bool blankDown=false,skipUp=false;
 void down(bool isBlank){skipUp=false;blankDown=isBlank;}
 bool doubleClick(bool isBlank){bool back=blankDown&&isBlank;blankDown=false;skipUp=true;return back;}
 bool consumeUp(){bool skip=skipUp;skipUp=false;return skip;}
 void navigate(){blankDown=false;}
 void clear(){blankDown=skipUp=false;}
};
}