#include "../src/app.h"
#include <cassert>
#include <cstdio>
static std::vector<BYTE> bytes(const std::wstring& path){FILE* f=_wfopen(path.c_str(),L"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);std::vector<BYTE> b(n);assert(fread(b.data(),1,n,f)==(size_t)n);fclose(f);return b;}
static void put(const std::wstring& path,const std::wstring& data){FILE* f=_wfopen(path.c_str(),L"wb");assert(f);WORD bom=0xfeff;fwrite(&bom,2,1,f);fwrite(data.data(),2,data.size(),f);fclose(f);}
int wmain(int argc,wchar_t** argv){
 assert(argc==2);CoInitialize(nullptr);baseDir=argv[1];CreateDirectoryW(baseDir.c_str(),nullptr);iniPath=baseDir+L"\\FolderIcon.ini";folderRulesPath=baseDir+L"\\folder-rules.bin";exePath=baseDir+L"\\FolderIconCN.exe";
 std::wstring keyPath=L"Software\\FolderIconCN.Test."+std::to_wstring(GetCurrentProcessId());HKEY key=nullptr;
 assert(RegCreateKeyExW(HKEY_CURRENT_USER,keyPath.c_str(),0,nullptr,0,KEY_ALL_ACCESS,nullptr,&key,nullptr)==ERROR_SUCCESS);
 // Startup preference migrates independently of the live listener state.
 put(iniPath,L"[General]\r\nResident=1\r\n");loadConfig();assert(cfg.autoStart==1);
 put(iniPath,L"[General]\r\nResident=1\r\nAutoStart=0\r\n");loadConfig();
 if(cfg.autoStart!=0){std::puts("FAIL AutoStart=0 overrides legacy Resident=1");return 1;}
 put(iniPath,L"[General]\r\nResident=0\r\nAutoStart=1\r\n");loadConfig();assert(cfg.autoStart==1);
 assert(saveConfig(keyPath.c_str()));assert(GetPrivateProfileIntW(L"General",L"AutoStart",-1,iniPath.c_str())==1);
 assert(GetPrivateProfileIntW(L"General",L"Resident",-1,iniPath.c_str())==1);
 wchar_t startup[32768];DWORD startupSize=sizeof(startup),startupType=0;
 assert(RegQueryValueExW(key,L"FolderIconCN",nullptr,&startupType,(BYTE*)startup,&startupSize)==ERROR_SUCCESS);
 assert(startupType==REG_SZ&&std::wstring(startup)==L"\""+exePath+L"\" --resident");
 cfg.autoStart=0;assert(saveConfig(keyPath.c_str()));
 assert(RegQueryValueExW(key,L"FolderIconCN",nullptr,nullptr,nullptr,nullptr)==ERROR_FILE_NOT_FOUND);
 assert(GetPrivateProfileIntW(L"General",L"AutoStart",-1,iniPath.c_str())==0);
 assert(GetPrivateProfileIntW(L"General",L"Resident",-1,iniPath.c_str())==0);
 put(iniPath,L"[General]\r\n");loadConfig();assert(cfg.autoStart==0);
 std::puts("PASS startup preference migration, precedence, exact login command, opt-out and fresh default");
 put(iniPath,L"[General]\r\nResident=0\r\nContextMenu=0\r\n[Appearance]\r\nFollowDesktop=0\r\nFollowDesktopFont=0\r\n[Unknown]\r\nKeep=中文 extra\r\n");
 loadConfig();assert(cfg.contextMenu==0);cfg.columns=7;cfg.contextMenu=1;folderRules.clear();assert(FolderRules::set(folderRules,baseDir+L"\\双名单",true,true));
 assert(saveConfig(keyPath.c_str()));loadConfig();assert(cfg.columns==7&&cfg.contextMenu==1&&folderRules.size()==1&&folderRules[0].panel&&folderRules[0].desktop);
 wchar_t keep[128];GetPrivateProfileStringW(L"Unknown",L"Keep",L"",keep,128,iniPath.c_str());assert(!wcscmp(keep,L"中文 extra"));
 entries.push_back({L"custom_test123",L"自选 中文 & folder",baseDir+L"\\自选 中文 & folder",baseDir+L"\\icons\\custom.ico"});
 assert(saveConfig(keyPath.c_str()));loadConfig();
 if(entries.size()!=4||entries.back().id!=L"custom_test123"||entries.back().name!=L"自选 中文 & folder"){std::puts("FAIL custom desktop entry survives config reload");return 1;}
 std::puts("PASS custom desktop entry survives config reload");
 auto original=bytes(iniPath);std::wstring oldRun=L"%LOCALAPPDATA%\\previous.exe --special";
 assert(RegSetValueExW(key,L"FolderIconCN",0,REG_EXPAND_SZ,(BYTE*)oldRun.c_str(),(DWORD)((oldRun.size()+1)*2))==ERROR_SUCCESS);
 HANDLE lock=CreateFileW(iniPath.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(lock!=INVALID_HANDLE_VALUE);
 cfg.autoStart=1;cfg.columns=9;folderRules.clear();assert(!saveConfig(keyPath.c_str()));CloseHandle(lock);assert(bytes(iniPath)==original);
 DWORD type=0,size=sizeof(keep);assert(RegQueryValueExW(key,L"FolderIconCN",nullptr,&type,(BYTE*)keep,&size)==ERROR_SUCCESS);assert(type==REG_EXPAND_SZ&&!wcscmp(keep,oldRun.c_str()));
 RegDeleteValueW(key,L"FolderIconCN");lock=CreateFileW(iniPath.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(lock!=INVALID_HANDLE_VALUE);assert(!saveConfig(keyPath.c_str()));CloseHandle(lock);assert(RegQueryValueExW(key,L"FolderIconCN",nullptr,nullptr,nullptr,nullptr)==ERROR_FILE_NOT_FOUND);assert(bytes(iniPath)==original);
 auto invalid=keyPath+L"\\"+std::wstring(300,L'x');assert(!saveConfig(invalid.c_str()));assert(bytes(iniPath)==original);
 assert(SetFileAttributesW(iniPath.c_str(),FILE_ATTRIBUTE_READONLY));assert(!saveConfig(keyPath.c_str()));SetFileAttributesW(iniPath.c_str(),FILE_ATTRIBUTE_NORMAL);assert(bytes(iniPath)==original);
 auto oldIni=iniPath;iniPath=baseDir+L"\\missing\\config.ini";assert(!saveConfig(keyPath.c_str()));iniPath=oldIni;
 cfg.autoStart=0;assert(saveConfig(keyPath.c_str()));loadConfig();assert(cfg.columns==9&&folderRules.empty());
 // Legacy local binary is read only until the first successful INI migration.
 put(iniPath,L"[Appearance]\r\nFollowDesktop=0\r\nFollowDesktopFont=0\r\n");FolderRules::Rules legacy;assert(FolderRules::set(legacy,baseDir,true,false));assert(FolderRules::save(folderRulesPath,legacy));loadConfig();assert(folderRules.size()==1);folderRules.clear();assert(saveConfig(keyPath.c_str()));loadConfig();assert(folderRules.empty());
 RegCloseKey(key);RegDeleteTreeW(HKEY_CURRENT_USER,keyPath.c_str());CoUninitialize();std::puts("config: single-file allowlists, unknown keys, locked/read-only/missing failures, exact startup rollback, legacy migration PASS");
}
