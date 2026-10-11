#include "../src/core.h"
#include <cassert>
#include <iostream>
int main(){
 assert(clampValue(-1,24,256)==24); assert(clampValue(400,24,256)==256);
 Metrics m=normalizeMetrics(96,120,144); assert(m.icon==96 && m.cellW==120 && m.cellH==144);
 m=normalizeMetrics(96,20,40);assert(m.cellW>=112 && m.cellH>=140);
 ClickState c; assert(c.release(0,10,10,100,500,4,4)==ClickAction::Wait);
 assert(!c.previewDue(279,180)); assert(c.previewDue(280,180));
 c.markPreview(); assert(!c.previewDue(400,180));
 // Early preview MUST preserve the full system double-click window.
 assert(c.release(0,11,11,550,500,4,4)==ClickAction::Open);
 assert(c.release(1,10,10,600,500,4,4)==ClickAction::Wait);
 assert(c.release(1,30,30,650,500,4,4)==ClickAction::Wait);
 c.markPreview(); assert(c.release(1,30,30,1200,500,4,4)==ClickAction::Wait);
 assert(c.previewDue(1380,180));
 c.reset();assert(c.lastId==-1);assert(!c.previewDue(9999,180));
 assert(validColor(L"1a2B3c"));assert(!validColor(L"#bad"));assert(!validColor(L"ZZFFFF"));
 assert(fontPixels(12,96)==16);assert(fontPixels(12,144)==24);
 assert(labelTop(50,96,3)==154);
 assert(labelCellHeight(96,24,3)>=96+3+10+58);
 assert(previewDelay(180,500)==180);assert(previewDelay(0,500)==0);
 assert(previewDelay(1000,200)==200);
 auto p=placePanel(1850,950,550,700,0,0,1920,1040);
 assert(p.x+550<=1834);assert(p.y>=8&&p.y+700<=1032);
 p=placePanel(-1880,50,550,700,-1920,0,0,1040);
 assert(p.x>=-1864);assert(p.y>=8);
 assert(previewDelay(1,500)==1); assert(previewDelay(-1,500)==0);
 ClickState instant; instant.release(0,0,0,100,500,4,4);
 assert(instant.previewDue(100,0)); instant.markPreview();
 assert(instant.release(0,0,0,140,500,4,4)==ClickAction::Open);
 assert(!visibleItem(L".",0,true,true)); assert(!visibleItem(L"..",0,true,true));
 assert(visibleItem(L"normal.txt",0,true,false));
 assert(!visibleItem(L".git",0,true,false)); assert(!visibleItem(L".env",0,true,false));
 assert(visibleItem(L".git",0,false,false)); assert(visibleItem(L".env",0,true,true));
 assert(!visibleItem(L"hidden",2,false,false)); assert(visibleItem(L"hidden",2,true,true));
 assert(!visibleItem(L"system",4,false,false)); assert(visibleItem(L"system",4,true,true));
 assert(visibleItem(L"folder.name",16,true,false)); assert(!visibleItem(L".folder",16,true,false));
 std::cout<<"core regression assertions passed\n";
}
