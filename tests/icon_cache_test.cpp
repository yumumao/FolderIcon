#include "../src/icon_cache.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <iostream>
using namespace IconCache;
static void touch(const std::wstring& p,const char* data){FILE* f=_wfopen(p.c_str(),L"wb");assert(f);fputs(data,f);fclose(f);}
static HBITMAP image(){BITMAPINFO b={};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=32;b.bmiHeader.biHeight=-32;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;void* bits=nullptr;HBITMAP h=CreateDIBSection(nullptr,&b,DIB_RGB_COLORS,&bits,nullptr,0);assert(h);for(int i=0;i<32*32;i++)((DWORD*)bits)[i]=0x80402010;return h;}
static void orientation(const std::wstring& base,bool topDown){
 auto source=base+(topDown?L"\\top-down.txt":L"\\bottom-up.txt");touch(source,"asymmetric icon");
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=32;
 info.bmiHeader.biHeight=topDown?-32:32;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
 DWORD* bits=nullptr;HBITMAP a=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,(void**)&bits,nullptr,0);assert(a);
 for(int y=0;y<32;y++)for(int x=0;x<32;x++)bits[(topDown?y:31-y)*32+x]=y<16?0x80402010:0xc0306080;
 DIBSECTION ds{};GetObjectW(a,sizeof(ds),&ds);
 std::cout<<"orientation input topDown="<<topDown<<" reportedHeight="<<ds.dsBmih.biHeight<<std::endl;
 assert(store(base+L"\\orientation-cache",source,32,a));
 HBITMAP b=load(base+L"\\orientation-cache",source,32);assert(b);
 BITMAP bm{};GetObjectW(b,sizeof(bm),&bm);
 const auto* out=(const DWORD*)bm.bmBits;
 std::cout<<std::hex<<"first="<<out[0]<<" last="<<out[31*32]<<std::dec<<std::endl;
 assert(out[0]==0x80402010&&out[31*32]==0xc0306080);
 // Repeated warm loads must not alternate orientation and must preserve alpha.
 assert(store(base+L"\\orientation-cache",source,32,b));DeleteObject(b);
 b=load(base+L"\\orientation-cache",source,32);assert(b);GetObjectW(b,sizeof(bm),&bm);
 out=(const DWORD*)bm.bmBits;assert(out[0]==0x80402010&&out[31*32]==0xc0306080);
 DeleteObject(b);DeleteObject(a);
}
static int worker(const std::wstring& base){
 HBITMAP a=image();int writes=0;
 for(int i=0;i<25;i++){
  if(store(base+L"\\parallel-cache",base+L"\\shared-source.txt",32,a))writes++;
  HBITMAP b=load(base+L"\\parallel-cache",base+L"\\shared-source.txt",32);
  if(b){BITMAP bm{};GetObjectW(b,sizeof(bm),&bm);assert(bm.bmWidth==32&&((DWORD*)bm.bmBits)[0]==0x80402010);DeleteObject(b);}
 }DeleteObject(a);return writes?0:1;
}
static void concurrent(const std::wstring& base){
 touch(base+L"\\shared-source.txt","shared");wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);
 std::vector<HANDLE> workers;
 for(int i=0;i<4;i++){
  std::wstring command=L"\""+std::wstring(exe)+L"\" --parallel \""+base+L"\"";
  STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
  assert(CreateProcessW(exe,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi));
  CloseHandle(pi.hThread);workers.push_back(pi.hProcess);
 }
 for(int i=0;i<5;i++){rebuild(base+L"\\parallel-cache");Sleep(5);}
 for(HANDLE h:workers){assert(WaitForSingleObject(h,30000)==WAIT_OBJECT_0);DWORD code;assert(GetExitCodeProcess(h,&code)&&code==0);CloseHandle(h);}
 HBITMAP a=image();assert(store(base+L"\\parallel-cache",base+L"\\shared-source.txt",32,a));DeleteObject(a);
 assert(rebuild(base+L"\\parallel-cache"));
}
int wmain(int argc,wchar_t** argv){if(argc==3&&!wcscmp(argv[1],L"--parallel"))return worker(argv[2]);assert(argc==2);std::wstring base=argv[1];CreateDirectoryW(base.c_str(),nullptr);auto root=base+L"\\cache",source=base+L"\\sample.txt";touch(source,"first");
 assert(!load(root,source,32));HBITMAP a=image();assert(store(root,source,32,a));HBITMAP b=load(root,source,32);assert(b);BITMAP bm{};GetObjectW(b,sizeof(bm),&bm);assert(bm.bmWidth==32&&bm.bmHeight==32);assert(((DWORD*)bm.bmBits)[0]==0x80402010);DeleteObject(b);
 assert(!load(root,source,48));touch(source,"changed with different length");assert(!load(root,source,32));assert(store(root,source,32,a));b=load(root,source,32);assert(b);DeleteObject(b);
 auto path=filePath(root,source,32);
 // Old version-1 payloads may contain inverted rows; never reuse them.
 {FILE* f=_wfopen(path.c_str(),L"r+b");assert(f);DWORD legacy=1;fseek(f,8,SEEK_SET);assert(fwrite(&legacy,4,1,f)==1);fclose(f);}
 assert(!load(root,source,32));assert(store(root,source,32,a));touch(path,"broken");assert(!load(root,source,32));assert(store(root,source,32,a));
 auto directory=root+L"\\icons-v1";touch(directory+L"\\keep.txt","not owned cache");touch(directory+L"\\manual.fic","not generated cache");assert(rebuild(root));assert(!load(root,source,32));assert(GetFileAttributesW((directory+L"\\keep.txt").c_str())!=INVALID_FILE_ATTRIBUTES);assert(GetFileAttributesW((directory+L"\\manual.fic").c_str())!=INVALID_FILE_ATTRIBUTES);
 touch(base+L"\\blocked","file not a directory");assert(!store(base+L"\\blocked",source,32,a));assert(!load(base+L"\\blocked",source,32));assert(!rebuild(base+L"\\blocked"));
 assert(!store(root,source,0,a));assert(!load(root,source,0));assert(!store(root,base+L"\\missing",32,a));
 assert(store(root,source,32,a));
 // A matching-size payload with changed bytes must be rejected, not rendered.
 {FILE* f=_wfopen(filePath(root,source,32).c_str(),L"r+b");assert(f);fseek(f,-1,SEEK_END);fputc(0x33,f);fclose(f);}
 assert(!load(root,source,32));assert(store(root,source,32,a));
 assert(filePath(root,base+L"\\.\\sample.txt",32)==filePath(root,source,32));
 assert(!store(root,source,257,a));assert(!store(root,source,32,nullptr));
 assert(store(root,base,32,a));b=load(root,base,32);assert(b);DeleteObject(b);
 auto unicode=base+L"\\\u6d4b\u8bd5.txt";touch(unicode,"unicode");assert(store(root,unicode,32,a));b=load(root,unicode,32);assert(b);DeleteObject(b);
 // Same file size, changed modification time.
 {HANDLE f=CreateFileW(source.c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(f!=INVALID_HANDLE_VALUE);FILETIME t{12345,98765};assert(SetFileTime(f,nullptr,nullptr,&t));CloseHandle(f);}
 assert(!load(root,source,32));assert(store(root,source,32,a));
 DWORD before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 for(int i=0;i<100;i++){b=load(root,source,32);assert(b);DeleteObject(b);}
 assert(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)==before);
 DeleteFileW(source.c_str());assert(!load(root,source,32));assert(rebuild(root));DeleteObject(a);
 orientation(base,true);orientation(base,false);concurrent(base);
 std::cout<<"icon cache regression assertions passed\n";
}
