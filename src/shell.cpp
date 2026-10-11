#include "app.h"
#include "desktop_scope.h"
#include <cstdio>
Config cfg;std::vector<Entry> entries;std::wstring baseDir,exePath,iniPath,folderRulesPath;FolderRules::Rules folderRules;HINSTANCE instance;
static int readInt(const wchar_t* key,int def){return (int)GetPrivateProfileIntW(L"Appearance",key,def,iniPath.c_str());}
void setupEntries() {
    // No personal paths in source control. Missing targets use the actual Desktop.
    PWSTR desktop = nullptr;
    std::wstring fallback = L".";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktop))) {
        fallback = desktop;
        CoTaskMemFree(desktop);
    }
    entries = {{L"work", L"work", fallback, L"work.ico"},
               {L"coding", L"CodingProject", fallback, L"coding.ico"},
               {L"services", L"本地服务", fallback, L"services.ico"}};
    int count=GetPrivateProfileIntW(L"CustomEntries",L"Count",0,iniPath.c_str());
    if(count>=0&&count<=1024)for(int i=0;i<count;++i){
        wchar_t id[80]{},name[32768]{},target[32768]{},icon[32768]{};
        GetPrivateProfileStringW(L"CustomEntries",(L"Id"+std::to_wstring(i)).c_str(),L"",id,80,iniPath.c_str());
        std::wstring key=id;if(key.rfind(L"custom_",0)!=0||key.size()<=7||!std::all_of(key.begin()+7,key.end(),[](wchar_t c){return (c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z');}))continue;
        if(std::any_of(entries.begin(),entries.end(),[&](const Entry&e){return !_wcsicmp(e.id.c_str(),id);}))continue;
        GetPrivateProfileStringW(id,L"Name",L"",name,32768,iniPath.c_str());GetPrivateProfileStringW(id,L"Target",L"",target,32768,iniPath.c_str());GetPrivateProfileStringW(id,L"Icon",L"",icon,32768,iniPath.c_str());
        wchar_t location[32768]{};GetPrivateProfileStringW(id,L"Desktop",L"",location,32768,iniPath.c_str());
        if(!name[0]||!target[0])continue;entries.push_back({id,name,target,icon,location});
    }
    for (auto& entry : entries) {
        wchar_t raw[32768] = {}, expanded[32768] = {};
        GetPrivateProfileStringW(entry.id.c_str(), L"Target", entry.target.c_str(),
                                 raw, 32768, iniPath.c_str());
        DWORD length = ExpandEnvironmentStringsW(raw, expanded, 32768);
        entry.target = length && length <= 32768 ? expanded : raw;
        if (entry.target.empty()) entry.target = fallback;
    }
}
HICON applicationIcon(int size){return (HICON)LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,size,size,LR_SHARED);}
UINT monitorDpi(POINT pt){
 UINT x=96,y=96;HMODULE sh=LoadLibraryW(L"shcore.dll");
 if(sh){using Fn=HRESULT(WINAPI*)(HMONITOR,int,UINT*,UINT*);auto fn=(Fn)GetProcAddress(sh,"GetDpiForMonitor");if(fn)fn(MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST),0,&x,&y);FreeLibrary(sh);}return x;
}
DesktopFont desktopFont(UINT dpi){
 if(!dpi){using Fn=UINT(WINAPI*)();auto fn=(Fn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForSystem");dpi=fn?fn():96;}
 LOGFONTW lf{};using Fn=BOOL(WINAPI*)(UINT,UINT,PVOID,UINT,UINT);
 auto fn=(Fn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"SystemParametersInfoForDpi");
 bool ok=fn&&fn(SPI_GETICONTITLELOGFONT,sizeof(lf),&lf,0,dpi);
 if(!ok){ok=SystemParametersInfoW(SPI_GETICONTITLELOGFONT,sizeof(lf),&lf,0)!=0;if(ok){HDC dc=GetDC(nullptr);int source=GetDeviceCaps(dc,LOGPIXELSY);ReleaseDC(nullptr,dc);lf.lfHeight=MulDiv(lf.lfHeight,dpi,source);}}
 int pixels=ok?abs(lf.lfHeight):fontPixels(12,dpi);
 if(lf.lfHeight>0){HFONT f=CreateFontIndirectW(&lf);HDC dc=GetDC(nullptr);auto old=SelectObject(dc,f);TEXTMETRICW tm{};GetTextMetricsW(dc,&tm);pixels=tm.tmHeight-tm.tmInternalLeading;SelectObject(dc,old);DeleteObject(f);ReleaseDC(nullptr,dc);}
 return {ok?lf.lfFaceName:L"Microsoft YaHei UI",clampValue(pixels,8,144),MulDiv(pixels,72,dpi)};
}
void loadConfig(){wchar_t desktop[32768]{};GetPrivateProfileStringW(L"General",L"DesktopPath",L"",desktop,32768,iniPath.c_str());cfg.desktopPath=desktop;folderRulesPath=baseDir+L"\\folder-rules.bin";loadFolderRules();cfg.followFont=readInt(L"FollowDesktopFont",1)!=0;cfg.fontSize=clampValue(readInt(L"FontSize",12),8,36);if(cfg.followFont)cfg.fontSize=desktopFont().points;cfg.textGap=clampValue(readInt(L"TextGap",3),0,24);cfg.expandDelay=clampValue(readInt(L"ExpandDelay",80),0,500);cfg.dotHidden=readInt(L"DotNamesHidden",1)!=0;cfg.showHidden=readInt(L"ShowHidden",0)!=0;cfg.icon=readInt(L"IconSize",96);cfg.cellW=readInt(L"CellWidth",128);cfg.cellH=readInt(L"CellHeight",144);cfg.followDesktop=readInt(L"FollowDesktop",1)!=0;cfg.columns=clampValue(readInt(L"Columns",4),2,10);cfg.opacity=clampValue(readInt(L"Opacity",88),10,100);cfg.radius=clampValue(readInt(L"Radius",20),0,80);cfg.blur=readInt(L"Blur",1)!=0;cfg.autoStart=GetPrivateProfileIntW(L"General",L"AutoStart",GetPrivateProfileIntW(L"General",L"Resident",0,iniPath.c_str()),iniPath.c_str())!=0;cfg.contextMenu=GetPrivateProfileIntW(L"General",L"ContextMenu",0,iniPath.c_str())!=0;wchar_t b[32];GetPrivateProfileStringW(L"Appearance",L"Background",L"232830",b,32,iniPath.c_str());if(validColor(b)){unsigned long c=wcstoul(b,nullptr,16);cfg.background=RGB((c>>16)&255,(c>>8)&255,c&255);}if(cfg.followDesktop){auto m=desktopMetrics();cfg.icon=m.icon;cfg.cellW=m.cellW;cfg.cellH=m.cellH;}auto m=normalizeMetrics(cfg.icon,cfg.cellW,cfg.cellH);cfg.icon=m.icon;cfg.cellW=m.cellW;cfg.cellH=m.cellH;setupEntries();}
static IFolderView2* desktopView(){IShellWindows* sw=nullptr;IDispatch* disp=nullptr;IServiceProvider* sp=nullptr;IShellBrowser* browser=nullptr;IShellView* sv=nullptr;IFolderView2* fv=nullptr;VARIANT loc,empty;VariantInit(&loc);VariantInit(&empty);loc.vt=VT_I4;loc.lVal=CSIDL_DESKTOP;long hwnd=0;if(SUCCEEDED(CoCreateInstance(CLSID_ShellWindows,nullptr,CLSCTX_LOCAL_SERVER,IID_PPV_ARGS(&sw)))){if(SUCCEEDED(sw->FindWindowSW(&loc,&empty,SWC_DESKTOP,&hwnd,SWFO_NEEDDISPATCH,&disp))&&disp){if(SUCCEEDED(disp->QueryInterface(IID_PPV_ARGS(&sp)))){if(SUCCEEDED(sp->QueryService(SID_STopLevelBrowser,IID_PPV_ARGS(&browser)))){if(SUCCEEDED(browser->QueryActiveShellView(&sv)))sv->QueryInterface(IID_PPV_ARGS(&fv));}}}}if(sv)sv->Release();if(browser)browser->Release();if(sp)sp->Release();if(disp)disp->Release();if(sw)sw->Release();return fv;}
Metrics desktopMetrics(bool* actual){if(actual)*actual=false;Metrics m{96,128,144};auto v=desktopView();if(v){FOLDERVIEWMODE mode;int size;POINT spacing;if(SUCCEEDED(v->GetViewModeAndIconSize(&mode,&size))&&SUCCEEDED(v->GetSpacing(&spacing))){m={size,spacing.x,spacing.y};if(actual)*actual=true;}v->Release();}return normalizeMetrics(m.icon,m.cellW,m.cellH);}
bool isDesktopPoint(POINT pt){HWND h=WindowFromPoint(pt);wchar_t cls[128];if(!h)return false;GetClassNameW(h,cls,128);if(wcscmp(cls,L"SysListView32"))return false;HWND p=GetParent(h);GetClassNameW(p,cls,128);return wcscmp(cls,L"SHELLDLL_DefView")==0;}
static bool desktopItemPathAt(POINT pt,std::wstring& path,bool& isFolder){
 path.clear();isFolder=false;
 if(!isDesktopPoint(pt))return false;
 HWND lv=WindowFromPoint(pt);IAccessible* acc=nullptr;
 if(FAILED(AccessibleObjectFromWindow(lv,OBJID_CLIENT,IID_PPV_ARGS(&acc))))return false;
 VARIANT child{};
 bool hit=SUCCEEDED(acc->accHitTest(pt.x,pt.y,&child))&&child.vt==VT_I4&&child.lVal>0;
 if(!hit){VariantClear(&child);acc->Release();return false;}
 IFolderView2* view=desktopView();
 if(!view){VariantClear(&child);acc->Release();return false;}
 int count=0,icon=0;FOLDERVIEWMODE mode{};
 bool ready=SUCCEEDED(view->ItemCount(SVGIO_ALLVIEW,&count))&&count>0&&count<=4096&&
  SUCCEEDED(view->GetViewModeAndIconSize(&mode,&icon))&&icon>0;
 const ULONGLONG deadline=GetTickCount64()+100;
 // MSAA child IDs are only a fast candidate hint, never treated as Shell indexes.
 // Verify each candidate's icon position through MSAA before retrieving its PIDL path.
 // No pointer-bearing ListView message is sent into Explorer's address space.
 auto matches=[&](int index){
  PITEMID_CHILD pid=nullptr;POINT p{};bool same=false;
  if(SUCCEEDED(view->Item(index,&pid))&&pid&&SUCCEEDED(view->GetItemPosition(pid,&p))){
   p.x+=icon/2;p.y+=icon/2;ClientToScreen(lv,&p);VARIANT at{};
   same=SUCCEEDED(acc->accHitTest(p.x,p.y,&at))&&at.vt==VT_I4&&at.lVal==child.lVal;
   VariantClear(&at);
  }
  if(pid)CoTaskMemFree(pid);return same;
 };
 int index=-1,hint=child.lVal-1;
 if(ready&&hint>=0&&hint<count&&matches(hint))index=hint;
 if(ready&&index<0)for(int i=0;i<count&&GetTickCount64()<deadline;++i)if(i!=hint&&matches(i)){index=i;break;}
 bool ok=false;IShellItem* item=nullptr;
 if(index>=0&&SUCCEEDED(view->GetItem(index,IID_PPV_ARGS(&item)))&&item){
  BSTR label=nullptr;PWSTR display=nullptr;PWSTR raw=nullptr;VARIANT again{};
  ok=SUCCEEDED(acc->get_accName(child,&label))&&label&&
   SUCCEEDED(item->GetDisplayName(SIGDN_NORMALDISPLAY,&display))&&display&&!wcscmp(label,display)&&
   SUCCEEDED(acc->accHitTest(pt.x,pt.y,&again))&&again.vt==VT_I4&&again.lVal==child.lVal&&
   SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&raw))&&raw;
  if(ok){DWORD attrs=GetFileAttributesW(raw);ok=attrs!=INVALID_FILE_ATTRIBUTES;
   if(ok){path=raw;isFolder=(attrs&FILE_ATTRIBUTE_DIRECTORY)!=0;}}
  SysFreeString(label);CoTaskMemFree(display);CoTaskMemFree(raw);VariantClear(&again);item->Release();
 }
 view->Release();VariantClear(&child);acc->Release();return ok&&!path.empty();
}
DesktopHit desktopHitAt(POINT pt){DesktopHit hit;std::wstring path;bool folder=false;if(!desktopItemPathAt(pt,path,folder))return hit;hit.path=path;hit.realFolder=folder;if(folder){std::wstring error;hit.managed=FolderRules::contains(folderRules,path,true)&&DesktopScope::validate(path,cfg.desktopPath,error);return hit;}if(!_wcsicmp(PathFindExtensionW(path.c_str()),L".lnk")){IShellLinkW* link=nullptr;IPersistFile* file=nullptr;if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link)))&&SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))&&SUCCEEDED(file->Load(path.c_str(),STGM_READ))){wchar_t target[32768]={},args[2048]={};link->GetPath(target,32768,nullptr,SLGP_RAWPATH);link->GetArguments(args,2048);if(!_wcsicmp(target,exePath.c_str()))for(size_t i=0;i<entries.size();i++)if(std::wstring(args)==L"--entry "+entries[i].id){hit.entryId=(int)i;hit.managed=true;break;}}if(file)file->Release();if(link)link->Release();}return hit;}
int desktopEntryAt(POINT pt){auto hit=desktopHitAt(pt);return hit.entryId;}// Bypass the Windows image-factory cache for explicit shortcut icons. Rebuilding
// our cache must pick up edits to an external .ico even when the .lnk is unchanged.
static HBITMAP bitmapFromIcon(HICON icon,int size){
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;
 bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
 DWORD *dark=nullptr,*light=nullptr;
 HBITMAP a=CreateDIBSection(nullptr,&bi,DIB_RGB_COLORS,(void**)&dark,nullptr,0);
 HBITMAP b=CreateDIBSection(nullptr,&bi,DIB_RGB_COLORS,(void**)&light,nullptr,0);
 if(!a||!b){if(a)DeleteObject(a);if(b)DeleteObject(b);return nullptr;}
 for(int i=0;i<size*size;i++){dark[i]=0xff000000;light[i]=0xffffffff;}
 HDC dc=CreateCompatibleDC(nullptr);HGDIOBJ old=SelectObject(dc,a);
 bool ok=DrawIconEx(dc,0,0,icon,size,size,0,nullptr,DI_NORMAL)!=0;
 SelectObject(dc,b);ok=DrawIconEx(dc,0,0,icon,size,size,0,nullptr,DI_NORMAL)&&ok;
 SelectObject(dc,old);DeleteDC(dc);GdiFlush();
 // Reconstruct premultiplied alpha from black/white renders. This also handles
 // old mask-only icons without turning their transparent background black.
 if(ok)for(int i=0;i<size*size;i++){
  int difference=std::max({(int)(light[i]&255)-(int)(dark[i]&255),
   (int)((light[i]>>8)&255)-(int)((dark[i]>>8)&255),(int)((light[i]>>16)&255)-(int)((dark[i]>>16)&255)});
  dark[i]=(dark[i]&0xffffff)|((DWORD)(255-clampValue(difference,0,255))<<24);
 }
 DeleteObject(b);if(!ok){DeleteObject(a);return nullptr;}return a;
}
static HBITMAP customShortcutBitmap(const std::wstring& path,int size){
 wchar_t raw[32768]={},expanded[32768]={};int index=0;
 if(!_wcsicmp(PathFindExtensionW(path.c_str()),L".lnk")){
  IShellLinkW* link=nullptr;
  if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link)))){
   IPersistFile* file=nullptr;
   if(SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))){
    if(SUCCEEDED(file->Load(path.c_str(),STGM_READ)))link->GetIconLocation(raw,32768,&index);
    file->Release();
   }link->Release();
  }
 }else if(!_wcsicmp(PathFindExtensionW(path.c_str()),L".url")){
  GetPrivateProfileStringW(L"InternetShortcut",L"IconFile",L"",raw,32768,path.c_str());
  index=(int)GetPrivateProfileIntW(L"InternetShortcut",L"IconIndex",0,path.c_str());
 }
 if(!raw[0])return nullptr;
 DWORD n=ExpandEnvironmentStringsW(raw,expanded,32768);if(!n||n>32768)return nullptr;
 std::wstring iconPath=expanded;
 if(PathIsRelativeW(iconPath.c_str()))iconPath=path.substr(0,path.find_last_of(L"\\/")+1)+iconPath;
 HICON icon=nullptr;HBITMAP result=nullptr;
 if(SHDefExtractIconW(iconPath.c_str(),index,0,&icon,nullptr,MAKELONG(size,0))==S_OK&&icon)result=bitmapFromIcon(icon,size);
 if(icon)DestroyIcon(icon);return result;
}
HBITMAP shellBitmap(const std::wstring& path,int size){if(HBITMAP custom=customShortcutBitmap(path,size))return custom;IShellItemImageFactory* f=nullptr;HBITMAP bitmap=nullptr;if(SUCCEEDED(SHCreateItemFromParsingName(path.c_str(),nullptr,IID_PPV_ARGS(&f)))){f->GetImage({size,size},(SIIGBF)(SIIGBF_ICONONLY|SIIGBF_BIGGERSIZEOK),&bitmap);f->Release();}return bitmap;}
void launchSelf(const std::wstring& args){SHELLEXECUTEINFOW s={sizeof(s)};s.lpFile=exePath.c_str();s.lpParameters=args.c_str();s.nShow=SW_SHOWNORMAL;ShellExecuteExW(&s);}
bool openShellPath(HWND parent,const std::wstring& path,bool directory){
 if(path.empty())return false;
 auto target=path;
 // ShellExecute probes extensions even for an existing directory. A trailing
 // separator prevents a same-name .lnk/.exe from taking precedence.
 if(directory&&target.back()!=L'\\'&&target.back()!=L'/')target+=L'\\';
 return (INT_PTR)ShellExecuteW(parent,L"open",target.c_str(),nullptr,nullptr,SW_SHOWNORMAL)>32;
}
void openEntry(int id){if(id<0||id>=(int)entries.size())return;if(!openShellPath(nullptr,entries[id].target,true))MessageBoxW(nullptr,L"无法打开目标文件夹，请检查路径是否仍然有效。",L"文件夹面板",MB_ICONERROR);}
bool applyStartup(bool enabled){HKEY k;LONG e=RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,nullptr,0,KEY_SET_VALUE,nullptr,&k,nullptr);if(e!=ERROR_SUCCESS)return false;if(enabled){auto value=L"\""+exePath+L"\" --resident";e=RegSetValueExW(k,L"FolderIconCN",0,REG_SZ,(BYTE*)value.c_str(),(DWORD)((value.size()+1)*sizeof(wchar_t)));}else{e=RegDeleteValueW(k,L"FolderIconCN");if(e==ERROR_FILE_NOT_FOUND)e=ERROR_SUCCESS;}RegCloseKey(k);return e==ERROR_SUCCESS;}
void diagnose(const std::wstring& file){bool real;auto m=desktopMetrics(&real);auto f=desktopFont();FILE* out=_wfopen(file.c_str(),L"wb");if(out){fprintf(out,"{\"desktopMetricsActual\":%s,\"icon\":%d,\"cellWidth\":%d,\"cellHeight\":%d,\"desktopFontPixels\":%d,\"desktopFontPoints\":%d,\"fontSizePoints\":%d,\"followDesktopFont\":%d,\"textGap\":%d,\"expandDelay\":%d,\"autoStartConfig\":%d}\n",real?"true":"false",m.icon,m.cellW,m.cellH,f.pixels,f.points,cfg.fontSize,cfg.followFont,cfg.textGap,cfg.expandDelay,cfg.autoStart);fclose(out);}}
