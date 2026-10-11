#pragma once
#include <cwchar>
#include <cstdlib>
inline int clampValue(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
struct Metrics{int icon,cellW,cellH;};
inline Metrics normalizeMetrics(int i,int w,int h){i=clampValue(i,24,256);return{i,clampValue(w,i+16,512),clampValue(h,i+44,560)};}
inline int fontPixels(int points,int dpi){return (points*dpi+36)/72;}
inline int labelTop(int y,int icon,int gap){return y+5+icon+gap;}
inline int labelCellHeight(int icon,int pixels,int gap){return icon+gap+10+(pixels*24+9)/10;}
inline int previewDelay(int requested,int systemDelay){return clampValue(requested,0,systemDelay>0?systemDelay:0);}
struct PanelPosition{int x,y;};
inline PanelPosition placePanel(int ax,int ay,int w,int h,int left,int top,int right,int bottom){
 int x=clampValue(ax+16,left+8,right-w-8),y=clampValue(ay+16,top+8,bottom-h-8);
 // Prefer a side that leaves the original desktop click point uncovered.
 if(ax+16+w<=right-8)x=ax+16;
 else if(ax-16-w>=left+8)x=ax-16-w;
 else if(ay+16+h<=bottom-8)y=ay+16;
 else if(ay-16-h>=top+8)y=ay-16-h;
 return {x,y};
}
inline bool validColor(const wchar_t* s){if(wcslen(s)!=6)return false;for(int i=0;i<6;i++)if(!((s[i]>=L'0'&&s[i]<=L'9')||(s[i]>=L'a'&&s[i]<=L'f')||(s[i]>=L'A'&&s[i]<=L'F')))return false;return true;}
// Windows HIDDEN (2) and SYSTEM (4) are hidden entries; never enumerate dot navigation.
inline bool visibleItem(const wchar_t* name,unsigned long attributes,bool dotHidden,bool showHidden){
 if(!name||!name[0]||!wcscmp(name,L".")||!wcscmp(name,L".."))return false;
 return showHidden||!((attributes&6)||(dotHidden&&name[0]==L'.'));
}
enum class ClickAction{Wait,Open};
struct ClickState{
 int lastId=-1,x=0,y=0;unsigned long long time=0;bool previewed=false;
 void reset(){lastId=-1;time=0;previewed=false;}
 bool previewDue(unsigned long long now,unsigned int delay)const{return lastId>=0&&!previewed&&now-time>=delay;}
 void markPreview(){previewed=true;}
 ClickAction release(int id,int px,int py,unsigned long long now,unsigned int delay,int dx,int dy){
  if(id==lastId&&now-time<=delay&&abs(px-x)<=dx&&abs(py-y)<=dy){reset();return ClickAction::Open;}
  lastId=id;x=px;y=py;time=now;previewed=false;return ClickAction::Wait;
 }
};
