#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <vector>
#include <string>
#include <cstdio>
#include "../src/folder_badge.h"
#include "../src/badge_visual.h"
static int count=0;
static void check(bool v,const char* s){printf("%s %s\n",v?"PASS":"FAIL",s);fflush(stdout);if(!v)ExitProcess(1);++count;}
static std::vector<BYTE> read(const std::wstring& p){HANDLE h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);check(h!=INVALID_HANDLE_VALUE,"read icon");DWORD n=GetFileSize(h,nullptr),got;std::vector<BYTE>b(n);check(ReadFile(h,b.data(),n,&got,nullptr)&&n==got,"read complete");CloseHandle(h);return b;}
static DWORD u32(const std::vector<BYTE>& b,size_t p){DWORD v;memcpy(&v,b.data()+p,4);return v;}
static void verifyGrid(){
 using namespace Gdiplus;
 for(int n:{16,20,24,32,40,48,64,96,128,256}){
  int d=std::max(8,n*32/100),reference=std::max(8,n*36/100);
  int tile=std::max(1,reference*21/100),gap=std::max(1,reference*11/100),left=(d-2*tile-gap)/2;
  printf("grid %d: frame=%d reference=%d tile=%d gap=%d\n",n,d,reference,tile,gap);
  check(BadgeVisual::extent(n)==d,"outer frame shrinks to 32 percent; 8px minimum");
  std::vector<BYTE> alpha;
  for(auto kind:{BadgeVisual::Kind::Shortcut,BadgeVisual::Kind::DesktopFolder,BadgeVisual::Kind::NestedFolder}){
   Bitmap image(n,n,PixelFormat32bppARGB);Graphics g(&image);g.Clear(Color(0,0,0,0));
   BadgeVisual::draw(g,(float)(n-d),0,n,kind);g.Flush(FlushIntentionSync);
   Color corner;image.GetPixel(n-d,0,&corner);check(corner.GetAlpha()<230,"rounded corner not solid square");
   bool outside=true,same=true,tiles=true,gaps=true;std::vector<BYTE> current;
   for(int y=0;y<n;++y)for(int x=0;x<n;++x){Color c;image.GetPixel(x,y,&c);current.push_back(c.GetAlpha());if(x<n-d||y>=d)outside&=c.GetAlpha()==0;}
   for(int row=0;row<2;++row)for(int col=0;col<2;++col)for(int y=0;y<tile;++y)for(int x=0;x<tile;++x){Color c;image.GetPixel(n-d+left+col*(tile+gap)+x,left+row*(tile+gap)+y,&c);tiles&=c.GetValue()==0xffffffff;}
   for(int y=left;y<left+2*tile+gap;++y)for(int x=left+tile;x<left+tile+gap;++x){Color c;image.GetPixel(n-d+x,y,&c);gaps&=c.GetValue()!=0xffffffff;}
   check(outside,"anti alias stays inside badge bounds");check(tiles,"four white tiles keep approved size");check(gaps,"approved gap remains colored, not white divider");
   if(alpha.empty())alpha=current;else same=alpha==current;check(same,"three kinds share identical alpha silhouette");
  }
 }
}
int wmain(int argc,wchar_t**argv){if(argc!=2)return 2;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);ULONG_PTR token;Gdiplus::GdiplusStartupInput startup;Gdiplus::GdiplusStartup(&token,&startup,nullptr);
 verifyGrid(); std::wstring root=argv[1],folder=root+L"\\plain",blue=root+L"\\shortcut.ico",error;CreateDirectoryW(root.c_str(),nullptr);CreateDirectoryW(folder.c_str(),nullptr);
 check(FolderBadge::shortcutIcon(folder,blue,error),"blue icon generated");check(FolderBadge::apply(root+L"\\store",folder,error),"orange icon generated");wchar_t path[32768]{};GetPrivateProfileStringW(L".ShellClassInfo",L"IconFile",L"",path,32768,(folder+L"\\desktop.ini").c_str());auto a=read(blue),b=read(path);check(a[4]==10&&b[4]==10,"ten DPI frames");
 for(int f=0;f<10;++f){int size=a[6+16*f]?a[6+16*f]:256;size_t xoff=u32(a,6+16*f+12)+40,yoff=u32(b,6+16*f+12)+40;int d=std::max(8,size*32/100),geometry=0,outside=0,colors=0;
  for(int y=0;y<size;++y)for(int x=0;x<size;++x){size_t p=((size-1-y)*size+x)*4;bool badge=x>=size-d&&y<d;if(!badge){if(memcmp(a.data()+xoff+p,b.data()+yoff+p,4))++outside;}else{if(abs((int)a[xoff+p+3]-(int)b[yoff+p+3])>2)++geometry;if(a[xoff+p+3]>240&&b[yoff+p+3]>240&&abs((int)a[xoff+p]-(int)b[yoff+p])>30)++colors;}}
  printf("frame %d: silhouette differences=%d outside=%d color differences=%d\n",size,geometry,outside,colors);
  check(geometry==0,"same badge silhouette at every DPI");check(outside==0,"same underlying icon outside badge");check(colors>0,"types retain distinct colors");
 }
 check(FolderBadge::restore(root+L"\\store",folder,error),"restorable after style change");Gdiplus::GdiplusShutdown(token);CoUninitialize();printf("PASS %d badge style checks\n",count);return 0;
}