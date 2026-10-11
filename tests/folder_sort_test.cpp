#include "../src/app.h"
#include "../src/folder_sort.h"
#include <cassert>
#include <cstdio>
using namespace FolderSort;
int wmain(int argc,wchar_t**argv){
 assert(argc==2);std::wstring root=argv[1];CreateDirectoryW(root.c_str(),nullptr);
 assert(Config{}.expandDelay==80);
 Preferences p;assert(p.mode==Mode::Name&&!p.descending);
 std::vector<Item> items={{L"item10.txt",L"item10.txt",false,10},{L"item2.txt",L"item2.txt",false,20},{L"folder",L"folder",true,5},{L"same.url",L"same",false,30},{L"same.lnk",L"same",false,30}};
 auto order=indices(items,p);assert(order==std::vector<size_t>({2,1,0,4,3}));
 p.descending=true;order=indices(items,p);assert(order==std::vector<size_t>({3,4,0,1,2}));
 p.mode=Mode::Modified;p.descending=false;order=indices(items,p);assert(order==std::vector<size_t>({2,0,1,4,3}));
 p.descending=true;order=indices(items,p);assert(order==std::vector<size_t>({4,3,1,0,2}));
 p.mode=Mode::Custom;p.custom={L"same.url",L"item10.txt",L"same.url",L"missing"};
 order=indices(items,p);assert(order==std::vector<size_t>({3,0,2,1,4}));
 p.descending=false;assert(indices(items,p)==order);
 assert(indices({},p).empty());
 auto merged=mergeCustom({L"a",L".hidden",L"b"},{L"b",L"a",L"new"});
 assert(merged==std::vector<std::wstring>({L"b",L".hidden",L"a",L"new"}));
 assert(mergeCustom({},{}).empty());
 assert(mergeCustom({L"a",L"a"},{L"a"})==std::vector<std::wstring>({L"a"}));
 std::vector<int> n={0,1,2,3};assert(move(n,0,3));assert(n==std::vector<int>({1,2,3,0}));
 assert(move(n,3,0));assert(n==std::vector<int>({0,1,2,3}));
 assert(!move(n,-1,0));assert(!move(n,0,4));assert(!move(n,0,0));
 auto folder=root+L"\\Example";auto other=root+L"\\Other";
 assert(filePath(root,folder)==filePath(root,root+L"\\example\\"));
 assert(filePath(root,folder)!=filePath(root,other));
 assert(load(root,folder).mode==Mode::Name);
 p.custom={L"中文.txt",L"a=b.url",L"quoted \"name\"",L".hidden"};
 assert(save(root,folder,p));auto q=load(root,folder);assert(q.mode==p.mode&&q.descending==p.descending&&q.custom==p.custom);
 assert(load(root,other).mode==Mode::Name);
 p.mode=Mode::Modified;p.descending=true;assert(save(root,folder,p));q=load(root,folder);assert(q.mode==p.mode&&q.descending&&q.custom==p.custom);
 // A corrupt file must fall back, never crash or allocate from unchecked lengths.
 auto path=filePath(root,folder);HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);assert(h!=INVALID_HANDLE_VALUE);
 const char bad[]="corrupt";DWORD written;WriteFile(h,bad,sizeof(bad),&written,nullptr);CloseHandle(h);assert(load(root,folder).mode==Mode::Name);
 assert(save(root,folder,p));SetFileAttributesW(path.c_str(),FILE_ATTRIBUTE_READONLY);assert(!save(root,folder,Preferences{}));
 assert(load(root,folder).mode==Mode::Modified);SetFileAttributesW(path.c_str(),FILE_ATTRIBUTE_NORMAL);
 puts("folder sorting, default delay, persistence, corrupt input and save rollback assertions passed");
}
