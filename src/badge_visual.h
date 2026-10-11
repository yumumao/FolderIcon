#pragma once
#include <algorithm>
#include <gdiplus.h>
// Shared rounded four-tile badge. Tighten only the frame, never scale the tiles.
namespace BadgeVisual {
enum class Kind { Shortcut, DesktopFolder, NestedFolder };
inline int extent(int iconSize){return std::max(8,iconSize*32/100);}
inline void rounded(Gdiplus::GraphicsPath& p,float x,float y,float d,float r){
 float a=2*r;p.AddArc(x,y,a,a,180,90);p.AddArc(x+d-a,y,a,a,270,90);
 p.AddArc(x+d-a,y+d-a,a,a,0,90);p.AddArc(x,y+d-a,a,a,90,90);p.CloseFigure();
}
inline void draw(Gdiplus::Graphics& g,float x,float y,int iconSize,Kind kind){
 using namespace Gdiplus;auto state=g.Save();int d=extent(iconSize),reference=std::max(8,iconSize*36/100);
 // Clip antialiasing too: the original image outside this rectangle is protected.
 g.SetClip(RectF(x,y,(float)d,(float)d),CombineModeIntersect);
 g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetPixelOffsetMode(PixelOffsetModeHalf);
 Color color=kind==Kind::Shortcut?Color(255,29,106,177):kind==Kind::DesktopFolder?Color(255,218,125,24):Color(255,0,126,137);
 SolidBrush white(Color(255,255,255,255)),fill(color);int stroke=std::max(1,d/20),radius=std::max(2,(d*23+50)/100);
 GraphicsPath outer,inner;rounded(outer,x,y,(float)d,(float)radius);
 rounded(inner,x+stroke,y+stroke,(float)(d-2*stroke),(float)std::max(1,radius-stroke));
 g.FillPath(&white,&outer);g.FillPath(&fill,&inner);
 // These dimensions intentionally use the approved 36% frame, not the new 32% one.
 int tile=std::max(1,reference*21/100),gap=std::max(1,reference*11/100),left=(d-2*tile-gap)/2;
 g.SetSmoothingMode(SmoothingModeNone);
 for(int row=0;row<2;++row)for(int col=0;col<2;++col)
  g.FillRectangle(&white,x+left+col*(tile+gap),y+left+row*(tile+gap),(float)tile,(float)tile);
 g.Restore(state);
}
}
