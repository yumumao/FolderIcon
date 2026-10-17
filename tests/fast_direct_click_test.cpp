#include "../src/fast_direct_click.h"
#include <cassert>
#include <cstdio>
int main(){
 FastDirectClick state;
 auto a=state.press(1,true,10,10,100,500,4,4);assert(!a.convert&&state.claim(a.gesture));
 auto b=state.press(1,true,11,10,200,500,4,4);assert(b.convert&&!state.claim(b.gesture));
 auto c=state.press(1,true,10,10,700,500,4,4);assert(!c.convert&&state.claim(c.gesture));
 state.reset();a=state.press(1,true,10,10,100,500,4,4);b=state.press(1,true,10,10,200,500,4,4);
 assert(!b.convert&&!state.claim(a.gesture)&&!state.claim(b.gesture)); // native double won while resolving
 state.reset();a=state.press(1,true,10,10,100,500,4,4);state.cancel(a.gesture);assert(!state.claim(a.gesture));
 state.reset();a=state.press(1,true,10,10,100,500,4,4);b=state.press(1,false,10,10,200,500,4,4);assert(!b.convert&&!state.claim(a.gesture)&&!state.claim(b.gesture));
 state.reset();a=state.press(1,true,10,10,100,500,4,4);assert(state.claim(a.gesture));b=state.press(2,true,10,10,200,500,4,4);assert(!b.convert&&state.claim(b.gesture));
 state.reset();a=state.press(1,true,10,10,100,500,4,4);assert(state.claim(a.gesture));b=state.press(1,true,12,10,200,500,4,4);assert(!b.convert&&state.claim(b.gesture));
 state.reset();a=state.press(1,true,10,10,0xffffff00,500,4,4);assert(state.claim(a.gesture));b=state.press(1,true,10,10,0x40,500,4,4);assert(b.convert);
 state.reset();a=state.press(1,true,10,10,100,500,4,4);b=state.press(1,true,100,100,200,500,4,4);assert(!state.claim(a.gesture)&&state.claim(b.gesture));
 puts("PASS immediate claim, duplicate, native-race, drag, modifiers, windows, edges, wrap, stale click");
}
