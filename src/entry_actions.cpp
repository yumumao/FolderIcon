#include "entry_actions.h"
#include "desktop_scope.h"
#include "context_menu.h"
#include "folder_badge.h"
#include "desktop_modes.h"
namespace EntryActions {
bool realFolder(const std::wstring& path){return DesktopScope::realFolder(path);}
bool setPanelExpansion(const std::wstring& path,bool enabled,std::wstring& error){
 DesktopModes::Guard modeGuard;if(!modeGuard.held){error=L"打开方式正在保存，请稍后重试。";return false;}loadConfig();
 error.clear();if(!realFolder(path)){error=L"目标不是真实文件夹或已不可访问。";return false;}
 auto before=folderRules;
 if(!FolderRules::set(folderRules,path,enabled,FolderRules::contains(folderRules,path,true))){error=L"无法记录这个文件夹。";return false;}
 if(!saveConfig(nullptr)){folderRules=std::move(before);error=configSaveError;return false;}return true;
}
bool enableDesktopFolder(const std::wstring& path,bool& alreadyEnabled,std::wstring& warning,std::wstring& error){
 DesktopModes::Guard modeGuard;if(!modeGuard.held){error=L"打开方式正在保存，请稍后重试。";return false;}loadConfig();alreadyEnabled=false;warning.clear();error.clear();if(!realFolder(path)){error=L"目标不是真实文件夹或已不可访问。";return false;}
 if(!DesktopScope::validate(path,cfg.desktopPath,error))return false;
 if(DesktopModes::direct(path)){error=L"这个目录已设为单击直接打开，请从右键打开方式切换为面板。";return false;}
 auto normalized=FolderRules::normalize(path);
 alreadyEnabled=FolderRules::contains(folderRules,normalized,true);auto before=folderRules;bool panel=FolderRules::contains(folderRules,normalized,false);
 if(!FolderRules::set(folderRules,normalized,panel,true)){error=L"无法记录这个文件夹。";return false;}
 if(!alreadyEnabled&&!saveConfig(nullptr)){folderRules=std::move(before);error=configSaveError;return false;}
 std::wstring badgeError;if(!FolderBadge::apply(baseDir+L"\\folder-badges",normalized,badgeError))warning=L"规则已添加，但橙色角标未能写入：\n"+badgeError;
 return true;
}FolderRules::Rules mergeDesktopRules(const FolderRules::Rules& live,const FolderRules::Rules& draft){
 // Only the desktop flag belongs to the settings draft. Panel right-click edits are live.
 auto out=live;
 for(const auto&r:live)FolderRules::set(out,r.path,r.panel,FolderRules::contains(draft,r.path,true));
 for(const auto&r:draft)if(r.desktop)FolderRules::set(out,r.path,FolderRules::contains(out,r.path,false),true);
 return out;
}
std::wstring syncDesktopBadges(const FolderRules::Rules& before,const FolderRules::Rules& after){
 DesktopModes::Guard modeGuard;if(!modeGuard.held)return L"打开方式正在保存，角标稍后重试。";DesktopModes::load();std::wstring failures;for(const auto&r:after)if(r.desktop&&!FolderRules::contains(before,r.path,true)&&!DesktopModes::direct(r.path)){
  std::wstring error;if(!FolderBadge::apply(baseDir+L"\\folder-badges",r.path,error))failures+=r.path+L"\n"+error+L"\n\n";
 }
 // A panel-only rule draws a transient badge in our panel, not on the real directory.
 // Only committed desktop-role changes reach this function; cancelling a draft is inert.
 for(const auto&r:before)if(r.desktop&&!FolderRules::contains(after,r.path,true)&&!DesktopModes::direct(r.path)){
  std::wstring error;if(!FolderBadge::restore(baseDir+L"\\folder-badges",r.path,error))failures+=r.path+L"\n"+error+L"\n\n";
 }return failures;
}
static bool fail(std::wstring& error,const wchar_t* text){error=text;return false;}
static bool hasCustomTarget(const std::wstring& path){
 auto normalized=FolderRules::normalize(path);if(normalized.empty())return false;
 for(size_t i=3;i<entries.size();++i){auto target=FolderRules::normalize(entries[i].target);if(!target.empty()&&!_wcsicmp(target.c_str(),normalized.c_str()))return true;}
 return false;
}
bool createDesktopEntry(const std::wstring& folder,const std::wstring& desktop,std::wstring& shortcut,std::wstring& error){
 DesktopModes::Guard modeGuard;if(!modeGuard.held){error=L"打开方式正在保存，请稍后重试。";return false;}loadConfig();
 error.clear();shortcut.clear();auto path=FolderRules::normalize(folder);
 DWORD attr=GetFileAttributesW(path.c_str()),da=GetFileAttributesW(desktop.c_str());
 if(attr==INVALID_FILE_ATTRIBUTES||!(attr&FILE_ATTRIBUTE_DIRECTORY))return fail(error,L"请选择可访问的文件夹。");
 if(da==INVALID_FILE_ATTRIBUTES||!(da&FILE_ATTRIBUTE_DIRECTORY))return fail(error,L"桌面位置不可访问，未创建入口。");
 if(hasCustomTarget(path))return true;
 if(entries.size()>=1027)return fail(error,L"已达到1024个自选入口上限。");
 GUID guid{};if(FAILED(CoCreateGuid(&guid)))return fail(error,L"无法生成入口标识。");wchar_t text[40]{};StringFromGUID2(guid,text,40);std::wstring id=L"custom_";for(wchar_t c:std::wstring(text))if(iswalnum(c))id+=c;
 std::wstring name=PathFindFileNameW(path.c_str());if(name.empty())name=path;
 for(auto&c:name)if(c<32||wcschr(L"<>:\"/\\|?*",c))c=L'_';while(!name.empty()&&(name.back()==L'.'||name.back()==L' '))name.pop_back();if(name.empty())name=L"文件夹面板";
 auto iconDir=baseDir+L"\\icons\\entries";int made=SHCreateDirectoryExW(nullptr,iconDir.c_str(),nullptr);if(made!=ERROR_SUCCESS&&made!=ERROR_ALREADY_EXISTS)return fail(error,L"无法创建程序旁的入口图标目录。");
 auto icon=iconDir+L"\\"+id+L".ico",temp=iconDir+L"\\"+id+L".pending.lnk";
 if(!FolderBadge::shortcutIcon(path,icon,error))return false;
 IShellLinkW* link=nullptr;IPersistFile* file=nullptr;HRESULT hr=CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link));
 if(SUCCEEDED(hr))hr=link->SetPath(exePath.c_str());if(SUCCEEDED(hr))hr=link->SetArguments((L"--entry "+id).c_str());if(SUCCEEDED(hr))hr=link->SetWorkingDirectory(baseDir.c_str());if(SUCCEEDED(hr))hr=link->SetIconLocation(icon.c_str(),0);if(SUCCEEDED(hr))hr=link->SetDescription((L"文件夹面板："+path).c_str());if(SUCCEEDED(hr))hr=link->SetShowCmd(SW_SHOWNORMAL);if(SUCCEEDED(hr))hr=link->QueryInterface(IID_PPV_ARGS(&file));if(SUCCEEDED(hr))hr=file->Save(temp.c_str(),TRUE);if(file)file->Release();if(link)link->Release();
 auto cleanup=[&](){DeleteFileW(temp.c_str());DeleteFileW(icon.c_str());};
 if(FAILED(hr)){cleanup();return fail(error,L"无法生成桌面快捷方式，原文件夹未修改。");}
 std::wstring dest;for(int n=1;n<=9999;++n){dest=desktop+L"\\"+name+(n==1?L"":L" ("+std::to_wstring(n)+L")")+L".lnk";
  // No replacement flag: even a concurrent same-name file is never overwritten.
  if(MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_WRITE_THROUGH))break;
  auto e=GetLastError();dest.clear();if(e!=ERROR_ALREADY_EXISTS&&e!=ERROR_FILE_EXISTS)break;
 }
 if(dest.empty()){cleanup();return fail(error,L"桌面入口无法写入；未覆盖已有同名项目。");}
 entries.push_back({id,name,path,icon,FolderRules::normalize(desktop)});
 if(!saveConfig(nullptr)){entries.pop_back();bool removed=DeleteFileW(dest.c_str())!=FALSE;if(removed)cleanup();error=configSaveError;if(!removed)error+=L"\n配置未保存，刚生成的快捷方式未能清理，请手动移除："+dest;return false;}
 shortcut=dest;SHChangeNotify(SHCNE_CREATE,SHCNF_PATHW|SHCNF_FLUSHNOWAIT,dest.c_str(),nullptr);return true;
}
static bool samePath(const std::wstring& a,const std::wstring& b){auto x=FolderRules::normalize(a),y=FolderRules::normalize(b);return !x.empty()&&!y.empty()&&!_wcsicmp(x.c_str(),y.c_str());}
static bool removeGeneratedShortcut(const Entry& entry,std::wstring& warning){
 // Only known single-level locations, never a disk scan. Match EXE and exact ID,
 // not the display name; renamed shortcuts remain removable after a root switch.
 std::vector<std::wstring> roots;
 auto add=[&](const std::wstring& path){auto p=FolderRules::normalize(path);if(p.empty()||PathIsRelativeW(path.c_str()))return;for(const auto& r:roots)if(samePath(r,p))return;roots.push_back(p);};
 add(entry.desktop);std::wstring current,error,system;
 if(DesktopScope::resolve(cfg.desktopPath,current,error))add(current);
 if(DesktopScope::systemPath(system))add(system);
 if(roots.empty()){warning+=L"无法取得快捷方式所在目录，快捷方式未删除。\n";return false;}
 bool ok=true;
 for(const auto& desktopPath:roots){
  std::wstring pattern=desktopPath+L"\\*.lnk";WIN32_FIND_DATAW data{};HANDLE find=FindFirstFileW(pattern.c_str(),&data);
  if(find==INVALID_HANDLE_VALUE){auto e=GetLastError();if(e!=ERROR_FILE_NOT_FOUND&&e!=ERROR_PATH_NOT_FOUND){ok=false;warning+=L"无法检查快捷方式目录："+desktopPath+L"\n";}continue;}
  do{
   if(data.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))continue;
   std::wstring full=desktopPath+L"\\"+data.cFileName;IShellLinkW* link=nullptr;IPersistFile* file=nullptr;
   HRESULT hr=CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link));
   if(SUCCEEDED(hr))hr=link->QueryInterface(IID_PPV_ARGS(&file));
   if(SUCCEEDED(hr))hr=file->Load(full.c_str(),STGM_READ);
   wchar_t target[32768]{},args[32768]{};WIN32_FIND_DATAW ignored{};
   if(SUCCEEDED(hr))hr=link->GetPath(target,32768,&ignored,SLGP_RAWPATH);
   if(SUCCEEDED(hr))hr=link->GetArguments(args,32768);
   bool match=SUCCEEDED(hr)&&samePath(target,exePath)&&std::wstring(args)==L"--entry "+entry.id;
   if(file)file->Release();if(link)link->Release();
   if(match){if(DeleteFileW(full.c_str()))SHChangeNotify(SHCNE_DELETE,SHCNF_PATHW|SHCNF_FLUSHNOWAIT,full.c_str(),nullptr);else{ok=false;warning+=L"无法删除桌面快捷方式："+full+L"\n";}}
  }while(FindNextFileW(find,&data));FindClose(find);
 }return ok;
}
static bool removeGeneratedIcon(const Entry& entry,std::wstring& warning){
 auto expected=baseDir+L"\\icons\\entries\\"+entry.id+L".ico";
 if(!samePath(entry.icon,expected))return true;
 DWORD attr=GetFileAttributesW(expected.c_str());if(attr==INVALID_FILE_ATTRIBUTES)return true;
 if(DeleteFileW(expected.c_str()))return true;warning+=L"无法删除入口图标："+expected+L"\n";return false;
}
bool removeDesktopEntry(size_t index,std::wstring& warning,std::wstring& error){
 warning.clear();error.clear();if(index<3||index>=entries.size()){error=L"只能删除已生成的桌面展开快捷方式，不能删除内置入口。";return false;}
 auto selectedId=entries[index].id;DesktopModes::Guard modeGuard;if(!modeGuard.held){error=L"打开方式正在保存，请稍后重试。";return false;}loadConfig();index=entries.size();for(size_t i=3;i<entries.size();++i)if(entries[i].id==selectedId){index=i;break;}if(index==entries.size()){error=L"入口已经由另一个窗口移除，请刷新后重试。";return false;}
 if(!DesktopModes::validStore){error=L"打开方式记录不可读，未删除入口。";return false;}
 for(const auto&r:DesktopModes::records)if(!r.folder){IShellLinkW*l=nullptr;IPersistFile*f=nullptr;bool match=false;
  if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&l)))&&SUCCEEDED(l->QueryInterface(IID_PPV_ARGS(&f)))&&SUCCEEDED(f->Load(r.path.c_str(),STGM_READ))){wchar_t target[32768]{},args[32768]{};l->GetPath(target,32768,nullptr,SLGP_RAWPATH);l->GetArguments(args,32768);match=samePath(target,exePath)&&std::wstring(args)==L"--entry "+entries[index].id;}
  if(f)f->Release();if(l)l->Release();if(match){error=L"此入口已单独设置打开方式，请先在该图标的右键菜单恢复默认，再删除展开入口。";return false;}}
 Entry removed=entries[index];auto before=entries;entries.erase(entries.begin()+index);
 if(!saveConfig(nullptr)){entries=std::move(before);error=configSaveError;return false;}
 removeGeneratedShortcut(removed,warning);removeGeneratedIcon(removed,warning);
 // Older candidates could leave a static badge on a shortcut source. Only the
 // last committed reference may restore it; a live desktop role still owns it.
 if(!DesktopModes::direct(removed.target)&&!hasCustomTarget(removed.target)&&!FolderRules::contains(folderRules,removed.target,true)){
  std::wstring badgeError;
  if(!FolderBadge::restore(baseDir+L"\\folder-badges",removed.target,badgeError))
   warning+=L"源文件夹的历史角标未能恢复："+removed.target+L"\n"+badgeError+L"\n可在设置中使用恢复原图标重试。\n";
 }
 SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST|SHCNF_FLUSHNOWAIT,nullptr,nullptr);return true;
}
}
