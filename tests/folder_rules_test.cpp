#define UNICODE
#define _UNICODE
#include <windows.h>
#include "../src/folder_rules.h"
#include <cassert>
#include <cstdio>
using namespace FolderRules;
int wmain(int argc,wchar_t** argv){
 assert(argc==2);std::wstring root=argv[1];CreateDirectoryW(root.c_str(),nullptr);
 Rules rules;assert(!contains(rules,root,false));
 assert(set(rules,root+L"\\Child",true,false));
 assert(contains(rules,root+L"/CHILD/",false));assert(!contains(rules,root+L"\\Child",true));
 assert(!contains(rules,root+L"\\Child\\Grandchild",false));
 assert(set(rules,root+L"\\child\\..\\Child",false,true));assert(rules.size()==1);
 assert(!contains(rules,root+L"\\Child",false));assert(contains(rules,root+L"\\Child",true));
 assert(!set(rules,L"relative",true,true));assert(!set(rules,L"C:relative",true,true));
 assert(!set(rules,L"\\rooted",true,true));assert(!set(rules,L"shell:Desktop",true,true));
 auto file=root+L"\\folder-rules.bin";assert(save(file,rules));Rules loaded;assert(load(file,loaded));
 assert(loaded.size()==1&&loaded[0].desktop&&!loaded[0].panel);
 assert(set(rules,root+L"\\第二层",true,true));assert(contains(rules,root+L"\\第二层",false));assert(contains(rules,root+L"\\第二层",true));assert(save(file,rules));assert(load(file,loaded)&&loaded.size()==2);
 assert(set(rules,root+L"\\Child",false,false));assert(rules.size()==1);
 assert(!save(root+L"\\missing\\rules.bin",rules));assert(load(file,loaded)&&loaded.size()==2);
 HANDLE f=CreateFileW(file.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);assert(f!=INVALID_HANDLE_VALUE);SetEndOfFile(f);CloseHandle(f);
 assert(!load(file,loaded)&&loaded.empty());assert(load(root+L"\\absent.bin",loaded)&&loaded.empty());
 auto id=identity(root);assert(!id.empty());assert(identity(root+L"\\missing").empty());
 assert(!canEnter({id},id));assert(!canEnter({},L""));assert(canEnter({},id));
 assert(!canEnter(std::vector<std::wstring>(32,L"old"),id));
 std::puts("folder rules: normalization, isolation, atomic persistence, corruption and navigation PASS");
}
