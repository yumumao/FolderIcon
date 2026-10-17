#include "../src/launch_geometry.h"
#include <cstdio>
#include <cstdlib>
using namespace LaunchGeometry;
static int checks=0;
static void check(bool value,const char* name){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
int main(){
 Rect a{0,0,1920,1040},b{1920,0,4480,1400};
 auto r=fit({400,200,1400,800},a,b);
 check(r.left>=b.left&&r.right<=b.right&&r.top>=b.top&&r.bottom<=b.bottom,"window moves completely onto the other monitor");
 check(r.right-r.left==1000&&r.bottom-r.top==600,"normal window keeps pixel size");
 r=fit({400,200,1400,800},a,a);check(r.left==400&&r.top==200,"same monitor preserves position");
 r=fit({100,100,1100,700},a,{-1280,-300,0,700});check(r.left<0&&r.left>=-1280&&r.right<=0&&r.top>=-300&&r.bottom<=700,"left and vertically offset monitor supports negative coordinates");
 r=fit({0,0,1800,1000},a,{1920,40,2720,640});check(r.left==1920&&r.top==40&&r.right==2720&&r.bottom==640,"oversized window fits smaller usable area");
 r=fit({-3000,-2000,-2000,-1400},a,b);check(r.left==b.left&&r.top==b.top,"offscreen old position clamps safely");
 r=fit({1600,900,2600,1500},a,b);check(r.right==b.right&&r.bottom==b.bottom,"partly outside position clamps at far edge");
 r=fit({0,0,1920,1040},a,b);check(r.left==b.left&&r.top==b.top,"full source area maps to target origin");
 std::printf("PASS %d launch geometry checks\n",checks);
}
