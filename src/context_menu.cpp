#include "context_menu.h"
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
namespace {
constexpr wchar_t kMenuKey[]=L"Software\\Classes\\Directory\\shell\\FolderIconCN";
constexpr wchar_t kLinkKey[]=L"Software\\Classes\\lnkfile\\shell\\FolderIconCN";
constexpr wchar_t kLegacyKey[]=L"Software\\Classes\\Directory\\shell\\AddAsFolderIcon";
std::wstring normalize(std::wstring path){
 if(path.rfind(L"\\\\?\\",0)==0)path=path.substr(4);
 wchar_t full[32768]={};DWORD n=GetFullPathNameW(path.c_str(),32768,full,nullptr);if(!n||n>=32768)return {};
 std::wstring result(full,n);for(auto& c:result)if(c==L'/')c=L'\\';
 while(result.size()>3&&result.back()==L'\\')result.pop_back();return result;
}
bool setValue(HKEY key,const wchar_t* name,const std::wstring& value){return RegSetValueExW(key,name,0,REG_SZ,(const BYTE*)value.c_str(),(DWORD)((value.size()+1)*sizeof(wchar_t)))==ERROR_SUCCESS;}
bool removeTree(const std::wstring& key,std::wstring& error){LONG e=RegDeleteTreeW(HKEY_CURRENT_USER,key.c_str());if(e==ERROR_SUCCESS||e==ERROR_FILE_NOT_FOUND||e==ERROR_PATH_NOT_FOUND)return true;error=L"无法更新文件夹右键菜单，请检查当前用户注册表权限。";return false;}
bool writeCommand(HKEY menu,const std::wstring& executable){
 HKEY command=nullptr;LONG e=RegCreateKeyExW(menu,L"command",0,nullptr,0,KEY_SET_VALUE,nullptr,&command,nullptr);
 if(e!=ERROR_SUCCESS)return false;
 bool ok=setValue(command,nullptr,ContextMenu::commandLine(executable,L"choose"));RegCloseKey(command);return ok;
}
}
namespace ContextMenu {
const wchar_t* registrySubKey(){return kMenuKey;}
std::wstring commandLine(const std::wstring& executable){return commandLine(executable,L"choose");}
std::wstring commandLine(const std::wstring& executable,const std::wstring& action){return L"\""+executable+L"\" --context-folder "+action+L" \"%1\"";}
bool isDirectDesktopChild(const std::wstring& folder,const std::wstring& desktop){
 auto f=normalize(folder),d=normalize(desktop);if(f.empty()||d.empty()||f.size()<=d.size()+1)return false;
 if(_wcsnicmp(f.c_str(),d.c_str(),d.size())||f[d.size()]!=L'\\')return false;
 return f.find(L'\\',d.size()+1)==std::wstring::npos;
}
bool sync(bool enabled,const std::wstring& executable,std::wstring& error){
 error.clear();
 for(const wchar_t* currentKey:{kMenuKey,kLinkKey}){
 if(enabled){
  std::wstring ignored;if(!removeTree(currentKey,ignored)){error=ignored;return false;}
  HKEY menu=nullptr;LONG e=RegCreateKeyExW(HKEY_CURRENT_USER,currentKey,0,nullptr,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY,nullptr,&menu,nullptr);
  if(e!=ERROR_SUCCESS){error=L"无法添加文件夹右键菜单，请检查当前用户注册表权限。";return false;}
  bool ok=setValue(menu,L"MUIVerb",L"FolderIcon打开方式…")&&setValue(menu,L"Icon",executable)&&writeCommand(menu,executable);
  RegCloseKey(menu);
  if(!ok){std::wstring ignored2;removeTree(currentKey,ignored2);error=L"无法写入文件夹右键菜单，设置未完成。";return false;}
 }else if(!removeTree(currentKey,error))return false;
 }
 // Remove only the original FolderIcon legacy key created by main.c; do not scan or touch other applications.
 if(!removeTree(kLegacyKey,error)){
  if(enabled){std::wstring ignored;removeTree(kMenuKey,ignored);removeTree(kLinkKey,ignored);}
  return false;
 }
 SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST|SHCNF_FLUSHNOWAIT,nullptr,nullptr);return true;
}
}
