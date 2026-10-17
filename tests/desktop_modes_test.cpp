#include "../src/app.h"
#include "../src/desktop_modes.h"
#include "../src/direct_click.h"
#include "../src/entry_maintenance.h"
#include "../src/entry_actions.h"
#include <cstdio>
static int checks=0;
static void check(bool v,const char* text){printf("%s %s\n",v?"PASS":"FAIL",text);fflush(stdout);if(!v)ExitProcess(1);++checks;}
static std::vector<BYTE> bytes(const std::wstring&p){FILE*f=_wfopen(p.c_str(),L"rb");if(!f)return {};fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);std::vector<BYTE>b(n);fread(b.data(),1,n,f);fclose(f);return b;}
static void put(const std::wstring&p,const std::wstring&s){FILE*f=_wfopen(p.c_str(),L"wb");check(f!=nullptr,"fixture writable");WORD bom=0xfeff;fwrite(&bom,2,1,f);fwrite(s.data(),2,s.size(),f);fclose(f);}
static void link(const std::wstring&p,const std::wstring&target,const std::wstring&args){IShellLinkW*l=nullptr;IPersistFile*f=nullptr;check(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&l))),"link COM");l->SetPath(target.c_str());l->SetArguments(args.c_str());l->SetWorkingDirectory(baseDir.c_str());l->SetShowCmd(SW_SHOWMINNOACTIVE);l->SetDescription(L"Synthetic preserved description");l->QueryInterface(IID_PPV_ARGS(&f));check(SUCCEEDED(f->Save(p.c_str(),TRUE)),"fixture link");f->Release();l->Release();}
static std::wstring stopAt,raceSource,raceDestination;
static void interrupted(const wchar_t*stage){if(stopAt==stage)ExitProcess(71);if(std::wstring(stage)==L"replace-ready"&&!raceSource.empty()){check(CopyFileW(raceSource.c_str(),raceDestination.c_str(),FALSE)!=FALSE,"external save injected at commit boundary");raceSource.clear();}}
static int child(const std::wstring&root,const std::wstring&path,const wchar_t*stage,int mode){
 wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);std::wstring cmd=L"\""+std::wstring(exe)+L"\" \""+root+L"\" \""+path+L"\" "+stage+L" "+std::to_wstring(mode);
 STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};check(CreateProcessW(nullptr,cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi)!=FALSE,"interrupted writer started");
 check(WaitForSingleObject(pi.hProcess,15000)==WAIT_OBJECT_0,"writer finished within timeout");DWORD code=0;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return code;
}
int wmain(int argc,wchar_t**argv){if(argc!=2&&argc!=5)return 2;CoInitialize(nullptr);ULONG_PTR token;Gdiplus::GdiplusStartupInput gi;Gdiplus::GdiplusStartup(&token,&gi,nullptr);
 baseDir=argv[1];SHCreateDirectoryExW(nullptr,baseDir.c_str(),nullptr);iniPath=baseDir+L"\\FolderIcon.ini";exePath=baseDir+L"\\FolderIconCN.exe";folderRulesPath=baseDir+L"\\folder-rules.bin";
 if(argc==5){loadConfig();stopAt=argv[3];if(stopAt==L"replace-applied"){raceSource=baseDir+L"\\external.lnk";raceDestination=argv[2];}if(stopAt==L"locked"){auto before=bytes(iniPath);std::wstring ignored,out;bool deferred=EntryMaintenance::reconcile().deferred>0;bool blocked=!saveConfig(nullptr)&&!EntryActions::setPanelExpansion(argv[2],true,ignored)&&!EntryActions::createDesktopEntry(argv[2],cfg.desktopPath,out,ignored);return deferred&&blocked&&before==bytes(iniPath)?73:74;}DesktopModes::checkpoint=interrupted;std::wstring e;return DesktopModes::set(argv[2],(DesktopModes::Mode)_wtoi(argv[4]),e)?0:72;}
 auto desk=baseDir+L"\\Desktop",folder=desk+L"\\Folder 中文",target=baseDir+L"\\Target",app=desk+L"\\Application.lnk",dirlink=desk+L"\\Directory.lnk";
 SHCreateDirectoryExW(nullptr,folder.c_str(),nullptr);CreateDirectoryW(target.c_str(),nullptr);put(iniPath,L"[General]\r\nDesktopPath="+desk+L"\r\n[Appearance]\r\nFollowDesktop=0\r\nFollowDesktopFont=0\r\n");loadConfig();DesktopModes::load();
 wchar_t sys[32768]{};GetSystemDirectoryW(sys,32768);link(app,std::wstring(sys)+L"\\cmd.exe",L"/c echo synthetic");link(dirlink,target,L"");auto original=bytes(app);std::wstring error;
 auto info=DesktopModes::inspect(app);check(info.supported&&!info.canPanel,"application shortcut only permits direct/default");check(!DesktopModes::set(app,DesktopModes::Mode::Panel,error),"app cannot become a folder panel");
 check(DesktopModes::set(app,DesktopModes::Mode::Direct,error),"enable direct on shortcut");DesktopModes::load();check(DesktopModes::inspect(app).mode==DesktopModes::Mode::Direct,"direct mode persists");
 IShellLinkW*l=nullptr;IPersistFile*f=nullptr;CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&l));l->QueryInterface(IID_PPV_ARGS(&f));f->Load(app.c_str(),STGM_READ);wchar_t args[512]{},work[32768]{},icon[32768]{};int show=0,index=0;l->GetArguments(args,512);l->GetWorkingDirectory(work,32768);l->GetShowCmd(&show);l->GetIconLocation(icon,32768,&index);f->Release();l->Release();
 check(std::wstring(args)==L"/c echo synthetic"&&std::wstring(work)==baseDir&&show==SW_SHOWMINNOACTIVE,"shortcut arguments working directory and show state preserved");check(icon[0]&&GetFileAttributesW(icon)!=INVALID_FILE_ATTRIBUTES,"direct shortcut has generated icon");
 check(DesktopModes::repairBadge(app,error),"repair direct badge keeps mode");
 check(DesktopModes::inspect(app).mode==DesktopModes::Mode::Direct,"repair does not drop direct rule");
 auto once=bytes(app);check(DesktopModes::set(app,DesktopModes::Mode::Direct,error)&&bytes(app)==once,"same mode is idempotent");check(DesktopModes::set(app,DesktopModes::Mode::Native,error)&&bytes(app)==original,"restore original shortcut bytes");
 check(DesktopModes::set(dirlink,DesktopModes::Mode::Panel,error),"folder shortcut supports panel in place");check(DesktopModes::inspect(dirlink).target==target,"panel uses resolved folder");check(DesktopModes::set(dirlink,DesktopModes::Mode::Direct,error),"panel shortcut switches to direct");check(DesktopModes::set(dirlink,DesktopModes::Mode::Native,error),"folder link restored");
 auto attrs=GetFileAttributesW(folder.c_str());check(DesktopModes::set(folder,DesktopModes::Mode::Panel,error),"folder panel enabled");check(FolderRules::contains(folderRules,folder,true),"existing settings tracks panel folder");
 check(DesktopModes::set(folder,DesktopModes::Mode::Direct,error),"folder switches to direct");check(!FolderRules::contains(folderRules,folder,true),"panel/direct mutually exclusive");check(DesktopModes::set(folder,DesktopModes::Mode::Native,error),"folder restored");check(GetFileAttributesW((folder+L"\\desktop.ini").c_str())==INVALID_FILE_ATTRIBUTES&&GetFileAttributesW(folder.c_str())==attrs,"original folder metadata restored");
 put(desk+L"\\file.txt",L"payload");check(!DesktopModes::inspect(desk+L"\\file.txt").supported,"plain file not silently converted");check(!DesktopModes::inspect(target).supported,"external folder excluded from in-place mode");
 SetFileAttributesW(app.c_str(),FILE_ATTRIBUTE_READONLY);check(!DesktopModes::set(app,DesktopModes::Mode::Direct,error)&&bytes(app)==original,"readonly shortcut unchanged");SetFileAttributesW(app.c_str(),FILE_ATTRIBUTE_NORMAL);
 check(DesktopModes::set(app,DesktopModes::Mode::Direct,error),"direct again");link(app,std::wstring(sys)+L"\\cmd.exe",L"/c echo externally-edited");auto edited=bytes(app);check(!DesktopModes::repairBadge(app,error)&&bytes(app)==edited,"repair does not overwrite external shortcut edits");check(!DesktopModes::set(app,DesktopModes::Mode::Native,error)&&bytes(app)==edited,"external shortcut edit not overwritten on restore");

 for(const wchar_t* stage:{L"prepared",L"entry",L"applied",L"config"}){
  auto p=desk+L"\\Interrupt-"+stage+L".lnk";link(p,std::wstring(sys)+L"\\cmd.exe",L"/c echo interruption");auto originalCrash=bytes(p);
  check(child(baseDir,p,stage,2)==71,"injected process interruption");DesktopModes::load();check(DesktopModes::find(p)&&DesktopModes::find(p)->pending==2,"pending index survives interruption");
  check(DesktopModes::set(p,DesktopModes::Mode::Native,error)&&bytes(p)==originalCrash,"interrupted enable can restore original bytes");
  check(DesktopModes::set(p,DesktopModes::Mode::Direct,error),"prepare interrupted restore");
  check(child(baseDir,p,stage,0)==71,"restore interrupted at boundary");
  check(DesktopModes::set(p,DesktopModes::Mode::Native,error)&&bytes(p)==originalCrash&&!DesktopModes::find(p),"restore retry finishes without orphaned record");
 }
 auto crashFolder=desk+L"\\Interrupted folder";CreateDirectoryW(crashFolder.c_str(),nullptr);
 check(child(baseDir,crashFolder,L"entry",2)==71,"folder interrupted after badge");
 check(EntryMaintenance::reconcile().deferred>0&&GetFileAttributesW((crashFolder+L"\\desktop.ini").c_str())!=INVALID_FILE_ATTRIBUTES,"maintenance defers pending folder transaction");
 check(DesktopModes::set(crashFolder,DesktopModes::Mode::Native,error)&&GetFileAttributesW((crashFolder+L"\\desktop.ini").c_str())==INVALID_FILE_ATTRIBUTES,"interrupted folder restores");

 check(child(baseDir,crashFolder,L"entry",2)==71,"folder interruption for internal recovery phase");
 WIN32_FIND_DATAW fd{};auto pattern=baseDir+L"\\folder-badges\\*";HANDLE fs=FindFirstFileW(pattern.c_str(),&fd);bool patched=false;
 if(fs!=INVALID_HANDLE_VALUE){do{if(fd.cFileName[0]==L'.')continue;auto dir=baseDir+L"\\folder-badges\\"+fd.cFileName;auto pathBytes=bytes(dir+L"\\folder.path");std::wstring recorded((wchar_t*)pathBytes.data(),pathBytes.size()/2);if(recorded!=crashFolder)continue;auto state=bytes(dir+L"\\state.bin");if(state.size()==20){DWORD phase=1;memcpy(state.data()+16,&phase,4);FILE*f=_wfopen((dir+L"\\state.bin").c_str(),L"wb");if(f){fwrite(state.data(),1,state.size(),f);fclose(f);patched=true;}}}while(FindNextFileW(fs,&fd));FindClose(fs);}
 check(patched,"simulate internal badge phase one interruption");check(DesktopModes::set(crashFolder,DesktopModes::Mode::Native,error),"restore default recovers incomplete internal badge phase");
 std::wstring generated;check(EntryActions::createDesktopEntry(target,desk,generated,error)&&!generated.empty(),"generated entry fixture");auto genBytes=bytes(generated);
 check(DesktopModes::set(generated,DesktopModes::Mode::Direct,error),"generated entry supports direct");size_t updated=0;auto directBytes=bytes(generated);
 check(!EntryMaintenance::repairShortcutBadge(entries.size()-1,updated,error)&&bytes(generated)==directBytes,"legacy blue repair preserves mode bytes");
 std::wstring warning;check(!EntryActions::removeDesktopEntry(entries.size()-1,warning,error)&&bytes(generated)==directBytes,"legacy delete protects active mode recovery");
 check(DesktopModes::set(generated,DesktopModes::Mode::Native,error)&&bytes(generated)==genBytes,"generated original bytes still recoverable");

 {DesktopModes::Guard guard;check(guard.held,"parent owns mode transaction lock");check(child(baseDir,folder,L"locked",0)==73,"second process defers maintenance and refuses all competing config writes");}
 auto denied=desk+L"\\Denied folder";CreateDirectoryW(denied.c_str(),nullptr);put(denied+L"\\desktop.ini",L"[.ShellClassInfo]\r\nIconIndex=0\r\n");SetFileAttributesW((denied+L"\\desktop.ini").c_str(),FILE_ATTRIBUTE_READONLY);
 check(!DesktopModes::set(denied,DesktopModes::Mode::Direct,error),"readonly folder metadata blocks enabling");
 check(DesktopModes::set(denied,DesktopModes::Mode::Native,error)&&!DesktopModes::find(denied),"restore default cancels failed pending operation");


 auto racing=desk+L"\\Concurrent edit.lnk",external=baseDir+L"\\external.lnk";
 link(racing,std::wstring(sys)+L"\\cmd.exe",L"/c echo initial");link(external,std::wstring(sys)+L"\\cmd.exe",L"/c echo external-at-commit");auto externalBytes=bytes(external);
 raceSource=external;raceDestination=racing;DesktopModes::checkpoint=interrupted;
 check(!DesktopModes::set(racing,DesktopModes::Mode::Direct,error),"concurrent replacement is detected");DesktopModes::checkpoint=nullptr;
 check(bytes(racing)==externalBytes,"external commit restored without losing changed arguments");
 auto raceRecord=DesktopModes::find(racing);bool retained=false;
 if(raceRecord){WIN32_FIND_DATAW data{};HANDLE h=FindFirstFileW((raceRecord->backup+L"\\concurrent-*.lnk").c_str(),&data);if(h!=INVALID_HANDLE_VALUE){do{if(bytes(raceRecord->backup+L"\\"+data.cFileName)==externalBytes)retained=true;}while(FindNextFileW(h,&data));FindClose(h);}}
 check(retained,"external commit also retained in recovery directory");
 check(!DesktopModes::set(racing,DesktopModes::Mode::Native,error)&&bytes(racing)==externalBytes,"later restore does not overwrite concurrent external save");

 auto raceCrash=desk+L"\\Concurrent crash.lnk";link(raceCrash,std::wstring(sys)+L"\\cmd.exe",L"/c echo before-crash");
 check(child(baseDir,raceCrash,L"replace-applied",2)==71,"exit after atomic replacement before verification");
 check(!DesktopModes::set(raceCrash,DesktopModes::Mode::Direct,error)&&bytes(raceCrash)==externalBytes,"retry detects displaced external version and restores it");
 auto raceCancel=desk+L"\\Concurrent cancel.lnk";link(raceCancel,std::wstring(sys)+L"\\cmd.exe",L"/c echo before-cancel");
 check(child(baseDir,raceCancel,L"replace-applied",2)==71,"exit before verification then request default");
 check(!DesktopModes::set(raceCancel,DesktopModes::Mode::Native,error)&&bytes(raceCancel)==externalBytes,"default recovery checks displaced external save before replacing snapshots");
 // Simulate another writer committing while an older settings process is open.
 loadConfig();auto oldCfg=cfg;auto oldEntries=entries;auto oldRules=folderRules;
 auto newerTarget=baseDir+L"\\Newer target",newerRule=desk+L"\\Newer rule";
 CreateDirectoryW(newerTarget.c_str(),nullptr);CreateDirectoryW(newerRule.c_str(),nullptr);
 std::wstring newerLink;check(EntryActions::createDesktopEntry(newerTarget,desk,newerLink,error),"concurrent writer adds entry");
 auto newerId=entries.back().id;cfg.opacity=67;FolderRules::set(folderRules,newerRule,true,false);
 check(saveConfig(nullptr),"concurrent writer persists appearance rule and entry");
 cfg=oldCfg;entries=oldEntries;folderRules=oldRules;bool already=false;
 check(EntryActions::enableDesktopFolder(folder,already,warning,error),"stale enable reloads current configuration");
 loadConfig();check(cfg.opacity==67&&FolderRules::contains(folderRules,newerRule,false)&&std::any_of(entries.begin(),entries.end(),[&](const Entry&e){return e.id==newerId;}),"stale enable preserves newer appearance rule and entry");
 size_t selected=3;auto selectedId=entries[selected].id;
 oldCfg=cfg;oldEntries=entries;oldRules=folderRules;
 std::swap(entries[selected],entries.back());cfg.opacity=71;FolderRules::set(folderRules,newerRule,true,true);
 check(saveConfig(nullptr),"concurrent writer reorders entries and updates config");
 cfg=oldCfg;entries=oldEntries;folderRules=oldRules;
 check(EntryActions::removeDesktopEntry(selected,warning,error),"stale removal finds selected entry by stable ID");
 loadConfig();check(cfg.opacity==71&&FolderRules::contains(folderRules,newerRule,true)&&std::any_of(entries.begin(),entries.end(),[&](const Entry&e){return e.id==newerId;})&&!std::any_of(entries.begin(),entries.end(),[&](const Entry&e){return e.id==selectedId;}),"stale removal preserves other entry latest appearance and rule");
 check(GetFileAttributesW(newerLink.c_str())!=INVALID_FILE_ATTRIBUTES,"stale removal leaves newer shortcut intact");
 DirectClick click;click.press(L"a",10,10,900,500,4,4);click.release(L"a",10,10,1000,500,4,4);check(!click.due(1499)&&click.due(1500),"single waits for double click interval");click.reset();click.press(L"a",10,10,900,500,4,4);click.release(L"a",10,10,1000,500,4,4);click.press(L"a",10,10,1100,500,4,4);check(click.release(L"a",10,10,1200,500,4,4)==DirectClick::Result::NativeDouble&&!click.due(2000),"double leaves one native launch and cancels single");click.release(L"a",10,10,2200,500,4,4);click.reset();check(!click.due(3000),"cancel/drag cancels pending launch");
 click.reset();click.press(L"a",10,10,1000,500,4,4);click.release(L"a",10,10,1020,500,4,4);click.press(L"a",10,10,1300,500,4,4);check(click.release(L"a",10,10,1700,500,4,4)==DirectClick::Result::NativeDouble&&!click.due(2300),"second press held beyond double interval cannot launch again");
 click.reset();click.press(L"a",10,10,1000,500,4,4);click.release(L"a",10,10,1800,500,4,4);click.press(L"a",10,10,1900,500,4,4);check(click.release(L"a",10,10,1920,500,4,4)==DirectClick::Result::Wait&&click.due(2420),"first long press does not make a false double");
 click.reset();click.press(L"a",10,10,1000,500,4,4);click.release(L"a",10,10,1020,500,4,4);click.press(L"a",12,10,1300,500,4,4);check(click.release(L"a",12,10,1320,500,4,4)==DirectClick::Result::Wait,"double rectangle uses half-width and exclusive right edge");
 click.reset();click.press(L"a",10,10,1000,500,4,4);click.release(L"a",10,10,1020,500,4,4);click.press(L"b",10,10,1300,500,4,4);check(click.release(L"b",10,10,1320,500,4,4)==DirectClick::Result::Wait,"different item never treated as same double");
 click.reset();click.press(L"a",10,10,1000,500,4,4);click.release(L"a",11,10,1020,500,4,4);click.press(L"a",8,10,1300,500,4,4);check(click.release(L"a",8,10,1320,500,4,4)==DirectClick::Result::NativeDouble,"double rectangle stays centered at first DOWN despite UP drift");
 click.reset();click.press(L"a",10,10,0xffffff00,500,4,4);click.release(L"a",10,10,0xffffff20,500,4,4);click.press(L"a",10,10,0x40,500,4,4);check(click.release(L"a",10,10,0x60,500,4,4)==DirectClick::Result::NativeDouble,"input timestamp wrap remains a double");

 click.reset();click.begin(10,10,1000);check(!click.due(1800),"unresolved DOWN alone never launches");
 check(click.finish(L"late identity",10,10,1800,500,4,4)==DirectClick::Result::Wait&&click.due(2300),"identity first resolved at UP still opens exactly once");
 click.reset();click.begin(10,10,1000);click.finish(L"a",10,10,1020,500,4,4);click.begin(10,10,1300);
 check(!click.due(1900)&&click.finish(L"a",10,10,2000,500,4,4)==DirectClick::Result::NativeDouble,"deferred identity keeps original DOWN time for held double");
 click.reset();click.begin(10,10,1000);click.reset();check(click.finish(L"a",10,10,1100,500,4,4)==DirectClick::Result::NativeDouble&&!click.due(2000),"drag or right click cancellation cannot be revived by release");
 click.begin(10,10,1000);click.finish(L"a",10,10,1020,500,4,4);click.begin(10,10,1300);check(click.finish(L"b",10,10,1320,500,4,4)==DirectClick::Result::Wait,"deferred resolution distinguishes different desktop items");
 printf("PASS %d desktop mode checks\n",checks);Gdiplus::GdiplusShutdown(token);CoUninitialize();return 0;}
