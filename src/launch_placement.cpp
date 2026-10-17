#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "launch_placement.h"
#include "launch_geometry.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shldisp.h>
#include <exdisp.h>
#include <dwmapi.h>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <mutex>
#include <atomic>

namespace LaunchPlacement {
namespace {
// Two successive activations of the same app must not fight over its window.
std::mutex requestMutex;
std::map<std::wstring,uint64_t> requests;
std::atomic<uint64_t> requestCounter{0};
struct Request {
 std::wstring key;uint64_t id=++requestCounter;
 explicit Request(std::wstring value):key(std::move(value)){
  std::transform(key.begin(),key.end(),key.begin(),[](wchar_t c){return towlower(c);});
  std::lock_guard<std::mutex> lock(requestMutex);requests[key]=id;
 }
 bool current(){std::lock_guard<std::mutex> lock(requestMutex);auto i=requests.find(key);return i!=requests.end()&&i->second==id;}
 ~Request(){std::lock_guard<std::mutex> lock(requestMutex);auto i=requests.find(key);if(i!=requests.end()&&i->second==id)requests.erase(i);}
};
std::wstring normalized(std::wstring path){
 if(path.empty())return path;wchar_t expanded[32768]{},full[32768]{};
 DWORD n=ExpandEnvironmentStringsW(path.c_str(),expanded,32768);if(n&&n<=32768)path=expanded;
 n=GetFullPathNameW(path.c_str(),32768,full,nullptr);if(n&&n<32768)path=full;
 std::replace(path.begin(),path.end(),L'/',L'\\');while(path.size()>3&&path.back()==L'\\')path.pop_back();return path;
}
bool same(const std::wstring&a,const std::wstring&b){return !a.empty()&&!b.empty()&&!_wcsicmp(a.c_str(),b.c_str());}
DWORD pid(HWND h){DWORD p=0;GetWindowThreadProcessId(h,&p);return p;}
std::wstring imagePath(DWORD process){
 HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,process);if(!h)return {};
 wchar_t buffer[32768]{};DWORD size=32768;bool ok=QueryFullProcessImageNameW(h,0,buffer,&size)!=FALSE;CloseHandle(h);return ok?normalized(buffer):L"";
}
struct Target {std::wstring folder,image;};
Target identify(const std::wstring&path,bool directory){
 Target target;std::wstring item=normalized(path);
 if(directory){target.folder=item;return target;}
 if(!_wcsicmp(PathFindExtensionW(item.c_str()),L".lnk")){
  IShellLinkW*link=nullptr;IPersistFile*file=nullptr;
  if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link)))&&
     SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))&&SUCCEEDED(file->Load(item.c_str(),STGM_READ))){
   wchar_t raw[32768]{};if(SUCCEEDED(link->GetPath(raw,32768,nullptr,SLGP_RAWPATH))&&raw[0])item=normalized(raw);
  }
  if(file)file->Release();if(link)link->Release();
 }
 DWORD attr=GetFileAttributesW(item.c_str());
 if(attr!=INVALID_FILE_ATTRIBUTES&&(attr&FILE_ATTRIBUTE_DIRECTORY)){target.folder=item;return target;}
 if(!_wcsicmp(PathFindExtensionW(item.c_str()),L".exe")){target.image=item;return target;}
 wchar_t associated[32768]{};DWORD count=32768;
 if(SUCCEEDED(AssocQueryStringW(ASSOCF_NONE,ASSOCSTR_EXECUTABLE,PathFindExtensionW(item.c_str()),L"open",associated,&count)))target.image=normalized(associated);
 return target;
}
bool normalWindow(HWND h){
 if(!IsWindow(h)||!IsWindowVisible(h)||GetAncestor(h,GA_ROOT)!=h)return false;
 LONG_PTR ex=GetWindowLongPtrW(h,GWL_EXSTYLE),style=GetWindowLongPtrW(h,GWL_STYLE);
 if((style&WS_CHILD)||(ex&(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE))||GetWindow(h,GW_OWNER))return false;
 DWORD cloaked=0;if(SUCCEEDED(DwmGetWindowAttribute(h,DWMWA_CLOAKED,&cloaked,sizeof(cloaked)))&&cloaked)return false;
 wchar_t cls[128]{};GetClassNameW(h,cls,128);
 return wcscmp(cls,L"Progman")&&wcscmp(cls,L"WorkerW")&&wcscmp(cls,L"Shell_TrayWnd")&&wcscmp(cls,L"Shell_SecondaryTrayWnd");
}
struct Before {DWORD process;bool visible,minimized;};
struct Watch {
 std::map<HWND,Before> before;std::map<HWND,DWORD> activated;std::set<HWND> userMoved;
 HWND foreground=GetForegroundWindow();DWORD since=GetTickCount();
};
thread_local Watch*watch=nullptr;
void CALLBACK event(HWINEVENTHOOK,DWORD kind,HWND window,LONG object,LONG child,DWORD,DWORD time){
 if(!watch||!window||int32_t(time-watch->since)<0)return;
 if(kind==EVENT_SYSTEM_FOREGROUND)watch->activated[GetAncestor(window,GA_ROOT)]=pid(window);
 else if(kind==EVENT_SYSTEM_MOVESIZESTART)watch->userMoved.insert(GetAncestor(window,GA_ROOT));
 else if(kind==EVENT_OBJECT_DESTROY&&object==OBJID_WINDOW&&child==CHILDID_SELF){watch->before.erase(window);watch->activated.erase(window);}
}
struct Hooks {
 HWINEVENTHOOK foreground=nullptr,moving=nullptr,destroy=nullptr;
 Hooks(){foreground=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,event,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
  moving=SetWinEventHook(EVENT_SYSTEM_MOVESIZESTART,EVENT_SYSTEM_MOVESIZESTART,nullptr,event,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
  destroy=SetWinEventHook(EVENT_OBJECT_DESTROY,EVENT_OBJECT_DESTROY,nullptr,event,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);}
 ~Hooks(){if(foreground)UnhookWinEvent(foreground);if(moving)UnhookWinEvent(moving);if(destroy)UnhookWinEvent(destroy);watch=nullptr;}
};
void messages(){MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}}
std::set<HWND> folders(const std::wstring&path){
 std::set<HWND> result;IShellWindows* windows=nullptr;
 if(FAILED(CoCreateInstance(CLSID_ShellWindows,nullptr,CLSCTX_LOCAL_SERVER,IID_PPV_ARGS(&windows))))return result;
 long count=0;windows->get_Count(&count);
 for(long i=0;i<count&&i<256;++i){VARIANT index{};index.vt=VT_I4;index.lVal=i;IDispatch*disp=nullptr;IWebBrowser2*browser=nullptr;
  if(SUCCEEDED(windows->Item(index,&disp))&&disp&&SUCCEEDED(disp->QueryInterface(IID_PPV_ARGS(&browser)))){
   SHANDLE_PTR handle=0;browser->get_HWND(&handle);BSTR url=nullptr;
   if(SUCCEEDED(browser->get_LocationURL(&url))&&url){wchar_t decoded[32768]{};DWORD length=32768;
    if(SUCCEEDED(PathCreateFromUrlW(url,decoded,&length,0))&&same(normalized(decoded),path))result.insert((HWND)handle);}
   SysFreeString(url);browser->Release();
  }
  if(disp)disp->Release();
 }
 windows->Release();return result;
}
struct Search {Watch& state;const Target& target;DWORD launched;std::set<HWND> folders;HWND best=nullptr;int score=0;bool ambiguous=false;};
BOOL CALLBACK candidate(HWND h,LPARAM raw){
 auto&s=*(Search*)raw;if(!normalWindow(h)||s.state.userMoved.count(h))return TRUE;
 DWORD process=pid(h);if(!process||process==GetCurrentProcessId())return TRUE;
 auto old=s.state.before.find(h);bool fresh=old==s.state.before.end()||old->second.process!=process;
 auto activation=s.state.activated.find(h);
 bool active=(activation!=s.state.activated.end()&&activation->second==process)||
             (h==GetForegroundWindow()&&h!=s.state.foreground);
 bool restored=old!=s.state.before.end()&&old->second.process==process&&
               ((!old->second.visible&&IsWindowVisible(h))||(old->second.minimized&&!IsIconic(h)));
 if(!fresh&&!active&&!restored)return TRUE;
 bool match=!s.target.folder.empty()?s.folders.count(h)!=0:
            (s.launched&&process==s.launched)||same(imagePath(process),s.target.image);
 if(!match)return TRUE;
 int score=active?30:fresh?20:10;if(s.launched==process)score+=2;
 if(score>s.score){s.best=h;s.score=score;s.ambiguous=false;}else if(score==s.score&&s.best!=h)s.ambiguous=true;
 return TRUE;
}
LaunchGeometry::Rect rect(const RECT&r){return {r.left,r.top,r.right,r.bottom};}
}
bool moveWindow(HWND window,HMONITOR monitor){
 if(!normalWindow(window))return false;
 HMONITOR from=MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST);if(from==monitor)return true;
 MONITORINFO source{sizeof(source)},target{sizeof(target)};
 if(!GetMonitorInfoW(from,&source)||!GetMonitorInfoW(monitor,&target))return false;
 WINDOWPLACEMENT placement{sizeof(placement)};if(!GetWindowPlacement(window,&placement))return false;
 bool special=IsZoomed(window)||IsIconic(window);RECT current{};
 if(special){current=placement.rcNormalPosition;OffsetRect(&current,source.rcWork.left-source.rcMonitor.left,source.rcWork.top-source.rcMonitor.top);}
 else if(!GetWindowRect(window,&current))return false;
 auto next=LaunchGeometry::fit(rect(current),rect(source.rcWork),rect(target.rcWork));
 if(special){
  // Maximized windows retain their current monitor when only the restore rect
  // changes. Move the displayed frame first, without restoring or activating it.
  if(IsZoomed(window)){
   SetWindowPos(window,nullptr,target.rcWork.left,target.rcWork.top,target.rcWork.right-target.rcWork.left,target.rcWork.bottom-target.rcWork.top,SWP_NOZORDER|SWP_NOACTIVATE|SWP_ASYNCWINDOWPOS);
   auto deadline=GetTickCount64()+300;
   while(IsWindow(window)&&MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST)!=monitor&&GetTickCount64()<deadline){messages();Sleep(10);}
  }
  placement.rcNormalPosition={next.left,next.top,next.right,next.bottom};
  OffsetRect(&placement.rcNormalPosition,target.rcMonitor.left-target.rcWork.left,target.rcMonitor.top-target.rcWork.top);
  placement.ptMaxPosition={-1,-1};placement.flags|=WPF_ASYNCWINDOWPLACEMENT;
  return SetWindowPlacement(window,&placement)!=FALSE;
 }
 return SetWindowPos(window,nullptr,next.left,next.top,next.right-next.left,next.bottom-next.top,SWP_NOZORDER|SWP_NOACTIVATE|SWP_ASYNCWINDOWPOS)!=FALSE;
}
bool open(const std::wstring&path,bool directory,POINT anchor,const Dispatcher&dispatch){
 HMONITOR monitor=MonitorFromPoint(anchor,MONITOR_DEFAULTTONEAREST);
 Target target=identify(path,directory);Request request(!target.folder.empty()?target.folder:!target.image.empty()?target.image:normalized(path));Watch state;watch=&state;
 EnumWindows([](HWND h,LPARAM p)->BOOL{auto&s=*(Watch*)p;s.before[h]={pid(h),IsWindowVisible(h)!=FALSE,IsIconic(h)!=FALSE};return TRUE;},(LPARAM)&state);
 Hooks hooks;state.since=GetTickCount();HANDLE process=nullptr;
 bool ok=dispatch(monitor,&process);DWORD launched=process?GetProcessId(process):0;if(process)CloseHandle(process);
 if(!ok)return false;
 if(!launched&&target.folder.empty()&&target.image.empty())return true;
 ULONGLONG until=GetTickCount64()+8000,stable=0;HWND previous=nullptr;
 while(GetTickCount64()<until&&request.current()){
  messages();Search search{state,target,launched,{}};
  if(!target.folder.empty())search.folders=folders(target.folder);
  EnumWindows(candidate,(LPARAM)&search);
  if(search.best&&!search.ambiguous){
   if(previous!=search.best){previous=search.best;stable=GetTickCount64();}
   else if(GetTickCount64()-stable>=80){
    // One placement per invocation. Never keep pulling a user-moved window back.
    if(request.current())moveWindow(search.best,monitor);return true;
   }
  }else{previous=nullptr;stable=0;}
  MsgWaitForMultipleObjects(0,nullptr,FALSE,60,QS_ALLINPUT);
 }
 return true; // Launch succeeded even if a third-party window cannot be identified.
}
}
