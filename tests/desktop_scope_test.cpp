#include "../src/app.h"
#include "../src/entry_actions.h"
#include "../src/desktop_scope.h"
#include "../src/folder_badge.h"
#include <cstdio>
static int checks=0;
static void check(bool ok,const char* what){printf("%s %s\n",ok?"PASS":"FAIL",what);fflush(stdout);if(!ok)ExitProcess(1);++checks;}
static void dir(const std::wstring& p){check(CreateDirectoryW(p.c_str(),nullptr)!=FALSE,"create isolated directory");}
int wmain(int argc,wchar_t** argv){
 check(argc==2,"fixture argument");CoInitialize(nullptr);ULONG_PTR gdip;Gdiplus::GdiplusStartupInput gi;Gdiplus::GdiplusStartup(&gdip,&gi,nullptr);
 baseDir=argv[1];dir(baseDir);iniPath=baseDir+L"\\FolderIcon.ini";exePath=baseDir+L"\\FolderIconCN.exe";
 auto desktop=baseDir+L"\\桌面 A",child=desktop+L"\\真文件夹",nested=child+L"\\子文件夹",outside=baseDir+L"\\外部目录";
 dir(desktop);dir(child);dir(nested);dir(outside);
 check(WritePrivateProfileStringW(L"General",L"DesktopPath",desktop.c_str(),iniPath.c_str())!=FALSE,"write desktop override");
 loadConfig();bool already=false;std::wstring warning,error;
 check(EntryActions::enableDesktopFolder(child,already,warning,error),"configured desktop child accepted by shared service");
 check(warning.empty()&&!already,"first enable adds badge without warning");
 check(!EntryActions::enableDesktopFolder(desktop,already,warning,error),"desktop root rejected");
 check(!EntryActions::enableDesktopFolder(nested,already,warning,error),"grandchild rejected");
 check(!EntryActions::enableDesktopFolder(outside,already,warning,error),"outside folder rejected");
 check(EntryActions::setPanelExpansion(nested,true,error),"panel child expansion stays independent");
 check(cfg.desktopPath==desktop,"configured desktop loaded");
 check(DesktopScope::directChild(L"F:\\folder",L"F:\\"),"drive root direct child");
 check(DesktopScope::directChild(desktop+L"\\X",desktop+L"\\"),"trailing slash normalized");
 check(!DesktopScope::directChild(desktop+L" extra\\X",desktop),"same-prefix sibling rejected");
 check(DesktopScope::validate(child+L"\\..\\真文件夹",desktop,error),"dot segments normalize inside root");
 check(!DesktopScope::validate(desktop+L"\\..\\外部目录",desktop,error),"dot escape rejected");
 auto file=desktop+L"\\ordinary.txt";HANDLE h=CreateFileW(file.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,0,nullptr);check(h!=INVALID_HANDLE_VALUE,"create plain file");CloseHandle(h);
 check(!DesktopScope::validate(file,desktop,error),"plain file rejected");
 std::wstring root;check(!DesktopScope::resolve(baseDir+L"\\missing",root,error)&&!error.empty(),"missing override never falls back");
 check(!DesktopScope::resolve(L"relative",root,error),"relative override rejected");
 check(!DesktopScope::resolve(file,root,error),"file override rejected");
 std::wstring system;check(DesktopScope::systemPath(system)&&DesktopScope::resolve(L"",root,error)&&root==system,"automatic mode uses redirected system desktop");
 check(EntryActions::enableDesktopFolder(child,already,warning,error)&&already,"idempotent enable");
 check(EntryActions::setPanelExpansion(child,true,error),"dual role setup");
 auto oldRules=folderRules,draft=folderRules;
 auto deskB=baseDir+L"\\桌面 B";dir(deskB);
 check(DesktopScope::prune(draft,deskB)==1,"switch removes only desktop role");
 check(!FolderRules::contains(draft,child,true)&&FolderRules::contains(draft,child,false)&&FolderRules::contains(draft,nested,false),"switch preserves all panel roles");
 check(FolderRules::contains(folderRules,child,true),"draft pruning does not mutate live rules");
 std::wstring shortcut;check(EntryActions::createDesktopEntry(outside,desktop,shortcut,error),"arbitrary non-desktop source creates shortcut");
 check(entries.back().desktop==desktop,"new shortcut remembers location");
 check(GetFileAttributesW((outside+L"\\desktop.ini").c_str())==INVALID_FILE_ATTRIBUTES,"generated entry does not alter source icon");
 auto savedId=entries.back().id;cfg.desktopPath=deskB;folderRules=draft;
 check(saveConfig(nullptr),"commit changed root and role removal");loadConfig();
 check(cfg.desktopPath==deskB&&entries.back().id==savedId&&entries.back().desktop==desktop,"root and old shortcut location persist");
 check(EntryActions::syncDesktopBadges(oldRules,folderRules).empty(),"root switch restores old badge");
 check(GetFileAttributesW((child+L"\\desktop.ini").c_str())==INVALID_FILE_ATTRIBUTES,"old desktop badge gone but folder preserved");
 auto renamed=desktop+L"\\renamed entry.lnk";check(MoveFileW(shortcut.c_str(),renamed.c_str())!=FALSE,"rename generated shortcut");
 auto unrelated=desktop+L"\\unrelated.lnk";IShellLinkW* link=nullptr;IPersistFile* persist=nullptr;
 check(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link)))&&SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&persist))),"foreign link fixture");
 link->SetPath(exePath.c_str());link->SetArguments(L"--entry custom_other");check(SUCCEEDED(persist->Save(unrelated.c_str(),TRUE)),"write unrelated link");persist->Release();link->Release();
 check(EntryActions::removeDesktopEntry(entries.size()-1,warning,error)&&warning.empty(),"delete from remembered old root");
 check(GetFileAttributesW(renamed.c_str())==INVALID_FILE_ATTRIBUTES,"renamed owned link removed");
 check(GetFileAttributesW(unrelated.c_str())!=INVALID_FILE_ATTRIBUTES,"unrelated link protected");
 check(EntryActions::realFolder(outside)&&EntryActions::realFolder(child),"real sources preserved");
 std::vector<Entry> legacy={{L"a",L"a",L"",L""},{L"b",L"b",L"",L""},{L"c",L"c",L"",L""},{L"custom_d",L"d",outside,L""},{L"custom_e",L"e",outside,L"",deskB}};
 DesktopScope::rememberEntryLocations(legacy,desktop);check(legacy[0].desktop.empty()&&legacy[3].desktop==desktop&&legacy[4].desktop==deskB,"legacy location backfill preserves explicit and built-in entries");
 cfg.desktopPath=L"";check(saveConfig(nullptr),"save automatic mode");loadConfig();check(cfg.desktopPath.empty(),"automatic mode persists");
 printf("DESKTOP SCOPE %d checks passed\n",checks);Gdiplus::GdiplusShutdown(gdip);CoUninitialize();return 0;
}