#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <cstdio>
#include "../src/folder_badge.h"
static int checks=0;
static void require(bool ok,const char* name){printf("%s %s\n",ok?"PASS":"FAIL",name);fflush(stdout);if(!ok)ExitProcess(1);++checks;}
static std::vector<char> bytes(const std::wstring& p){HANDLE h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(h==INVALID_HANDLE_VALUE)return {};DWORD n=GetFileSize(h,nullptr),got=0;std::vector<char>b(n);ReadFile(h,b.data(),n,&got,nullptr);CloseHandle(h);require(got==n,"read fixture");return b;}
static void write(const std::wstring& p,const std::wstring& t){HANDLE h=CreateFileW(p.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);require(h!=INVALID_HANDLE_VALUE,"write fixture handle");DWORD n;WORD bom=0xfeff;WriteFile(h,&bom,2,&n,nullptr);WriteFile(h,t.data(),(DWORD)t.size()*2,&n,nullptr);CloseHandle(h);}
static void rawWrite(const std::wstring& p,const std::vector<char>& b){
 HANDLE h=CreateFileW(p.c_str(),GENERIC_WRITE,0,nullptr,GetFileAttributesW(p.c_str())==INVALID_FILE_ATTRIBUTES?CREATE_NEW:TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(h!=INVALID_HANDLE_VALUE,"raw fixture handle");DWORD n;require(WriteFile(h,b.data(),(DWORD)b.size(),&n,nullptr)&&n==b.size(),"raw fixture write");CloseHandle(h);
}
static std::wstring value(const std::wstring& p,const wchar_t* key){wchar_t b[32768]{};GetPrivateProfileStringW(L".ShellClassInfo",key,L"",b,32768,p.c_str());return b;}
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);ULONG_PTR gdip;Gdiplus::GdiplusStartupInput gi;Gdiplus::GdiplusStartup(&gdip,&gi,nullptr);
 std::wstring parent=argv[1];CreateDirectoryW(parent.c_str(),nullptr);std::wstring root=parent+L"\\run-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()),store=root+L"\\store",a=root+L"\\plain",b=root+L"\\custom",error;
 CreateDirectoryW(root.c_str(),nullptr);CreateDirectoryW(a.c_str(),nullptr);CreateDirectoryW(b.c_str(),nullptr);
 auto ini=a+L"\\desktop.ini";auto origAttr=GetFileAttributesW(a.c_str());write(a+L"\\keep.txt",L"Do not change content");auto payload=bytes(a+L"\\keep.txt");
 require(FolderBadge::apply(store,a,error),"apply to plain folder");
 require((GetFileAttributesW(a.c_str())&FILE_ATTRIBUTE_READONLY)!=0,"folder customization enabled");
 require((GetFileAttributesW(ini.c_str())&(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM),"desktop.ini hidden/system");
 auto icon=value(ini,L"IconFile");require(!icon.empty()&&PathFileExistsW(icon.c_str()),"static ICO generated in app storage");
 HICON ico=(HICON)LoadImageW(nullptr,icon.c_str(),IMAGE_ICON,48,48,LR_LOADFROMFILE);require(ico!=nullptr,"generated ICO decodes");if(ico)DestroyIcon(ico);
 auto applied=bytes(ini);require(FolderBadge::apply(store,a,error)&&bytes(ini)==applied,"apply idempotent, no nested badge");
 require(FolderBadge::restore(store,a,error),"restore plain folder");require(!PathFileExistsW(ini.c_str()),"remove only tool-created unmodified metadata");require(GetFileAttributesW(a.c_str())==origAttr,"restore original folder attributes");require(bytes(a+L"\\keep.txt")==payload,"business file unchanged");
 ini=b+L"\\desktop.ini";write(ini,L"; keep comment\r\n[.ShellClassInfo]\r\nIconResource=old.dll,3\r\nIconFile=old.ico\r\nIconIndex=2\r\nInfoTip=original\r\n[Other]\r\nValue=preserve\r\n");auto original=bytes(ini);auto ia=GetFileAttributesW(ini.c_str());
 require(FolderBadge::apply(store,b,error),"apply custom folder");require(value(ini,L"InfoTip")==L"original","preserve other keys");require(FolderBadge::restore(store,b,error)&&bytes(ini)==original,"restore original INI byte-for-byte");require(GetFileAttributesW(ini.c_str())==ia,"restore original INI attributes");
 require(FolderBadge::apply(store,b,error),"reapply after restore");require(WritePrivateProfileStringW(L".ShellClassInfo",L"InfoTip",L"user-edited-later",ini.c_str())!=0,"simulate later unrelated edit");
 require(FolderBadge::restore(store,b,error),"merge restore after unrelated edits");require(value(ini,L"InfoTip")==L"user-edited-later"&&value(ini,L"IconResource")==L"old.dll,3"&&value(ini,L"IconFile")==L"old.ico"&&value(ini,L"IconIndex")==L"2","preserve newer data while restoring old icon keys");
 require(FolderBadge::apply(store,b,error),"apply before conflict test");require(WritePrivateProfileStringW(L".ShellClassInfo",L"IconFile",L"users-new.ico",ini.c_str())!=0,"simulate user icon edit");auto conflict=bytes(ini);require(!FolderBadge::restore(store,b,error)&&bytes(ini)==conflict,"refuse to overwrite newer user icon");require(!FolderBadge::apply(store,b,error)&&bytes(ini)==conflict,"reapply also preserves user icon conflict");
 ini=a+L"\\desktop.ini";require(FolderBadge::apply(store,a,error),"apply plain before external metadata");WritePrivateProfileStringW(L".ShellClassInfo",L"InfoTip",L"new metadata",ini.c_str());require(FolderBadge::restore(store,a,error)&&value(ini,L"InfoTip")==L"new metadata"&&value(ini,L"IconFile").empty(),"do not delete tool-created INI with later user data");
 auto c=root+L"\\readonly";CreateDirectoryW(c.c_str(),nullptr);write(c+L"\\desktop.ini",L"[.ShellClassInfo]\r\nInfoTip=protected\r\n");SetFileAttributesW((c+L"\\desktop.ini").c_str(),FILE_ATTRIBUTE_READONLY);auto immutable=bytes(c+L"\\desktop.ini");require(!FolderBadge::apply(store,c,error)&&bytes(c+L"\\desktop.ini")==immutable,"respect read-only existing metadata");
 auto d=root+L"\\hardlink";CreateDirectoryW(d.c_str(),nullptr);require(CreateHardLinkW((d+L"\\desktop.ini").c_str(),(c+L"\\desktop.ini").c_str(),nullptr)!=0,"create hardlink fixture");require(!FolderBadge::apply(store,d,error),"reject hardlinked metadata");
 auto e=root+L"\\duplicate";CreateDirectoryW(e.c_str(),nullptr);write(e+L"\\desktop.ini",L"[.ShellClassInfo]\r\nIconFile=a.ico\r\nIconFile=b.ico\r\n");auto duplicate=bytes(e+L"\\desktop.ini");require(!FolderBadge::apply(store,e,error)&&bytes(e+L"\\desktop.ini")==duplicate,"reject ambiguous duplicate icon keys");
 require(!FolderBadge::apply(store,root+L"\\missing",error),"reject nonexistent folder");
 // Simulate a process exit after restoring bytes, before restoring attributes.
 auto resumed=root+L"\\resumed";CreateDirectoryW(resumed.c_str(),nullptr);
 auto ri=resumed+L"\\desktop.ini";write(ri,L"[.ShellClassInfo]\r\nIconFile=before.ico\r\nInfoTip=keep\r\n");auto rb=bytes(ri);
 require(FolderBadge::apply(store,resumed,error),"apply before interrupted restore");
 auto generated=value(ri,L"IconFile");auto stateDir=generated.substr(0,generated.find_last_of(L'\\'));
 auto from=bytes(ri);rawWrite(stateDir+L"\\restoring-from.ini",from);rawWrite(stateDir+L"\\restoring-to.ini",rb);
 auto st=bytes(stateDir+L"\\state.bin");require(st.size()==20,"state fixture size");st[16]=4;rawWrite(stateDir+L"\\state.bin",st);rawWrite(ri,rb);
 require(FolderBadge::restore(store,resumed,error)&&bytes(ri)==rb,"resume interrupted restore without false user-icon conflict");
 require(FolderBadge::restore(store,resumed,error),"repeat restore is safe");
 require(FolderBadge::apply(store,resumed,error),"apply before rename");auto renamed=root+L"\\renamed";
 require(MoveFileW(resumed.c_str(),renamed.c_str())!=FALSE,"rename fixture on same volume");
 require(FolderBadge::restore(store,renamed,error)&&bytes(renamed+L"\\desktop.ini")==rb,"restore by directory identity after rename");
 auto utf=root+L"\\utf8";CreateDirectoryW(utf.c_str(),nullptr);std::string text="\xef\xbb\xbf[.ShellClassInfo]\r\nInfoTip=中文注释\r\n";std::vector<char> utfbytes(text.begin(),text.end());rawWrite(utf+L"\\desktop.ini",utfbytes);
 require(FolderBadge::apply(store,utf,error)&&FolderBadge::restore(store,utf,error)&&bytes(utf+L"\\desktop.ini")==utfbytes,"UTF8 BOM Chinese round trip");
 require(!FolderBadge::apply(root+L"\\plain\\keep.txt",utf,error),"reject non-directory storage without modifying folder");
 // Versioned ICO references must migrate desktop.ini AND applied.ini together.
 for(int pass=0;pass<2;++pass){
  auto vf=root+L"\\versioned-"+std::to_wstring(pass);CreateDirectoryW(vf.c_str(),nullptr);auto vi=vf+L"\\desktop.ini";
  write(vi,L"; preserved comment\r\n[.ShellClassInfo]\r\nIconResource=original.dll,7\r\nInfoTip=keep\r\n[Other]\r\nValue=preserve\r\n");auto vb=bytes(vi);auto va=GetFileAttributesW(vf.c_str());
  require(FolderBadge::apply(store,vf,error),"apply version migration fixture");auto oldIcon=value(vi,L"IconFile"),dir=oldIcon.substr(0,oldIcon.find_last_of(L'\\')),nextIcon=dir+L"\\panel14.ico";auto originalRecord=bytes(dir+L"\\original.ini"),stateRecord=bytes(dir+L"\\state.bin");
  require(CopyFileW(oldIcon.c_str(),nextIcon.c_str(),TRUE)!=0,"copy versioned icon without replacing original");
  require(WritePrivateProfileStringW(L".ShellClassInfo",L"IconFile",nextIcon.c_str(),vi.c_str())!=0,"migrate target reference");auto incomplete=bytes(vi);
  require(!FolderBadge::restore(store,vf,error)&&bytes(vi)==incomplete,"incomplete migration refuses to overwrite metadata");
  require(WritePrivateProfileStringW(L".ShellClassInfo",L"IconFile",nextIcon.c_str(),(dir+L"\\applied.ini").c_str())!=0,"migrate matching recovery reference");
  require(bytes(dir+L"\\original.ini")==originalRecord&&bytes(dir+L"\\state.bin")==stateRecord,"migration preserves original record and state");
  require(FolderBadge::apply(store,vf,error)&&value(vi,L"IconFile")==nextIcon,"versioned reference remains idempotent");
  if(pass)require(WritePrivateProfileStringW(L".ShellClassInfo",L"InfoTip",L"later edit",vi.c_str())!=0,"later edit after versioning");
  require(FolderBadge::restore(store,vf,error),"restore works after versioned reference migration");
  require(pass?(value(vi,L"InfoTip")==L"later edit"&&value(vi,L"IconResource")==L"original.dll,7"):(bytes(vi)==vb),"versioned restore preserves exact original or newer unrelated edits");
  require(GetFileAttributesW(vf.c_str())==va,"versioned restore returns original directory attributes");
 }
 printf("ALL %d checks passed\n",checks);Gdiplus::GdiplusShutdown(gdip);CoUninitialize();return 0;
}