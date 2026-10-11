#include "app.h"
#include <cstdio>
#include <set>
std::wstring configSaveError;
void loadFolderRules(){
 folderRules.clear();wchar_t format[8]={};
 GetPrivateProfileStringW(L"FolderRules",L"Format",L"",format,8,iniPath.c_str());
 if(!format[0]){FolderRules::load(folderRulesPath,folderRules);return;}
 if(wcscmp(format,L"1"))return;
 int count=GetPrivateProfileIntW(L"FolderRules",L"Count",-1,iniPath.c_str());
 if(count<0||count>4096)return;
 FolderRules::Rules parsed;
 for(int i=0;i<count;++i){wchar_t path[32768]={};auto suffix=std::to_wstring(i);
  GetPrivateProfileStringW(L"FolderRules",(L"Path"+suffix).c_str(),L"",path,32768,iniPath.c_str());
  int flags=GetPrivateProfileIntW(L"FolderRules",(L"Flags"+suffix).c_str(),0,iniPath.c_str());
  if(flags<1||flags>3||!FolderRules::set(parsed,path,(flags&1)!=0,(flags&2)!=0))return;
 }
 folderRules.swap(parsed);
}
static bool writeConfigTo(const std::wstring& file){
 bool ok=true;
 auto set=[&](const wchar_t* section,const wchar_t* key,const std::wstring& value){ok=WritePrivateProfileStringW(section,key,value.c_str(),file.c_str())&&ok;};
 set(L"General",L"DesktopPath",cfg.desktopPath);
 // AutoStart is a login preference, never permission to run the listener.
 // Mirror the legacy key for older binaries; the new key takes precedence.
 set(L"General",L"AutoStart",std::to_wstring(cfg.autoStart));
 set(L"General",L"Resident",std::to_wstring(cfg.autoStart));set(L"General",L"ContextMenu",std::to_wstring(cfg.contextMenu));
 for(auto p:{std::pair<const wchar_t*,int>{L"IconSize",cfg.icon},{L"CellWidth",cfg.cellW},{L"CellHeight",cfg.cellH},{L"Columns",cfg.columns},{L"Opacity",cfg.opacity},{L"Radius",cfg.radius},{L"Blur",cfg.blur},{L"FollowDesktop",cfg.followDesktop},{L"FollowDesktopFont",cfg.followFont},{L"FontSize",cfg.fontSize},{L"TextGap",cfg.textGap},{L"ExpandDelay",cfg.expandDelay},{L"DotNamesHidden",cfg.dotHidden},{L"ShowHidden",cfg.showHidden}})set(L"Appearance",p.first,std::to_wstring(p.second));
 wchar_t color[8];swprintf(color,8,L"%02X%02X%02X",GetRValue(cfg.background),GetGValue(cfg.background),GetBValue(cfg.background));set(L"Appearance",L"Background",color);
int previousCount=GetPrivateProfileIntW(L"CustomEntries",L"Count",0,file.c_str());std::vector<std::wstring> previousIds;for(int i=0;i<previousCount&&i<=1024;++i){wchar_t id[80]{};GetPrivateProfileStringW(L"CustomEntries",(L"Id"+std::to_wstring(i)).c_str(),L"",id,80,file.c_str());if(id[0])previousIds.emplace_back(id);}
int currentCount=entries.size()>3?(int)entries.size()-3:0;set(L"CustomEntries",L"Count",std::to_wstring(currentCount));
for(size_t i=0;i<entries.size();++i){const auto&e=entries[i];set(e.id.c_str(),L"Target",e.target);
  if(i>=3){set(L"CustomEntries",(L"Id"+std::to_wstring(i-3)).c_str(),e.id);set(e.id.c_str(),L"Name",e.name);set(e.id.c_str(),L"Icon",e.icon);set(e.id.c_str(),L"Desktop",e.desktop);}
}
for(const auto& oldId:previousIds){bool kept=std::any_of(entries.begin()+std::min<size_t>(3,entries.size()),entries.end(),[&](const Entry&e){return !_wcsicmp(e.id.c_str(),oldId.c_str());});if(!kept)WritePrivateProfileStringW(oldId.c_str(),nullptr,nullptr,file.c_str());}
for(int i=currentCount;i<previousCount;++i)WritePrivateProfileStringW(L"CustomEntries",(L"Id"+std::to_wstring(i)).c_str(),nullptr,file.c_str());
 // Settings and both allowlists commit in one atomic file replacement.
 ok=WritePrivateProfileStringW(L"FolderRules",nullptr,nullptr,file.c_str())&&ok;
 set(L"FolderRules",L"Format",L"1");set(L"FolderRules",L"Count",std::to_wstring(folderRules.size()));
 for(size_t i=0;i<folderRules.size();++i){const auto& rule=folderRules[i];auto n=std::to_wstring(i);
  set(L"FolderRules",(L"Path"+n).c_str(),rule.path);set(L"FolderRules",(L"Flags"+n).c_str(),std::to_wstring((rule.panel?1:0)|(rule.desktop?2:0)));
 }
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,file.c_str());return ok;
}
bool saveConfig(const wchar_t* runSubkey){
 configSaveError=L"配置无法写入，请检查目录权限。";
 const auto temp=iniPath+L".pending."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64());
 HANDLE f=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(f==INVALID_HANDLE_VALUE)return false;
 const WORD bom=0xfeff;DWORD written=0;bool ok=WriteFile(f,&bom,sizeof(bom),&written,nullptr)&&written==sizeof(bom);CloseHandle(f);
 auto cleanup=[&](){SetFileAttributesW(temp.c_str(),FILE_ATTRIBUTE_NORMAL);DeleteFileW(temp.c_str());};
 DWORD attrs=GetFileAttributesW(iniPath.c_str());
 if(attrs!=INVALID_FILE_ATTRIBUTES){if(attrs&FILE_ATTRIBUTE_READONLY){cleanup();return false;}ok=ok&&CopyFileW(iniPath.c_str(),temp.c_str(),FALSE);}
 if(!ok||!writeConfigTo(temp)){cleanup();return false;}
 // Rule/menu and shortcut actions do not change login-startup settings.
 if(!runSubkey){ok=MoveFileExW(temp.c_str(),iniPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;if(!ok)cleanup();else configSaveError.clear();return ok;}
 // Snapshot only our own value, including its exact type and bytes, before changing it.
 HKEY key=nullptr;LONG error=RegCreateKeyExW(HKEY_CURRENT_USER,runSubkey,0,nullptr,0,KEY_QUERY_VALUE|KEY_SET_VALUE,nullptr,&key,nullptr);
 if(error!=ERROR_SUCCESS){configSaveError=L"无法更新本工具的启动项，配置未应用。";cleanup();return false;}
 DWORD type=0,size=0;error=RegQueryValueExW(key,L"FolderIconCN",nullptr,&type,nullptr,&size);
 bool existed=error==ERROR_SUCCESS;std::vector<BYTE> before;
 if((error!=ERROR_SUCCESS&&error!=ERROR_FILE_NOT_FOUND)||size>1024*1024){RegCloseKey(key);cleanup();return false;}
 if(existed){before.resize(size);error=RegQueryValueExW(key,L"FolderIconCN",nullptr,&type,before.data(),&size);if(error!=ERROR_SUCCESS){RegCloseKey(key);cleanup();return false;}before.resize(size);}
 auto value=L"\""+exePath+L"\" --resident";
 if(cfg.autoStart)error=RegSetValueExW(key,L"FolderIconCN",0,REG_SZ,(const BYTE*)value.c_str(),(DWORD)((value.size()+1)*sizeof(wchar_t)));
 else{error=RegDeleteValueW(key,L"FolderIconCN");if(error==ERROR_FILE_NOT_FOUND)error=ERROR_SUCCESS;}
 if(error!=ERROR_SUCCESS){configSaveError=L"无法更新本工具的启动项，配置未应用。";RegCloseKey(key);cleanup();return false;}
 ok=MoveFileExW(temp.c_str(),iniPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
 if(!ok){LONG rollback=existed?RegSetValueExW(key,L"FolderIconCN",0,type,before.data(),(DWORD)before.size()):RegDeleteValueW(key,L"FolderIconCN");
  if(rollback!=ERROR_SUCCESS&&rollback!=ERROR_FILE_NOT_FOUND)configSaveError=L"保存失败且启动项恢复失败，请检查本工具的登录启动项。";
 }
 RegCloseKey(key);if(!ok)cleanup();else configSaveError.clear();return ok;
}
