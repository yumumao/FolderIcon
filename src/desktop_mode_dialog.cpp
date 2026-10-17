#include "desktop_modes.h"
namespace DesktopModes {
namespace {
struct Dialog {std::wstring path;Info info;HFONT font=nullptr;bool changed=false;};
INT_PTR CALLBACK proc(HWND h,UINT msg,WPARAM w,LPARAM l){
 auto*d=(Dialog*)GetWindowLongPtrW(h,DWLP_USER);
 if(msg==WM_INITDIALOG){d=(Dialog*)l;SetWindowLongPtrW(h,DWLP_USER,l);SetWindowTextW(h,L"FolderIcon打开方式");
  UINT dpi=GetDpiForWindow(h);if(!dpi)dpi=96;auto px=[&](int n){return MulDiv(n,(int)dpi,96);};
  // Dialog units follow the system font; size the client and all controls in
  // the same DPI-scaled coordinate system instead of leaving a large blank tail.
  const int width=600;RECT bounds{0,0,px(width),px(515)};
  AdjustWindowRectExForDpi(&bounds,(DWORD)GetWindowLongPtrW(h,GWL_STYLE),FALSE,(DWORD)GetWindowLongPtrW(h,GWL_EXSTYLE),dpi);
  SetWindowPos(h,nullptr,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_NOMOVE|SWP_NOZORDER);
  d->font=CreateFontW(-px(17),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  auto make=[&](const wchar_t*c,const wchar_t*t,DWORD style,int x,int y,int cx,int cy,int id){HWND child=CreateWindowW(c,t,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(cx),px(cy),h,(HMENU)(INT_PTR)id,instance,nullptr);SendMessageW(child,WM_SETFONT,(WPARAM)d->font,TRUE);return child;};
  make(L"STATIC",L"为这个桌面项目选择打开方式",0,22,18,width-44,25,0);
  make(L"EDIT",d->path.c_str(),ES_READONLY|ES_AUTOHSCROLL|WS_BORDER,22,53,width-44,28,4010);
  make(L"BUTTON",L"展开面板",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,22,102,width-44,28,4001);
  make(L"STATIC",d->info.canPanel?L"单击就地展开文件夹内容，保留现有面板操作。":L"此快捷方式不指向文件夹，不能展开面板。",0,44,134,width-66,42,0);
  make(L"BUTTON",L"单击直接打开",BS_AUTORADIOBUTTON|WS_TABSTOP,22,183,width-44,28,4002);
  make(L"STATIC",L"按原设置打开目录或启动快捷方式，右上显示绿色箭头。",0,44,215,width-66,42,0);
  make(L"BUTTON",L"恢复默认",BS_AUTORADIOBUTTON|WS_TABSTOP,22,264,width-44,28,4000);
  make(L"STATIC",L"撤销本项目的打开方式设置，并恢复原图标。",0,44,296,width-66,32,0);
  make(L"STATIC",L"单击功能需要后台运行；退出后台后按系统原有方式打开。\n绿色项目松开即打开；拖动及Ctrl/Shift多选保持原有操作。",0,22,343,width-44,55,0);
  bool running=FindWindowW(CONTROL_CLASS,nullptr)!=nullptr;
  HWND start=make(L"BUTTON",L"保存后启动后台（本次）",BS_AUTOCHECKBOX|WS_TABSTOP,22,409,width-44,28,4020);SendMessageW(start,BM_SETCHECK,running?BST_UNCHECKED:BST_CHECKED,0);EnableWindow(start,!running);SetTimer(h,1,500,nullptr);
  make(L"BUTTON",L"保存",BS_DEFPUSHBUTTON|WS_TABSTOP,width-222,459,92,34,IDOK);make(L"BUTTON",L"取消",BS_PUSHBUTTON|WS_TABSTOP,width-114,459,92,34,IDCANCEL);
  EnableWindow(GetDlgItem(h,4001),d->info.canPanel);CheckRadioButton(h,4000,4002,4000+(int)d->info.mode);
  RECT box{};GetWindowRect(h,&box);POINT pt{};GetCursorPos(&pt);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST),&mi);SetWindowPos(h,nullptr,mi.rcWork.left+(mi.rcWork.right-mi.rcWork.left-(box.right-box.left))/2,mi.rcWork.top+(mi.rcWork.bottom-mi.rcWork.top-(box.bottom-box.top))/2,0,0,SWP_NOSIZE|SWP_NOZORDER);return TRUE;
 }
 if(msg==WM_TIMER&&w==1){HWND start=GetDlgItem(h,4020);if(!FindWindowW(CONTROL_CLASS,nullptr)&&!IsWindowEnabled(start)){EnableWindow(start,TRUE);SendMessageW(start,BM_SETCHECK,BST_CHECKED,0);}return TRUE;}
 if(msg==WM_COMMAND){int id=LOWORD(w);if(id==IDCANCEL){EndDialog(h,IDCANCEL);return TRUE;}if(id==IDOK&&d){Mode mode=IsDlgButtonChecked(h,4002)==BST_CHECKED?Mode::Direct:IsDlgButtonChecked(h,4001)==BST_CHECKED?Mode::Panel:Mode::Native;std::wstring error;
  if(!set(d->path,mode,error)){MessageBoxW(h,error.c_str(),L"打开方式未保存",MB_OK|MB_ICONWARNING);return TRUE;}
  if(HWND c=FindWindowW(CONTROL_CLASS,nullptr))PostMessageW(c,WM_CONFIG,0,0);else if(mode!=Mode::Native&&IsDlgButtonChecked(h,4020)==BST_CHECKED)launchSelf(L"--resident");
  if(HWND s=FindWindowW(L"FolderIconCN.Settings",nullptr))PostMessageW(s,WM_CONFIG,0,0);d->changed=true;EndDialog(h,IDOK);return TRUE;
 }}
 if(msg==WM_CLOSE){EndDialog(h,IDCANCEL);return TRUE;}return FALSE;
}
}
bool showDialog(HWND owner,const std::wstring&path){loadConfig();load();Dialog d;d.path=path;d.info=inspect(path);if(!d.info.supported){MessageBoxW(owner,d.info.error.c_str(),L"FolderIcon打开方式",MB_OK|MB_ICONINFORMATION);return false;}
 alignas(DWORD) WORD raw[12]{};auto*t=(DLGTEMPLATE*)raw;t->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;t->cx=350;t->cy=325;
 INT_PTR result=DialogBoxIndirectParamW(instance,t,owner,proc,(LPARAM)&d);if(d.font)DeleteObject(d.font);if(result==-1)MessageBoxW(owner,L"无法创建打开方式窗口。",L"FolderIcon",MB_OK|MB_ICONERROR);return d.changed;
}
}
