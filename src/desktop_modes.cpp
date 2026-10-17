#include "desktop_modes.h"
#include "desktop_scope.h"
#include "folder_badge.h"
#include <cstdio>
namespace DesktopModes {
namespace {
using Bytes=std::vector<BYTE>;
struct Link {IShellLinkW*l=nullptr;IPersistFile*f=nullptr;~Link(){if(f)f->Release();if(l)l->Release();}bool open(const std::wstring&p){return SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&l)))&&SUCCEEDED(l->QueryInterface(IID_PPV_ARGS(&f)))&&SUCCEEDED(f->Load(p.c_str(),STGM_READ));}};
bool read(const std::wstring&p,Bytes&b){HANDLE h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(h==INVALID_HANDLE_VALUE)return false;BY_HANDLE_FILE_INFORMATION i{};bool ok=GetFileInformationByHandle(h,&i)&&i.nNumberOfLinks==1&&!(i.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))&&!i.nFileSizeHigh&&i.nFileSizeLow<=4*1024*1024;if(ok){b.resize(i.nFileSizeLow);DWORD n;ok=ReadFile(h,b.data(),(DWORD)b.size(),&n,nullptr)&&n==b.size();}CloseHandle(h);return ok;}
bool write(const std::wstring&p,const Bytes&b){HANDLE h=CreateFileW(p.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return false;DWORD n;bool ok=WriteFile(h,b.data(),(DWORD)b.size(),&n,nullptr)&&n==b.size()&&FlushFileBuffers(h);CloseHandle(h);return ok;}
std::wstring unique(){GUID g{};if(FAILED(CoCreateGuid(&g)))return {};wchar_t b[40]{};StringFromGUID2(g,b,40);std::wstring s;for(auto c:std::wstring(b))if(iswxdigit(c))s+=c;return s;}
void step(const wchar_t*name);
bool verifyDisplaced(const std::wstring&p,const Bytes&expected,const Bytes&next,const std::wstring&backup,const std::wstring&displaced,std::wstring&error){
 Bytes current;
 Bytes actual;if(read(displaced,actual)&&actual==expected){return true;}
 auto retained=backup+L"\\concurrent-"+unique()+L".lnk";
 bool copied=CopyFileW(displaced.c_str(),retained.c_str(),TRUE)!=FALSE;
 std::wstring recovery=copied?retained:displaced;
 if(copied&&read(p,current)&&current==next){
  auto rollback=displaced+L".rollback-"+unique();
  if(ReplaceFileW(p.c_str(),displaced.c_str(),rollback.c_str(),0,nullptr,nullptr)){
   Bytes other;if(read(rollback,other)&&other==next)DeleteFileW(rollback.c_str());
   else recovery+=L"\n"+rollback; // Keep a second concurrent edit as well.
  }
 }
 error=L"检测到快捷方式同时被其它程序修改，已停止设置。外部修改已保留，请检查入口；恢复材料：\n"+recovery;
 return false;
}
// Both names are already in the durable pending record. This recovers the
// displaced file even if the process exits immediately after ReplaceFileW.
std::wstring displacedPath(const Record&r){return r.path.substr(0,r.path.find_last_of(L"\\/")+1)+L".fi-"+PathFindFileNameW(r.backup.c_str())+L"-"+PathFindFileNameW(r.staged.c_str())+L".previous";}
bool checkDisplaced(const Record&r,std::wstring&error){
 if(r.folder||r.staged.empty())return true;auto displaced=displacedPath(r);
 if(GetFileAttributesW(displaced.c_str())==INVALID_FILE_ATTRIBUTES)return true;
 if(!DesktopScope::directChild(r.backup,baseDir+L"\\mode-backups")||!DesktopScope::directChild(r.staged,r.backup)||(!r.applied.empty()&&!DesktopScope::directChild(r.applied,r.backup))){error=L"恢复路径无效。";return false;}
 auto expected=r.mode==Mode::Native?r.backup+L"\\original.lnk":r.applied.empty()?r.backup+L"\\applied.lnk":r.applied;Bytes before,after;
 if(!read(expected,before)||!read(r.staged,after)){error=L"无法核验中断操作的原件，恢复材料已保留。";return false;}
 return verifyDisplaced(r.path,before,after,r.backup,displaced,error);
}
bool replace(const std::wstring&p,const Bytes&expected,const Bytes&next,const Record&r,std::wstring&error){
 auto slash=p.find_last_of(L"\\/");auto tmp=p.substr(0,slash+1)+L".fi-"+unique()+L".tmp";auto displaced=displacedPath(r);
 if(!write(tmp,next))return false;
 Bytes current;if(!read(p,current)||current!=expected||(GetFileAttributesW(p.c_str())&FILE_ATTRIBUTE_READONLY)){DeleteFileW(tmp.c_str());return false;}
 if(GetFileAttributesW(displaced.c_str())!=INVALID_FILE_ATTRIBUTES){Bytes old;if(!read(displaced,old)||old!=expected||!DeleteFileW(displaced.c_str())){DeleteFileW(tmp.c_str());return false;}}
 step(L"replace-ready");
 if(!ReplaceFileW(p.c_str(),tmp.c_str(),displaced.c_str(),0,nullptr,nullptr)){DeleteFileW(tmp.c_str());return false;}
 step(L"replace-applied");
 return verifyDisplaced(p,expected,next,r.backup,displaced,error);
}
bool save(){auto target=storePath(),tmp=target+L"."+unique()+L".tmp";if(!write(tmp,{0xff,0xfe}))return false;bool ok=true;auto put=[&](const std::wstring&s,const wchar_t*k,const std::wstring&v){ok=WritePrivateProfileStringW(s.c_str(),k,v.c_str(),tmp.c_str())&&ok;};put(L"Modes",L"Format",L"1");put(L"Modes",L"Count",std::to_wstring(records.size()));for(size_t i=0;i<records.size();++i){auto s=L"Item"+std::to_wstring(i);auto&r=records[i];put(s,L"Path",r.path);put(s,L"Backup",r.backup);put(s,L"Mode",std::to_wstring((int)r.mode));put(s,L"Folder",r.folder?L"1":L"0");put(s,L"Pending",std::to_wstring(r.pending));put(s,L"Staged",r.staged);put(s,L"Applied",r.applied);}WritePrivateProfileStringW(nullptr,nullptr,nullptr,tmp.c_str());if(ok)ok=MoveFileExW(tmp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;if(!ok)DeleteFileW(tmp.c_str());return ok;}
bool fail(std::wstring&e,const wchar_t*s){e=s;return false;}
std::wstring linkFolder(Link&l,bool&managed){wchar_t target[32768]{},args[32768]{};l.l->GetPath(target,32768,nullptr,SLGP_RAWPATH);l.l->GetArguments(args,32768);managed=false;if(samePath(target,exePath)){for(const auto&e:entries)if(std::wstring(args)==L"--entry "+e.id){managed=true;return e.target;}}return args[0]?L"":DesktopScope::realFolder(target)?target:L"";}
}
Info inspect(const std::wstring&path){Info i;i.path=FolderRules::normalize(path);std::wstring root;if(!DesktopScope::resolve(cfg.desktopPath,root,i.error)||!DesktopScope::directChild(i.path,root)){i.error=L"仅支持当前桌面第一层的真实文件夹和快捷方式。";return i;}DWORD a=GetFileAttributesW(i.path.c_str());if(a==INVALID_FILE_ATTRIBUTES||(a&FILE_ATTRIBUTE_REPARSE_POINT)){i.error=L"入口不可访问或属于链接目录。";return i;}i.folder=(a&FILE_ATTRIBUTE_DIRECTORY)!=0;if(i.folder){i.supported=i.canPanel=true;i.target=i.path;i.mode=FolderRules::contains(folderRules,i.path,true)?Mode::Panel:Mode::Native;}else if(!_wcsicmp(PathFindExtensionW(i.path.c_str()),L".lnk")){Link l;if(!l.open(i.path)){i.error=L"无法读取快捷方式。";return i;}i.supported=true;i.target=linkFolder(l,i.managedEntry);i.canPanel=!i.target.empty();if(i.managedEntry)i.mode=Mode::Panel;}else i.error=L"首版支持桌面文件夹与.lnk快捷方式。普通文件请使用其快捷方式。";
 if(auto r=find(i.path)){i.mode=r->mode;if(r->folder&&r->mode==Mode::Panel&&!FolderRules::contains(folderRules,i.path,true))i.mode=Mode::Native;}return i;
}
namespace {
void step(const wchar_t*name){
#ifdef FOLDERICON_MODE_TEST
 if(checkpoint)checkpoint(name);
#else
 (void)name;
#endif
}
// Persist both snapshots and an index before changing an entry. On failure keep
// the pending index, so retry never depends on a best-effort rollback.
bool cancelPending(size_t index,std::wstring&error){
 Record old=records[index];auto&r=records[index];
 if(!r.folder){
  if(!DesktopScope::directChild(r.backup,baseDir+L"\\mode-backups")||!DesktopScope::directChild(r.staged,r.backup)||(!r.applied.empty()&&!DesktopScope::directChild(r.applied,r.backup)))return fail(error,L"恢复路径无效。");
  Bytes current,before,after;auto expected=r.mode==Mode::Native?r.backup+L"\\original.lnk":r.applied.empty()?r.backup+L"\\applied.lnk":r.applied;
  if(!read(r.path,current)||!read(expected,before)||!read(r.staged,after)||(current!=before&&current!=after))return fail(error,L"快捷方式已被外部修改，未覆盖；恢复材料已保留。");
  auto snap=r.backup+L"\\cancel-"+unique()+L".lnk";if(!write(snap,current))return fail(error,L"无法保存取消进度，恢复材料已保留。");
  r.applied=snap;r.mode=Mode::Direct;r.staged=r.backup+L"\\original.lnk";
 }
 r.pending=(int)Mode::Native;
 if(!save()){records[index]=old;return fail(error,L"无法保存恢复进度，请重试。");}return true;
}
bool finish(size_t index,std::wstring&error){
 Record pendingRecord=records[index];auto&r=records[index];
 if(r.pending<0)return true;if(!checkDisplaced(r,error))return false;Mode wanted=(Mode)r.pending;
 if(r.folder){
  if(!(wanted==Mode::Native?FolderBadge::restore(baseDir+L"\\folder-badges",r.path,error):FolderBadge::applyMode(baseDir+L"\\folder-badges",r.path,wanted==Mode::Direct,error)))return false;
  step(L"entry");
  FolderRules::set(folderRules,r.path,FolderRules::contains(folderRules,r.path,false),wanted==Mode::Panel);
  if(!saveConfig(nullptr))return fail(error,L"目录外观已更新，配置尚未完成；请重试，恢复记录已保留。");
 }else{
  if(!DesktopScope::directChild(r.backup,baseDir+L"\\mode-backups")||!DesktopScope::directChild(r.staged,r.backup)||(!r.applied.empty()&&!DesktopScope::directChild(r.applied,r.backup)))return fail(error,L"恢复路径无效，未覆盖入口。");
  Bytes before,after,current;auto expected=r.mode==Mode::Native?r.backup+L"\\original.lnk":r.applied.empty()?r.backup+L"\\applied.lnk":r.applied;
  if(!read(expected,before)||!read(r.staged,after)||!read(r.path,current))return fail(error,L"保存进度或快捷方式不可读，恢复索引已保留。");
  if(current!=after&&(current!=before||!replace(r.path,before,after,r,error))){if(error.empty())error=L"快捷方式被占用或已被外部修改，已保留新内容和恢复材料。";return false;}
  step(L"entry");r.applied=r.staged;
 }
 step(L"applied");step(L"config");
 r.mode=wanted;r.pending=-1;r.staged.clear();
 if(wanted==Mode::Native)records.erase(records.begin()+index);
 if(!save()){if(wanted==Mode::Native)records.insert(records.begin()+index,pendingRecord);else records[index]=pendingRecord;return fail(error,L"入口已更新，最后保存未完成。请重试；恢复索引仍然保留。");}
 if(!pendingRecord.folder)DeleteFileW(displacedPath(pendingRecord).c_str());
 SHChangeNotify(SHCNE_UPDATEITEM,SHCNF_PATHW|SHCNF_FLUSHNOWAIT,pendingRecord.path.c_str(),nullptr);return true;
}
}
static bool setInternal(const std::wstring&path,Mode mode,std::wstring&error,bool refresh){
 error.clear();Guard lock;if(!lock.held)return fail(error,L"另一个窗口正在修改打开方式，请稍后重试。");
 loadConfig();if(!validStore)return fail(error,L"打开方式记录不完整，已停止修改并保留恢复材料。");
 if(mode!=Mode::Native&&mode!=Mode::Panel&&mode!=Mode::Direct)return fail(error,L"打开方式无效。");
 for(size_t i=0;i<records.size();++i)if(samePath(records[i].path,path)&&records[i].pending>=0){if(!checkDisplaced(records[i],error))return false;auto oldDisplaced=records[i].folder?L"":displacedPath(records[i]);if(mode==Mode::Native&&!cancelPending(i,error))return false;if(!finish(i,error))return false;if(!oldDisplaced.empty())DeleteFileW(oldDisplaced.c_str());break;}
 auto info=inspect(path);if(!info.supported)return fail(error,info.error.c_str());if(mode==Mode::Panel&&!info.canPanel)return fail(error,L"这个快捷方式不指向文件夹，不能展开面板。");
 auto existing=find(info.path);if(!refresh&&info.mode==mode&&(!existing||existing->mode==mode))return true;
 DWORD attrs=GetFileAttributesW(info.path.c_str());if(attrs&FILE_ATTRIBUTE_READONLY&&!info.folder)return fail(error,L"快捷方式只读，未修改。");
 DWORD configAttrs=GetFileAttributesW(iniPath.c_str()),storeAttrs=GetFileAttributesW(storePath().c_str());if((configAttrs!=INVALID_FILE_ATTRIBUTES&&(configAttrs&FILE_ATTRIBUTE_READONLY))||(storeAttrs!=INVALID_FILE_ATTRIBUTES&&(storeAttrs&FILE_ATTRIBUTE_READONLY)))return fail(error,L"配置只读，未修改入口。");
 auto before=records;Record record=existing?*existing:Record{info.path,L"",info.mode,info.folder};
 if(!info.folder){
  Bytes original,current,next;if(!read(info.path,current))return fail(error,L"无法安全读取快捷方式，未修改。");
  if(record.backup.empty()){
   record.backup=baseDir+L"\\mode-backups\\"+unique();if(SHCreateDirectoryExW(nullptr,record.backup.c_str(),nullptr)!=ERROR_SUCCESS)return fail(error,L"无法创建原图标恢复目录。");
   if(!write(record.backup+L"\\original.lnk",current))return fail(error,L"无法备份原快捷方式。");
   record.applied=record.backup+L"\\original.lnk";
  }
  if(!DesktopScope::directChild(record.backup,baseDir+L"\\mode-backups")||(!record.applied.empty()&&!DesktopScope::directChild(record.applied,record.backup)))return fail(error,L"恢复记录路径无效。");
  auto originalFile=record.backup+L"\\original.lnk";if(!read(originalFile,original))return fail(error,L"原快捷方式备份不可读。");
  Bytes applied;auto expected=record.mode==Mode::Native?originalFile:record.applied.empty()?record.backup+L"\\applied.lnk":record.applied;
  if(!read(expected,applied)||applied!=current)return fail(error,L"快捷方式在设置后被外部修改，已保留新内容；原件备份仍保留。");
  next=original;
  if(mode!=Mode::Native){Link l;if(!l.open(originalFile))return fail(error,L"原快捷方式备份无法解析。");wchar_t icon[32768]{};int index=0;l.l->GetIconLocation(icon,32768,&index);std::wstring source=icon;
   if(!source.empty()&&PathIsRelativeW(source.c_str()))source=info.path.substr(0,info.path.find_last_of(L"\\/"))+L"\\"+source;
   auto destination=record.backup+L"\\badge-"+unique()+L".ico";
   if(!FolderBadge::modeIcon(originalFile,source,index,destination,mode==Mode::Direct,error))return false;
   if(FAILED(l.l->SetIconLocation(destination.c_str(),0)))return fail(error,L"无法设置快捷方式图标。");
   auto temp=record.backup+L"\\next-"+unique()+L".lnk";if(FAILED(l.f->Save(temp.c_str(),FALSE))||!read(temp,next))return fail(error,L"无法生成新的快捷方式。");
  }
  record.staged=record.backup+L"\\staged-"+unique()+L".lnk";if(!write(record.staged,next))return fail(error,L"无法保存操作进度，入口未修改。");
 }
 record.pending=(int)mode;size_t index=existing?size_t(existing-records.data()):records.size();
 if(existing)records[index]=record;else records.push_back(record);
 if(!save()){records=before;return fail(error,L"无法保存恢复索引，入口未修改。");}
 step(L"prepared");return finish(index,error);
}
bool set(const std::wstring&path,Mode mode,std::wstring&error){return setInternal(path,mode,error,false);}
bool repairBadge(const std::wstring&path,std::wstring&error){
 Guard lock;if(!lock.held)return fail(error,L"另一个窗口正在修改打开方式，请稍后重试。");
 loadConfig();auto info=inspect(path);if(!info.supported||info.mode==Mode::Native)return fail(error,L"请先为这个桌面项目设置打开方式。");
 return setInternal(path,info.mode,error,true);
}

}
