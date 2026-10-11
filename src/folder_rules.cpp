#include "folder_rules.h"
#include <windows.h>
#include <algorithm>
#include <cwctype>
namespace FolderRules {
static bool absolutePath(const std::wstring& p){
 if(p.rfind(L"\\\\?\\",0)==0)return p.size()>=7&&iswalpha(p[4])&&p[5]==L':'&&(p[6]==L'\\'||p[6]==L'/');
 return (p.size()>=3&&iswalpha(p[0])&&p[1]==L':'&&(p[2]==L'\\'||p[2]==L'/'))||p.rfind(L"\\\\",0)==0;
}
std::wstring normalize(const std::wstring& path){
 if(path.empty())return {};
 std::wstring p=path;if(p.rfind(L"\\\\?\\",0)==0)p=p.substr(4);
 wchar_t full[32768]={};DWORD n=GetFullPathNameW(p.c_str(),32768,full,nullptr);if(!n||n>=32768)return {};
 std::wstring out(full,n);for(auto& c:out)if(c==L'/')c=L'\\';
 while(out.size()>3&&out.back()==L'\\')out.pop_back();
 return out;
}
bool contains(const Rules& rules,const std::wstring& path,bool desktop){auto n=normalize(path);if(n.empty())return false;for(const auto& r:rules)if((desktop?r.desktop:r.panel)&&_wcsicmp(r.path.c_str(),n.c_str())==0)return true;return false;}
bool set(Rules& rules,const std::wstring& path,bool panel,bool desktop){
 if(!absolutePath(path))return false;auto n=normalize(path);if(n.empty()||n.size()<3)return false;
 for(auto i=rules.begin();i!=rules.end();++i)if(!_wcsicmp(i->path.c_str(),n.c_str())){ if(!panel&&!desktop)rules.erase(i); else {i->path=n;i->panel=panel;i->desktop=desktop;} return true; }
 if(!panel&&!desktop)return true;rules.push_back({n,panel,desktop});return true;
}
static bool writeAll(HANDLE h,const void* p,DWORD bytes){DWORD got=0;return WriteFile(h,p,bytes,&got,nullptr)&&got==bytes;}
static bool readAll(HANDLE h,void* p,DWORD bytes){DWORD got=0;return ReadFile(h,p,bytes,&got,nullptr)&&got==bytes;}
bool save(const std::wstring& file,const Rules& rules){
 std::wstring tmp=file+L".tmp";HANDLE h=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_HIDDEN,nullptr);if(h==INVALID_HANDLE_VALUE)return false;
 const DWORD magic=0x314C5246,version=1,count=(DWORD)rules.size();bool ok=writeAll(h,&magic,4)&&writeAll(h,&version,4)&&writeAll(h,&count,4);
 for(const auto& r:rules){DWORD chars=(DWORD)r.path.size(),flags=(r.panel?1u:0u)|(r.desktop?2u:0u);ok=ok&&writeAll(h,&chars,4)&&writeAll(h,&flags,4)&&writeAll(h,r.path.data(),chars*sizeof(wchar_t));}
 FlushFileBuffers(h);CloseHandle(h);if(!ok){DeleteFileW(tmp.c_str());return false;}
 if(!MoveFileExW(tmp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(tmp.c_str());return false;}return true;
}
bool load(const std::wstring& file,Rules& out){
 out.clear();HANDLE h=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(h==INVALID_HANDLE_VALUE)return true;
 DWORD magic=0,version=0,count=0;if(!readAll(h,&magic,4)||!readAll(h,&version,4)||!readAll(h,&count,4)||magic!=0x314C5246||version!=1||count>4096){CloseHandle(h);return false;}
 Rules temp;for(DWORD i=0;i<count;i++){DWORD chars=0,flags=0;if(!readAll(h,&chars,4)||!readAll(h,&flags,4)||chars>32767){CloseHandle(h);return false;}std::wstring p(chars,L'\0');if(chars&&!readAll(h,p.data(),chars*sizeof(wchar_t))){CloseHandle(h);return false;}if(!set(temp,p,(flags&1)!=0,(flags&2)!=0)){CloseHandle(h);return false;}}
 CloseHandle(h);out.swap(temp);return true;
}
std::wstring identity(const std::wstring& path){auto n=normalize(path);if(n.empty())return {};HANDLE h=CreateFileW(n.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);if(h==INVALID_HANDLE_VALUE)return {};BY_HANDLE_FILE_INFORMATION i{};std::wstring result;if(GetFileInformationByHandle(h,&i))result=std::to_wstring(i.dwVolumeSerialNumber)+L":"+std::to_wstring(i.nFileIndexHigh)+L":"+std::to_wstring(i.nFileIndexLow);CloseHandle(h);return result;}
bool canEnter(const std::vector<std::wstring>& ancestors,const std::wstring& next){if(next.empty()||ancestors.size()>=32)return false;for(const auto& a:ancestors)if(!_wcsicmp(a.c_str(),next.c_str()))return false;return true;}
}
