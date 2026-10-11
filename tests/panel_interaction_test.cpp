#include "../src/panel_interaction.h"
#include <cassert>
#include <cstdio>
int main(){using namespace PanelInteraction;
 for(int grid:{64,104,120,216,408}){int w=panelWidth(grid);assert(w>=216&&w>=grid);assert(backRect(w).w>=68);}
 for(int w:{104,216,264,408,1024}){auto r=backRect(w);assert(r.x+r.w<=w-3*actionStep(w));assert(r.h==34);}
 auto r=backRect(408);assert(r.contains(125,24));assert(!r.contains(155,24));assert(!r.contains(20,44));
 assert(blank(240,85,408,202,20,-1,false));assert(!blank(240,85,408,202,20,2,false));assert(!blank(100,20,408,202,20,-1,false));assert(!blank(-1,85,408,202,20,-1,false));assert(!blank(407,201,408,202,20,-1,false));assert(!blank(405,80,408,202,20,-1,true));assert(blank(200,190,408,202,20,-1,true));
 ClickGuard c;c.down(true);assert(!c.consumeUp());assert(c.doubleClick(true));c.navigate();assert(c.consumeUp());assert(!c.consumeUp());c.down(false);assert(!c.doubleClick(true));assert(c.consumeUp());c.down(true);assert(!c.doubleClick(false));assert(c.consumeUp());c.down(true);c.navigate();assert(!c.doubleClick(true));c.down(false);assert(!c.consumeUp());c.clear();assert(!c.skipUp&&!c.blankDown);
 puts("PASS panel hit regions, blank exclusions and trailing-UP guard");
}