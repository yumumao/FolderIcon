#pragma once
#include "app.h"
namespace DesktopModes {
enum class Mode { Native=0, Panel=1, Direct=2 };
struct Record {std::wstring path,backup;Mode mode=Mode::Native;bool folder=false;int pending=-1;std::wstring staged,applied;};
// All mode writers and background cleanup share this transaction mutex.
struct Guard {
 HANDLE h=CreateMutexW(nullptr,FALSE,L"Local\\FolderIconCN.DesktopModes");bool held=false;
 Guard(){if(h){DWORD w=WaitForSingleObject(h,0);held=w==WAIT_OBJECT_0||w==WAIT_ABANDONED;}}
 ~Guard(){if(held)ReleaseMutex(h);if(h)CloseHandle(h);}
 Guard(const Guard&)=delete;Guard&operator=(const Guard&)=delete;
};
inline std::vector<Record> records;
inline bool validStore=true;
inline bool samePath(const std::wstring&a,const std::wstring&b){return !_wcsicmp(FolderRules::normalize(a).c_str(),FolderRules::normalize(b).c_str());}
inline const Record* find(const std::wstring&path){for(const auto&r:records)if(samePath(path,r.path))return &r;return nullptr;}
inline std::wstring storePath(){return baseDir+L"\\desktop-modes.ini";}
inline void load(){
 records.clear();validStore=true;auto file=storePath();if(GetFileAttributesW(file.c_str())==INVALID_FILE_ATTRIBUTES)return;
 if(GetPrivateProfileIntW(L"Modes",L"Format",0,file.c_str())!=1){validStore=false;return;}
 int n=GetPrivateProfileIntW(L"Modes",L"Count",-1,file.c_str());if(n<0||n>4096){validStore=false;return;}
 for(int i=0;i<n;++i){auto section=L"Item"+std::to_wstring(i);wchar_t p[32768]{},b[32768]{};
  GetPrivateProfileStringW(section.c_str(),L"Path",L"",p,32768,file.c_str());GetPrivateProfileStringW(section.c_str(),L"Backup",L"",b,32768,file.c_str());
  int m=GetPrivateProfileIntW(section.c_str(),L"Mode",-1,file.c_str()),f=GetPrivateProfileIntW(section.c_str(),L"Folder",-1,file.c_str());
  if(!p[0]||PathIsRelativeW(p)||m<0||m>2||f<0||f>1||find(p)){records.clear();validStore=false;return;}
  Record r{p,b,(Mode)m,f!=0};
  r.pending=GetPrivateProfileIntW(section.c_str(),L"Pending",-1,file.c_str());
  wchar_t staged[32768]{},applied[32768]{};
  GetPrivateProfileStringW(section.c_str(),L"Staged",L"",staged,32768,file.c_str());
  GetPrivateProfileStringW(section.c_str(),L"Applied",L"",applied,32768,file.c_str());
  r.staged=staged;r.applied=applied;
  if(r.pending < -1||r.pending>2||(!r.folder&&r.pending>=0&&r.staged.empty())){records.clear();validStore=false;return;}
  records.push_back(r);
 }
}
inline bool pending(){return std::any_of(records.begin(),records.end(),[](const Record&r){return r.pending>=0;});}
inline bool direct(const std::wstring&path){auto r=find(path);return r&&r->mode==Mode::Direct;}
struct Info {bool supported=false,canPanel=false,folder=false,managedEntry=false;Mode mode=Mode::Native;std::wstring path,target,error;};
Info inspect(const std::wstring& path);
#ifdef FOLDERICON_MODE_TEST
inline void (*checkpoint)(const wchar_t*)=nullptr;
#endif
bool set(const std::wstring&path,Mode mode,std::wstring&error);
bool repairBadge(const std::wstring&path,std::wstring&error);
bool showDialog(HWND owner,const std::wstring&path);
}
