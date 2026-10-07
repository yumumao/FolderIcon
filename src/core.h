#pragma once
#include <cwchar>
#include <cstdlib>
inline int clampValue(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
struct Metrics{int icon,cellW,cellH;};
inline Metrics normalizeMetrics(int i,int w,int h){i=clampValue(i,24,256);return{i,clampValue(w,i+16,512),clampValue(h,i+44,560)};}
inline bool validColor(const wchar_t* s){if(wcslen(s)!=6)return false;for(int i=0;i<6;i++)if(!((s[i]>=L'0'&&s[i]<=L'9')||(s[i]>=L'a'&&s[i]<=L'f')||(s[i]>=L'A'&&s[i]<=L'F')))return false;return true;}
enum class ClickAction{Wait,Open};
struct ClickState{int lastId=-1,x=0,y=0;unsigned long long time=0;void reset(){lastId=-1;time=0;}ClickAction release(int id,int px,int py,unsigned long long now,unsigned int delay,int dx,int dy){if(id==lastId&&now-time<=delay&&abs(px-x)<=dx&&abs(py-y)<=dy){reset();return ClickAction::Open;}lastId=id;x=px;y=py;time=now;return ClickAction::Wait;}};
