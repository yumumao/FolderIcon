#include "app.h"
#include "context_menu.h"
#include "entry_actions.h"
#include "entry_maintenance.h"
#include "desktop_scope.h"
HWND controller=nullptr;bool residentMode=false,quitting=false;static HHOOK mouseHook=nullptr;static HANDLE singleton=nullptr;static NOTIFYICONDATAW tray={};static ClickState clicks,folderClicks;static int lastOpen=-1;static POINT downPoint{};static ULONGLONG openedAt=0;static UINT taskbarMessage;static bool hookDown=false,hookDragged=false;static POINT hookPoint{};static std::wstring folderLastPath,pendingFolderPath;static POINT pendingFolderPoint{};static bool pendingFolder=false;
static void resetFolderState(){folderClicks.reset();folderLastPath.clear();pendingFolder=false;pendingFolderPath.clear();}
static std::wstring chooseContextAction(bool direct,const std::wstring& folder){
 if(!direct){
  auto text=L"这个文件夹不在桌面上，不能原位替换桌面项目。\n\n按“确定”在桌面建立一个FolderIcon展开快捷方式；原文件夹不会移动、复制或删除。\n按“取消”放弃。\n\n"+folder;
  return MessageBoxW(nullptr,text.c_str(),L"添加到FolderIcon面板",MB_OKCANCEL|MB_ICONQUESTION)==IDOK?L"create":L"";
 }
 auto text=L"请选择对这个桌面文件夹的处理方式：\n\n“是”：当前文件夹原位变为FolderIcon可展开入口（不新建快捷方式）。\n“否”：保留原文件夹，在桌面另建FolderIcon展开快捷方式。\n“取消”：不做任何修改。\n\n"+folder;
 int choice=MessageBoxW(nullptr,text.c_str(),L"添加到FolderIcon面板",MB_YESNOCANCEL|MB_ICONQUESTION);
 return choice==IDYES?L"convert":choice==IDNO?L"create":L"";
}
static void notifyConfigChanged(){
 HWND controllerWindow=FindWindowW(CONTROL_CLASS,nullptr);
 if(controllerWindow&&controllerWindow!=controller)PostMessageW(controllerWindow,WM_CONFIG,0,0);
 HWND settings=FindWindowW(L"FolderIconCN.Settings",nullptr);
 if(settings)PostMessageW(settings,WM_CONFIG,0,0);
}
static int handleContextFolder(const std::wstring& requestedAction,const std::wstring& argument){
 if(!EntryActions::realFolder(argument)){MessageBoxW(nullptr,L"目标不是真实文件夹或已不可访问。",L"FolderIcon",MB_OK|MB_ICONERROR);return 2;}
 std::wstring desktopPath,desktopError;if(!DesktopScope::resolve(cfg.desktopPath,desktopPath,desktopError)){MessageBoxW(nullptr,desktopError.c_str(),L"FolderIcon",MB_OK|MB_ICONERROR);return 2;}
 auto folder=FolderRules::normalize(argument);
 if(!_wcsicmp(folder.c_str(),desktopPath.c_str())){MessageBoxW(nullptr,L"不能把桌面本身转换为入口。请选择一个文件夹。",L"FolderIcon",MB_OK|MB_ICONINFORMATION);return 2;}
 const bool direct=DesktopScope::directChild(folder,desktopPath);
 std::wstring action=requestedAction;
 if(action==L"choose"){
  action=chooseContextAction(direct,folder);
  if(action.empty())return 0;
 }
 // Keep manually-created links from older candidates usable, while the current
 // shell registration always asks the user in the dialog above.
 if(action.empty()||action==L"legacy")action=direct?L"convert":L"create";
 if(action==L"convert"){
  if(!direct){MessageBoxW(nullptr,L"只有桌面上的直接子文件夹可以原位变为FolderIcon展开入口。\n\n非桌面文件夹请使用“生成桌面展开快捷方式”。",L"FolderIcon",MB_OK|MB_ICONINFORMATION);return 2;}
  bool already=false;std::wstring warning,error;if(!EntryActions::enableDesktopFolder(folder,already,warning,error)){MessageBoxW(nullptr,error.c_str(),L"FolderIcon",MB_OK|MB_ICONERROR);return 2;}
  notifyConfigChanged();  auto message=already?L"这个桌面文件夹已经是可展开的FolderIcon入口。\n"+folder:L"已将这个桌面文件夹原位变为FolderIcon展开入口。\n"+folder;message+=L"\n\n原文件夹未移动、未删除，也未生成新的快捷方式。";if(!warning.empty())message+=L"\n\n"+warning;MessageBoxW(nullptr,message.c_str(),L"FolderIcon",MB_OK|(warning.empty()?MB_ICONINFORMATION:MB_ICONWARNING));return 0;
 }
 if(action==L"create"){
  std::wstring shortcut,error;if(!EntryActions::createDesktopEntry(folder,desktopPath,shortcut,error)){MessageBoxW(nullptr,error.c_str(),L"FolderIcon",MB_OK|MB_ICONERROR);return 2;}
  notifyConfigChanged();  auto message=shortcut.empty()?L"已存在这个文件夹的FolderIcon桌面入口。\n"+folder:L"已在桌面创建FolderIcon可展开快捷方式：\n"+shortcut;message+=L"\n\n原文件夹未移动、未复制、未删除。";MessageBoxW(nullptr,message.c_str(),L"FolderIcon",MB_OK|MB_ICONINFORMATION);return 0;
 }
 MessageBoxW(nullptr,L"无法识别右键操作。请重新打开设置并保存右键菜单。",L"FolderIcon",MB_OK|MB_ICONERROR);return 2;
}static void dispatchOpen(int id){KillTimer(controller,TIMER_CLICK);clicks.reset();resetFolderState();ULONGLONG now=GetTickCount64();if(id==lastOpen&&now-openedAt<1200)return;lastOpen=id;openedAt=now;if(panelWindow)closePanel();openEntry(id);}
static LRESULT CALLBACK hook(int code,WPARAM w,LPARAM l){if(code==HC_ACTION){auto p=(MSLLHOOKSTRUCT*)l;if(w==WM_LBUTTONDOWN){bool modifiers=(GetAsyncKeyState(VK_CONTROL)&0x8000)||(GetAsyncKeyState(VK_SHIFT)&0x8000);hookDown=!modifiers&&isDesktopPoint(p->pt);hookDragged=false;hookPoint=p->pt;if(hookDown)PostMessageW(controller,WM_HOOK,1,MAKELPARAM((short)p->pt.x,(short)p->pt.y));else PostMessageW(controller,WM_HOOK,3,0);}else if(w==WM_MOUSEMOVE&&hookDown){if(abs(p->pt.x-hookPoint.x)>GetSystemMetrics(SM_CXDRAG)||abs(p->pt.y-hookPoint.y)>GetSystemMetrics(SM_CYDRAG))hookDragged=true;}else if(w==WM_LBUTTONUP&&hookDown){hookDown=false;PostMessageW(controller,WM_HOOK,hookDragged?3:2,MAKELPARAM((short)p->pt.x,(short)p->pt.y));}}return CallNextHookEx(mouseHook,code,w,l);}
static void addTray(){tray={};tray.cbSize=sizeof(tray);tray.hWnd=controller;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=WM_TRAY;tray.hIcon=applicationIcon(GetSystemMetrics(SM_CXSMICON));wcscpy_s(tray.szTip,L"文件夹面板 · 后台运行中");Shell_NotifyIconW(NIM_ADD,&tray);}
static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){if(m==taskbarMessage){addTray();return 0;}switch(m){case WM_HOOK:{
 POINT pt{GET_X_LPARAM(l),GET_Y_LPARAM(l)};
 if(w==1){KillTimer(h,TIMER_CLICK);pendingFolder=false;pendingFolderPath.clear();downPoint=pt;}
 else if(w==3){KillTimer(h,TIMER_CLICK);clicks.reset();resetFolderState();}
 else if(w==2){
  KillTimer(h,TIMER_CLICK);
  if(!isDesktopPoint(pt)||abs(pt.x-downPoint.x)>GetSystemMetrics(SM_CXDRAG)||abs(pt.y-downPoint.y)>GetSystemMetrics(SM_CYDRAG)){
   clicks.reset();resetFolderState();return 0;
  }
  auto hit=desktopHitAt(pt);
  if(!hit.managed){clicks.reset();resetFolderState();return 0;}
  const auto now=GetTickCount64();
  if(hit.realFolder){
   clicks.reset();
   if(_wcsicmp(folderLastPath.c_str(),hit.path.c_str())){resetFolderState();folderLastPath=hit.path;}
   auto action=folderClicks.release(0,pt.x,pt.y,now,GetDoubleClickTime(),GetSystemMetrics(SM_CXDOUBLECLK),GetSystemMetrics(SM_CYDOUBLECLK));
   // Native Explorer handles real-folder double clicks; do not launch a second window.
   if(action==ClickAction::Open){resetFolderState();if(panelWindow)closePanel();return 0;}
   pendingFolder=true;pendingFolderPath=hit.path;pendingFolderPoint=pt;
   if(cfg.expandDelay==0){folderClicks.markPreview();pendingFolder=false;showPanel(hit.path,pt,true);}
   else SetTimer(h,TIMER_CLICK,previewDelay(cfg.expandDelay,GetDoubleClickTime()),nullptr);
  }else{
   resetFolderState();int id=hit.entryId;
   if(id<0||id>=(int)entries.size())return 0;
   if(id==lastOpen&&now-openedAt<GetDoubleClickTime())return 0;
   auto action=clicks.release(id,pt.x,pt.y,now,GetDoubleClickTime(),GetSystemMetrics(SM_CXDOUBLECLK),GetSystemMetrics(SM_CYDOUBLECLK));
   if(action==ClickAction::Open)dispatchOpen(id);
   else if(cfg.expandDelay==0){clicks.markPreview();showPanel(entries[id].target,pt,true);}
   else SetTimer(h,TIMER_CLICK,previewDelay(cfg.expandDelay,GetDoubleClickTime()),nullptr);
  }
 }
 return 0;
}
case WM_TIMER:if(w==31){if(EntryMaintenance::tick()){clicks.reset();resetFolderState();lastOpen=-1;HWND settings=FindWindowW(L"FolderIconCN.Settings",nullptr);if(settings)PostMessageW(settings,WM_CONFIG,0,0);}return 0;}if(w==TIMER_CLICK){KillTimer(h,TIMER_CLICK);if(pendingFolder){if(folderClicks.previewDue(GetTickCount64(),previewDelay(cfg.expandDelay,GetDoubleClickTime()))){folderClicks.markPreview();pendingFolder=false;showPanel(pendingFolderPath,pendingFolderPoint,true);}else SetTimer(h,TIMER_CLICK,10,nullptr);}else{int id=clicks.lastId;POINT pt{clicks.x,clicks.y};if(id>=0&&id<(int)entries.size()&&!clicks.previewed){if(clicks.previewDue(GetTickCount64(),previewDelay(cfg.expandDelay,GetDoubleClickTime()))){clicks.markPreview();showPanel(entries[id].target,pt,true);}else SetTimer(h,TIMER_CLICK,10,nullptr);}}}return 0;case WM_LAUNCH:dispatchOpen((int)w);return 0;case WM_CONFIG:{loadConfig();return 0;}case WM_TRAY:if(l==WM_LBUTTONDBLCLK){showSettings();return 0;}if(l==WM_RBUTTONUP||l==WM_CONTEXTMENU){HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"设置…");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,3,L"退出后台（本次）");POINT p;GetCursorPos(&p);SetForegroundWindow(h);int id=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,0,h,nullptr);DestroyMenu(menu);PostMessageW(h,WM_NULL,0,0);if(id==1)showSettings();if(id==3&&confirmStopBackground(h))PostMessageW(h,WM_CLOSE,0,0);}return 0;case WM_CLOSE:quitting=true;KillTimer(h,TIMER_CLICK);if(mouseHook){UnhookWindowsHookEx(mouseHook);mouseHook=nullptr;}Shell_NotifyIconW(NIM_DELETE,&tray);closePanel();if(settingsWindow)DestroyWindow(settingsWindow);DestroyWindow(h);return 0;case WM_DESTROY:PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR,int){instance=h;using DpiFn=BOOL(WINAPI*)(HANDLE);auto dpi=(DpiFn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetProcessDpiAwarenessContext");if(dpi)dpi((HANDLE)-4);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);ULONG_PTR gdip;Gdiplus::GdiplusStartupInput gi;Gdiplus::GdiplusStartup(&gdip,&gi,nullptr);wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);exePath=path;PathRemoveFileSpecW(path);baseDir=path;iniPath=baseDir+L"\\FolderIcon.ini";int argc;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::wstring command=argc>1?argv[1]:L"--settings",argument=argc>2?argv[2]:L"";loadConfig();int result=0;
 if(command==L"--diagnose"){diagnose(argument);goto done;}
 if(command==L"--stop"){if(HWND h=FindWindowW(CONTROL_CLASS,nullptr))PostMessageW(h,WM_CLOSE,0,0);goto done;}
 if(command==L"--resident"){singleton=CreateMutexW(nullptr,TRUE,L"Local\\FolderIconCN.Resident.v1");if(GetLastError()==ERROR_ALREADY_EXISTS)goto done;residentMode=true;WNDCLASSW wc{};wc.hInstance=h;wc.lpfnWndProc=proc;wc.lpszClassName=CONTROL_CLASS;RegisterClassW(&wc);controller=CreateWindowW(CONTROL_CLASS,L"FolderIcon中文监听",WS_OVERLAPPED,0,0,0,0,nullptr,nullptr,h,nullptr);taskbarMessage=RegisterWindowMessageW(L"TaskbarCreated");mouseHook=SetWindowsHookExW(WH_MOUSE_LL,hook,h,0);if(!mouseHook){MessageBoxW(nullptr,L"无法安装桌面点击监听。仍可通过已有入口按需打开面板。",L"文件夹面板",MB_ICONERROR);result=2;goto done;}EntryMaintenance::tick(true);SetTimer(controller,31,1500,nullptr);addTray();}
 else if(command==L"--settings"){showSettings();}
 else if(command==L"--entry"){int id=-1;for(size_t i=0;i<entries.size();i++)if(entries[i].id==argument)id=(int)i;if(id<0){result=2;goto done;}if(HWND other=FindWindowW(CONTROL_CLASS,nullptr)){PostMessageW(other,WM_LAUNCH,id,0);goto done;}POINT p;GetCursorPos(&p);showPanel(entries[id].target,p);}
 else if(command==L"--folder"){POINT p;GetCursorPos(&p);showPanel(argument,p);}
    else if(command==L"--context-folder"){
   std::wstring action=L"legacy",path;
   if(argc>=4){action=argv[2];path=argv[3];}else if(argc>=3)path=argv[2];else{MessageBoxW(nullptr,L"缺少右键目标文件夹路径。",L"FolderIcon",MB_OK|MB_ICONERROR);result=2;goto done;}
   result=handleContextFolder(action,path);goto done;
  }
 else{MessageBoxW(nullptr,L"参数：--settings、--resident、--entry work/coding/services、--folder 路径、--context-folder choose/create/convert 路径。",L"文件夹面板",MB_OK);goto done;}
 {MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(settingsWindow&&IsDialogMessageW(settingsWindow,&msg)){ensureSettingsFocusVisible();continue;}TranslateMessage(&msg);DispatchMessageW(&msg);}}
 done:EntryMaintenance::shutdown();if(mouseHook)UnhookWindowsHookEx(mouseHook);if(singleton)CloseHandle(singleton);if(argv)LocalFree(argv);Gdiplus::GdiplusShutdown(gdip);CoUninitialize();return result;
}