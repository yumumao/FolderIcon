#include "entry_maintenance.h"
#include "entry_actions.h"
#include "folder_badge.h"
#include "desktop_scope.h"
#include <set>
namespace EntryMaintenance {
namespace {
bool same(const std::wstring&a,const std::wstring&b){auto x=FolderRules::normalize(a),y=FolderRules::normalize(b);return !x.empty()&&!y.empty()&&!_wcsicmp(x.c_str(),y.c_str());}
void add(std::vector<std::wstring>&v,const std::wstring&p){if(p.empty()||PathIsRelativeW(p.c_str()))return;auto n=FolderRules::normalize(p);for(const auto&x:v)if(same(x,n))return;v.push_back(n);}
std::vector<std::wstring> roots(){std::vector<std::wstring> r;for(size_t i=3;i<entries.size();++i)add(r,entries[i].desktop);std::wstring path,error;if(DesktopScope::resolve(cfg.desktopPath,path,error))add(r,path);else if(!cfg.desktopPath.empty())add(r,cfg.desktopPath);if(DesktopScope::systemPath(path))add(r,path);return r;}
struct Link {std::wstring path,id,icon;int iconIndex=0;};
struct Scan {std::vector<Link> links;bool complete=true;};
struct ShellLink{IShellLinkW*l=nullptr;IPersistFile*f=nullptr;~ShellLink(){if(f)f->Release();if(l)l->Release();}bool load(const std::wstring&p){return SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&l)))&&SUCCEEDED(l->QueryInterface(IID_PPV_ARGS(&f)))&&SUCCEEDED(f->Load(p.c_str(),STGM_READ));}};
Scan scan(const std::vector<std::wstring>&locations){
 Scan s;if(locations.empty())s.complete=false;
 for(const auto&root:locations){if(!DesktopScope::realFolder(root)){s.complete=false;continue;}
  WIN32_FIND_DATAW data{};HANDLE h=FindFirstFileW((root+L"\\*.lnk").c_str(),&data);if(h==INVALID_HANDLE_VALUE){if(GetLastError()!=ERROR_FILE_NOT_FOUND)s.complete=false;continue;}
  size_t count=0;do{if(++count>8192){s.complete=false;break;}if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;
   if(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT){s.complete=false;continue;}
   std::wstring path=root+L"\\"+data.cFileName;ShellLink l;if(!l.load(path)){s.complete=false;continue;}
   wchar_t target[32768]{},args[32768]{},icon[32768]{};int index=0;
   if(FAILED(l.l->GetPath(target,32768,nullptr,SLGP_RAWPATH))||FAILED(l.l->GetArguments(args,32768))){s.complete=false;continue;}
   if(!same(target,exePath))continue;std::wstring arguments=args;
   if(arguments.rfind(L"--entry ",0)!=0)continue;std::wstring id=arguments.substr(8);
   if(FAILED(l.l->GetIconLocation(icon,32768,&index))){s.complete=false;continue;}s.links.push_back({path,id,icon,index});
  }while(FindNextFileW(h,&data));if(GetLastError()!=ERROR_NO_MORE_FILES)s.complete=false;FindClose(h);
 }return s;
}
bool owned(const std::wstring&p,const Entry&e){auto root=baseDir+L"\\icons\\entries";if(!DesktopScope::directChild(p,root))return false;std::wstring name=PathFindFileNameW(p.c_str());if(!_wcsicmp(name.c_str(),(e.id+L".ico").c_str()))return true;auto prefix=e.id+L"_badge_";if(name.rfind(prefix,0)!=0||name.size()!=prefix.size()+32+4||name.substr(name.size()-4)!=L".ico")return false;return std::all_of(name.begin()+prefix.size(),name.end()-4,[](wchar_t c){return iswxdigit(c)!=0;});}
bool referenced(const std::wstring&p){if(FolderRules::contains(folderRules,p,true))return true;for(size_t i=3;i<entries.size();++i)if(same(entries[i].target,p))return true;return false;}
std::wstring unique(){GUID g{};if(FAILED(CoCreateGuid(&g)))return {};wchar_t b[40]{};StringFromGUID2(g,b,40);std::wstring s;for(auto c:std::wstring(b))if(iswxdigit(c))s+=c;return s;}
struct Watch{std::wstring root;HANDLE h=INVALID_HANDLE_VALUE;};std::vector<Watch>watches;ULONGLONG nextRetry=0,due=0;bool started=false;std::wstring watchedSignature;
}
Result reconcile(bool history){
 Result result;loadConfig();
 // Invalid or incomplete configuration must never make a referenced badge look orphaned.
 if(GetFileAttributesW(iniPath.c_str())==INVALID_FILE_ATTRIBUTES||GetPrivateProfileIntW(L"CustomEntries",L"Count",0,iniPath.c_str())!=(entries.size()>3?entries.size()-3:0)){result.deferred=1;result.warning=L"配置不完整，本次未清理。";return result;}
 wchar_t format[8]{};GetPrivateProfileStringW(L"FolderRules",L"Format",L"",format,8,iniPath.c_str());if(format[0]&&(wcscmp(format,L"1")||GetPrivateProfileIntW(L"FolderRules",L"Count",-1,iniPath.c_str())!=folderRules.size())){result.deferred=1;return result;}
 auto s=scan(roots());auto before=entries;std::vector<Entry> removed;
 if(s.complete){for(size_t i=entries.size();i>3;){--i;bool found=false;for(const auto&l:s.links)if(l.id==entries[i].id){found=true;break;}if(!found){removed.push_back(entries[i]);entries.erase(entries.begin()+i);}}}
 else {++result.deferred;result.warning=L"部分桌面位置或快捷方式无法核验，相关配置暂时保留。\n";}
 if(!removed.empty()){if(!saveConfig(nullptr)){entries=std::move(before);++result.deferred;result.warning+=configSaveError;return result;}result.removed=removed.size();}
 // Keep generated icon files as recovery assets. Never delete an icon that another renamed/copied link may still use.
 if(history){size_t unresolved=0;auto folders=FolderBadge::recordedFolders(baseDir+L"\\folder-badges",unresolved);result.deferred+=unresolved;
  for(const auto&p:folders)if(!referenced(p)){std::wstring error;if(FolderBadge::restore(baseDir+L"\\folder-badges",p,error))++result.restored;else{++result.deferred;result.warning+=p+L"\n"+error+L"\n";}}
 }return result;
}
bool repairShortcutBadge(size_t index,size_t&updated,std::wstring&error){
 updated=0;error.clear();if(index>=entries.size()){error=L"请先选中一个展开快捷方式。";return false;}const auto entry=entries[index];auto s=scan(roots());size_t found=0;
 for(const auto&item:s.links){if(item.id!=entry.id)continue;++found;
  if(owned(item.icon,entry)&&GetFileAttributesW(item.icon.c_str())!=INVALID_FILE_ATTRIBUTES)continue;
  HANDLE guard=CreateFileW(item.path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);BY_HANDLE_FILE_INFORMATION identity{};
  if(guard==INVALID_HANDLE_VALUE||!GetFileInformationByHandle(guard,&identity)||identity.nNumberOfLinks!=1||(identity.dwFileAttributes&(FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_REPARSE_POINT))){if(guard!=INVALID_HANDLE_VALUE)CloseHandle(guard);error=L"快捷方式只读、被占用或身份不安全，未覆盖。";return false;}
  ShellLink l;if(!l.load(item.path)){CloseHandle(guard);error=L"无法读取快捷方式。";return false;}
  wchar_t current[32768]{};int currentIndex=0;l.l->GetIconLocation(current,32768,&currentIndex);
  auto token=unique(),dir=baseDir+L"\\icons\\entries";SHCreateDirectoryExW(nullptr,dir.c_str(),nullptr);auto ico=dir+L"\\"+entry.id+L"_badge_"+token+L".ico";
  bool generated=!token.empty()&&(current[0]?FolderBadge::resourceIcon(current,currentIndex,ico,error):FolderBadge::shortcutIcon(entry.target,ico,error));
  if(!generated){CloseHandle(guard);return false;}
  auto temp=item.path+L".FolderIcon-"+token+L".pending";
  bool ok=SUCCEEDED(l.l->SetIconLocation(ico.c_str(),0))&&SUCCEEDED(l.f->Save(temp.c_str(),FALSE));CloseHandle(guard);
  if(ok)ok=ReplaceFileW(item.path.c_str(),temp.c_str(),nullptr,0,nullptr,nullptr)!=FALSE;
  if(!ok){DeleteFileW(temp.c_str());DeleteFileW(ico.c_str());error=L"无法更新快捷方式，原图标设置已保留。";return false;}
  ++updated;SHChangeNotify(SHCNE_UPDATEITEM,SHCNF_PATHW|SHCNF_FLUSHNOWAIT,item.path.c_str(),nullptr);
 }
 if(!found){error=L"未找到该入口的可读快捷方式，请确认它仍在已知桌面位置。";return false;}return true;
}
void shutdown(){for(auto&w:watches)if(w.h!=INVALID_HANDLE_VALUE)FindCloseChangeNotification(w.h);watches.clear();started=false;due=nextRetry=0;}
bool tick(bool force){
 ULONGLONG now=GetTickCount64();bool first=!started;std::wstring signature=cfg.desktopPath;for(size_t i=3;i<entries.size();++i)signature+=L"|"+entries[i].id+L"|"+entries[i].desktop;bool changed=signature!=watchedSignature;
 bool retry=false;if(now>=nextRetry){nextRetry=now+60000;for(const auto&w:watches)if(w.h==INVALID_HANDLE_VALUE)retry=true;}
 if(first||changed||retry){watchedSignature=signature;auto locations=roots();for(auto&w:watches)if(w.h!=INVALID_HANDLE_VALUE)FindCloseChangeNotification(w.h);watches.clear();for(auto&r:locations)watches.push_back({r,FindFirstChangeNotificationW(r.c_str(),FALSE,FILE_NOTIFY_CHANGE_FILE_NAME)});started=true;due=now+2000;}
 for(auto&w:watches)if(w.h!=INVALID_HANDLE_VALUE&&WaitForSingleObject(w.h,0)==WAIT_OBJECT_0){due=now+2000;if(!FindNextChangeNotification(w.h)){FindCloseChangeNotification(w.h);w.h=INVALID_HANDLE_VALUE;}}
 if(force||first||(due&&now>=due)){due=0;auto result=reconcile();return result.removed!=0||result.restored!=0;}return false;
}
}
