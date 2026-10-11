#pragma once
#include "app.h"
namespace DesktopScope {
inline bool realFolder(const std::wstring& path){DWORD a=GetFileAttributesW(path.c_str());return a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_DIRECTORY)&&!(a&FILE_ATTRIBUTE_REPARSE_POINT);}
inline bool systemPath(std::wstring& path){PWSTR p=nullptr;auto hr=SHGetKnownFolderPath(FOLDERID_Desktop,0,nullptr,&p);path=SUCCEEDED(hr)?FolderRules::normalize(p):L"";CoTaskMemFree(p);return !path.empty();}
inline bool resolve(const std::wstring& configured,std::wstring& path,std::wstring& error){
 error.clear();path.clear();
 if(configured.empty()){if(!systemPath(path)){error=L"无法取得系统桌面位置。请在设置中指定桌面位置。";return false;}}
 else {if(PathIsRelativeW(configured.c_str())){error=L"桌面位置必须是绝对路径。";return false;}path=FolderRules::normalize(configured);}
 if(path.empty()||!realFolder(path)){error=L"桌面位置不存在、不可访问或不是真实目录：\n"+path+L"\n请在设置中重新指定，不会自动改用其它位置。";return false;}return true;
}
inline bool directChild(const std::wstring& path,const std::wstring& root){
 auto p=FolderRules::normalize(path),r=FolderRules::normalize(root);if(p.empty()||r.empty())return false;
 if(r.back()!=L'\\')r+=L'\\';
 return p.size()>r.size()&&!_wcsnicmp(p.c_str(),r.c_str(),r.size())&&p.find(L'\\',r.size())==std::wstring::npos;
}
inline bool validate(const std::wstring& folder,const std::wstring& configured,std::wstring& error){
 std::wstring root;if(!resolve(configured,root,error))return false;
 if(!realFolder(folder)){error=L"请选择可访问的真实文件夹，不支持文件、目录链接或重解析点。";return false;}
 if(!directChild(folder,root)){error=L"这里只能添加当前桌面的直接子文件夹：\n"+root+L"\n\n不能选择桌面本身、下级目录或外部目录。其它位置请使用左侧的创建桌面展开快捷方式。";return false;}return true;
}
// Operates on a draft only. Save/cancel and badge restoration belong to the caller.
inline size_t prune(FolderRules::Rules& rules,const std::wstring& root){
 auto before=rules;size_t count=0;for(const auto& r:before)if(r.desktop&&!directChild(r.path,root)){FolderRules::set(rules,r.path,r.panel,false);++count;}return count;
}
inline void rememberEntryLocations(std::vector<Entry>& entries,const std::wstring& oldRoot){
 for(size_t i=3;i<entries.size();++i)if(entries[i].desktop.empty())entries[i].desktop=oldRoot;
}
}
