#include "../src/folder_navigation.h"
#include <cassert>
#include <cstdio>
int main(){
 FolderNavigation n; n.reset(L"A",L"idA"); assert(n.path()==L"A"&&!n.hasParent());
 assert(!n.enter(L"self-junction",L"idA"));assert(!n.enter(L"missing",L""));
 assert(n.enter(L"B",L"idB"));assert(n.enter(L"C",L"idC"));
 assert(n.path()==L"C"&&n.hasParent());assert(n.back()&&n.path()==L"B");
 assert(n.back()&&n.path()==L"A");assert(!n.back()&&n.path()==L"A");
 assert(n.enter(L"B",L"idB"));assert(!n.enter(L"ancestor-junction",L"idA"));
 n.reset(L"root",L"0");for(int i=1;i<32;++i)assert(n.enter(std::to_wstring(i),std::to_wstring(i)));
 assert(!n.enter(L"too-deep",L"32"));for(int i=31;i>0;--i)assert(n.back());
 assert(n.path()==L"root"&&!n.back());n.clear();assert(n.path().empty()&&!n.hasParent());
 std::puts("folder navigation: 3 levels, exact back, self/ancestor loops, depth 32 PASS");
}
