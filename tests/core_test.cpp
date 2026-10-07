#include "../src/core.h"
#include <cassert>
#include <iostream>
int main(){
 assert(clampValue(-1,24,256)==24); assert(clampValue(400,24,256)==256);
 Metrics m=normalizeMetrics(96,120,144); assert(m.icon==96 && m.cellW==120 && m.cellH==144);
 m=normalizeMetrics(96,20,40);assert(m.cellW>=112 && m.cellH>=140);
 ClickState c; assert(c.release(0,10,10,100,500,4,4)==ClickAction::Wait);
 assert(c.release(0,11,11,300,500,4,4)==ClickAction::Open);
 assert(c.release(1,10,10,350,500,4,4)==ClickAction::Wait);
 assert(c.release(1,30,30,400,500,4,4)==ClickAction::Wait);
 c.reset();assert(c.lastId==-1);
 assert(validColor(L"1a2B3c"));assert(!validColor(L"#bad"));assert(!validColor(L"ZZFFFF"));
 std::cout<<"12 core assertions passed\n";
}
