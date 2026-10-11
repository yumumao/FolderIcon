#include "../src/app.h"
#include "../src/entry_actions.h"
#include "../src/folder_badge.h"
#include <cstdio>
#include <process.h>
static int checks=0;
static void check(bool ok,const char* message){printf("%s %s\n",ok?"PASS":"FAIL",message);fflush(stdout);if(!ok)ExitProcess(1);++checks;}
static void put(const std::wstring& p,const std::wstring& s){FILE*f=_wfopen(p.c_str(),L"wb");check(f!=nullptr,"fixture file");WORD bom=0xfeff;fwrite(&bom,2,1,f);fwrite(s.data(),2,s.size(),f);fclose(f);}
static std::vector<BYTE> bytes(const std::wstring&p){FILE*f=_wfopen(p.c_str(),L"rb");if(!f)return {};fseek(f,0,SEEK_END);auto n=ftell(f);rewind(f);std::vector<BYTE>b(n);fread(b.data(),1,n,f);fclose(f);return b;}
int wmain(int argc,wchar_t**argv){
 if(argc!=2&&argc!=3)return 2;CoInitialize(nullptr);ULONG_PTR token;Gdiplus::GdiplusStartupInput gi;Gdiplus::GdiplusStartup(&token,&gi,nullptr);
 baseDir=argv[1];CreateDirectoryW(baseDir.c_str(),nullptr);baseDir+=L"\\run-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());check(CreateDirectoryW(baseDir.c_str(),nullptr)!=FALSE,"fresh isolated fixture");iniPath=baseDir+L"\\FolderIcon.ini";exePath=baseDir+L"\\FolderIconCN.exe";folderRulesPath=baseDir+L"\\folder-rules.bin";
 put(iniPath,L"[General]\r\nResident=0\r\n[Appearance]\r\nFollowDesktop=0\r\nFollowDesktopFont=0\r\n");loadConfig();
 auto folder=baseDir+L"\\real-folder",desktop=baseDir+L"\\Desktop",store=baseDir+L"\\folder-badges";check(CreateDirectoryW(folder.c_str(),nullptr)!=FALSE,"source directory");check(CreateDirectoryW(desktop.c_str(),nullptr)!=FALSE,"synthetic desktop");
 auto ini=folder+L"\\desktop.ini";put(folder+L"\\keep.txt",L"payload must stay");auto payload=bytes(folder+L"\\keep.txt");DWORD attrs=GetFileAttributesW(folder.c_str());std::wstring error,warning;FolderRules::Rules none,on;FolderRules::set(on,folder,false,true);
 check(EntryActions::syncDesktopBadges(none,on).empty(),"apply desktop badge");auto applied=bytes(ini);
 check(EntryActions::syncDesktopBadges(on,on).empty()&&bytes(ini)==applied,"unchanged draft or cancel does not restore");
 check(EntryActions::syncDesktopBadges(on,none).empty(),"removal reports no errors");
 check(GetFileAttributesW(ini.c_str())==INVALID_FILE_ATTRIBUTES,"removing desktop rule restores original uncustomized appearance");
 check(GetFileAttributesW(folder.c_str())==attrs&&bytes(folder+L"\\keep.txt")==payload,"removing rule preserves folder attributes and contents");
 put(ini,L"[.ShellClassInfo]\r\nIconResource=private-icon.ico,0\r\n[ViewState]\r\nMode=preserve\r\n");auto original=bytes(ini);DWORD iniAttrs=GetFileAttributesW(ini.c_str());
 check(EntryActions::syncDesktopBadges(none,on).empty(),"apply over original private icon");
 auto panelOnly=on;FolderRules::set(panelOnly,folder,true,false);
 check(EntryActions::syncDesktopBadges(on,panelOnly).empty()&&bytes(ini)==original,"remove desktop role restores private icon even if panel expansion remains");
 check(GetFileAttributesW(ini.c_str())==iniAttrs,"private metadata attributes restored");
 check(EntryActions::syncDesktopBadges(none,on).empty(),"apply before external conflict");WritePrivateProfileStringW(L".ShellClassInfo",L"IconFile",L"new-private.ico",ini.c_str());auto changed=bytes(ini);
 check(!EntryActions::syncDesktopBadges(on,none).empty()&&bytes(ini)==changed,"later user icon wins; restoration reports failure without overwrite");
 auto plain=baseDir+L"\\shortcut-source";check(CreateDirectoryW(plain.c_str(),nullptr)!=FALSE,"plain source directory");put(plain+L"\\keep.txt",L"unchanged");auto plainAttrs=GetFileAttributesW(plain.c_str());std::wstring shortcut;
 check(EntryActions::createDesktopEntry(plain,desktop,shortcut,error),"create independent blue shortcut");
 check(DeleteFileW(shortcut.c_str())!=FALSE,"simulate direct desktop shortcut deletion");
 check(GetFileAttributesW((plain+L"\\desktop.ini").c_str())==INVALID_FILE_ATTRIBUTES&&GetFileAttributesW(plain.c_str())==plainAttrs,"direct shortcut deletion cannot leave a source badge because creation never writes one");
 // Simulate an older candidate that applied an orange badge to a shortcut source.
 check(FolderBadge::apply(store,plain,error),"legacy source badge fixture");auto legacy=bytes(plain+L"\\desktop.ini");
 SetFileAttributesW(iniPath.c_str(),FILE_ATTRIBUTE_READONLY);
 check(!EntryActions::removeDesktopEntry(3,warning,error)&&bytes(plain+L"\\desktop.ini")==legacy&&entries.size()==4,"failed config save cannot remove badge or entry");
 SetFileAttributesW(iniPath.c_str(),FILE_ATTRIBUTE_NORMAL);
 check(EntryActions::removeDesktopEntry(3,warning,error)&&warning.empty(),"remove entry after external shortcut deletion");
 check(GetFileAttributesW((plain+L"\\desktop.ini").c_str())==INVALID_FILE_ATTRIBUTES&&GetFileAttributesW(plain.c_str())==plainAttrs,"settings deletion restores legacy source badge after last entry is removed");
 // An unrelated live desktop role owns its own orange badge.
 check(EntryActions::createDesktopEntry(plain,desktop,shortcut,error),"create entry with another live desktop role");
 check(DeleteFileW(shortcut.c_str())!=FALSE,"remove only fixture link");
 FolderRules::set(folderRules,plain,false,true);check(saveConfig(nullptr),"persist independent desktop role");
 check(FolderBadge::apply(store,plain,error),"independent desktop badge");auto dual=bytes(plain+L"\\desktop.ini");
 check(EntryActions::removeDesktopEntry(3,warning,error)&&warning.empty()&&bytes(plain+L"\\desktop.ini")==dual,"deleting blue entry must not clear an active orange desktop role");
 FolderRules::set(folderRules,plain,false,false);check(saveConfig(nullptr),"remove independent desktop role");check(FolderBadge::restore(store,plain,error),"fixture restoration");
 // Two legacy references are handled by stable target ownership, not names.
 check(EntryActions::createDesktopEntry(plain,desktop,shortcut,error),"create first of duplicate legacy entries");check(DeleteFileW(shortcut.c_str())!=FALSE,"remove duplicate fixture link");
 auto duplicate=entries.back();duplicate.id=L"custom_legacyduplicate";duplicate.icon=L"private-not-owned.ico";entries.push_back(duplicate);check(saveConfig(nullptr),"persist legacy duplicate");check(FolderBadge::apply(store,plain,error),"badge before duplicate removal");auto duplicateBadge=bytes(plain+L"\\desktop.ini");
 check(EntryActions::removeDesktopEntry(3,warning,error)&&warning.empty()&&bytes(plain+L"\\desktop.ini")==duplicateBadge,"another live custom reference prevents premature cleanup");
 check(EntryActions::removeDesktopEntry(3,warning,error)&&warning.empty()&&GetFileAttributesW((plain+L"\\desktop.ini").c_str())==INVALID_FILE_ATTRIBUTES,"last custom reference restores legacy source appearance");
 // Restore failures are reported, not swallowed after config removal.
 check(EntryActions::createDesktopEntry(plain,desktop,shortcut,error),"create before protected restore");check(DeleteFileW(shortcut.c_str())!=FALSE,"remove protected fixture link");check(FolderBadge::apply(store,plain,error),"badge before read-only conflict");
 auto plainIni=plain+L"\\desktop.ini";auto protectedAttrs=GetFileAttributesW(plainIni.c_str());SetFileAttributesW(plainIni.c_str(),protectedAttrs|FILE_ATTRIBUTE_READONLY);auto protectedBytes=bytes(plainIni);
 check(EntryActions::removeDesktopEntry(3,warning,error)&&!warning.empty()&&bytes(plainIni)==protectedBytes,"failed restore is explicit and leaves metadata intact");
 SetFileAttributesW(plainIni.c_str(),protectedAttrs);check(FolderBadge::restore(store,plain,error),"failed restore remains retryable");
 if(argc==3){
  // Exercise the exact one-shot executable before it is used on named folders.
  check(FolderBadge::apply(store,plain,error),"helper restore fixture");auto helperBefore=bytes(plainIni);
  FolderRules::set(folderRules,plain,false,true);check(saveConfig(nullptr),"helper active-role fixture");
  auto runHelper=[&](const std::wstring& target){return _wspawnl(_P_WAIT,argv[2],argv[2],L"--restore",baseDir.c_str(),target.c_str(),static_cast<const wchar_t*>(nullptr));};
  check(runHelper(plain)==4&&bytes(plainIni)==helperBefore,"helper refuses active desktop rule without changing metadata");
  FolderRules::set(folderRules,plain,false,false);check(saveConfig(nullptr),"helper removed-role fixture");auto configBefore=bytes(iniPath);
  check(runHelper(plain)==0&&GetFileAttributesW(plainIni.c_str())==INVALID_FILE_ATTRIBUTES&&GetFileAttributesW(plain.c_str())==plainAttrs,"helper actually restores removed-role source");
  check(runHelper(plain)==0&&bytes(iniPath)==configBefore,"helper repeat is safe and leaves configuration unchanged");
  check(runHelper(folder)==5&&bytes(ini)==changed,"helper refuses later private icon conflict");
 }
 printf("badge removal: %d checks PASS\n",checks);Gdiplus::GdiplusShutdown(token);CoUninitialize();return 0;
}