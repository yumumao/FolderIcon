#include "entry_manager.h"
#include "desktop_modes.h"
#include "desktop_scope.h"
#include "entry_actions.h"
#include "entry_maintenance.h"
#include "folder_badge.h"
namespace {
struct Row {std::wstring id,path,label;bool builtin=false;};
struct Manager {HFONT font=nullptr;int tab=0;std::vector<Row> rows;};
std::wstring entryId(const std::wstring&path){
 IShellLinkW*link=nullptr;IPersistFile*file=nullptr;std::wstring id;
 if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link)))&&SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))&&SUCCEEDED(file->Load(path.c_str(),STGM_READ))){
  wchar_t target[32768]{},args[32768]{};link->GetPath(target,32768,nullptr,SLGP_RAWPATH);link->GetArguments(args,32768);
  if(DesktopModes::samePath(target,exePath))for(const auto&e:entries)if(std::wstring(args)==L"--entry "+e.id){id=e.id;break;}
 }
 if(file)file->Release();if(link)link->Release();return id;
}
bool hasPlainEntry(const Entry&e){
 std::wstring root,error;if(e.desktop.empty()){if(!DesktopScope::resolve(cfg.desktopPath,root,error))return true;}else root=e.desktop;
 WIN32_FIND_DATAW data{};HANDLE files=FindFirstFileW((root+L"\\*.lnk").c_str(),&data);
 if(files==INVALID_HANDLE_VALUE)return GetLastError()!=ERROR_FILE_NOT_FOUND;
 bool plain=false;do{if(data.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))continue;auto path=root+L"\\"+data.cFileName;
  auto mode=DesktopModes::find(path);if((!mode||mode->mode==DesktopModes::Mode::Native)&&entryId(path)==e.id){plain=true;break;}
 }while(FindNextFileW(files,&data));FindClose(files);return plain;
}
void notify(){if(HWND h=FindWindowW(CONTROL_CLASS,nullptr))PostMessageW(h,WM_CONFIG,0,0);if(HWND h=FindWindowW(L"FolderIconCN.Settings",nullptr))PostMessageW(h,WM_CONFIG,0,0);}
void status(HWND h,const std::wstring&text){SetDlgItemTextW(h,5004,text.c_str());}
void selection(HWND h,Manager&d){
 int i=(int)SendDlgItemMessageW(h,5001,LB_GETCURSEL,0,0);bool valid=i>=0&&i<(int)d.rows.size();
 SetDlgItemTextW(h,5003,valid?d.rows[i].path.c_str():L"");
 EnableWindow(GetDlgItem(h,5012),valid&&!d.rows[i].builtin);EnableWindow(GetDlgItem(h,5013),valid);EnableWindow(GetDlgItem(h,5014),valid&&d.rows[i].id.empty());
}
void refresh(HWND h,Manager&d,const std::wstring&selected=L""){
 loadConfig();d.rows.clear();
 if(d.tab==0){
  std::vector<std::wstring> overridden;
  for(const auto&r:DesktopModes::records)if(!r.folder&&r.mode!=DesktopModes::Mode::Native){auto id=entryId(r.path);if(!id.empty())overridden.push_back(id);}
  for(size_t i=0;i<entries.size();++i){auto&e=entries[i];if(std::find(overridden.begin(),overridden.end(),e.id)!=overridden.end()&&!hasPlainEntry(e))continue;
   d.rows.push_back({e.id,e.target,(i<3?L"[内置] ":L"[桌面入口] ")+e.name+L"  →  "+e.target,i<3});}
  for(const auto&r:DesktopModes::records)if(!r.folder&&r.mode==DesktopModes::Mode::Panel)d.rows.push_back({L"",r.path,L"[原有快捷方式] "+std::wstring(PathFindFileNameW(r.path.c_str()))});
 }else if(d.tab==1){
  for(const auto&r:folderRules)if(r.desktop&&!DesktopModes::direct(r.path))d.rows.push_back({L"",r.path,PathFindFileNameW(r.path.c_str())});
 }else for(const auto&r:DesktopModes::records)if(r.mode==DesktopModes::Mode::Direct||r.pending==(int)DesktopModes::Mode::Direct)d.rows.push_back({L"",r.path,(r.folder?L"[文件夹] ":L"[快捷方式] ")+std::wstring(PathFindFileNameW(r.path.c_str()))+(r.pending>=0?L"  [待完成] ":L"")});
 SendDlgItemMessageW(h,5001,LB_RESETCONTENT,0,0);int choose=-1;for(size_t i=0;i<d.rows.size();++i){auto&r=d.rows[i];SendDlgItemMessageW(h,5001,LB_ADDSTRING,0,(LPARAM)r.label.c_str());if(r.path==selected)choose=(int)i;}
 if(choose<0&&!d.rows.empty())choose=0;SendDlgItemMessageW(h,5001,LB_SETCURSEL,choose,0);
 const wchar_t*desc[]={L"蓝色角标 · 为其他文件夹创建桌面面板快捷方式，或添加已有的文件夹快捷方式。",L"橙色角标 · 只管理桌面第一层的真实文件夹，单击原位展开面板。",L"绿色箭头 · 桌面文件夹或快捷方式松开即打开；拖动和Ctrl/Shift多选保持原样。"};
 SetDlgItemTextW(h,5002,desc[d.tab]);SetDlgItemTextW(h,5010,d.tab==0?L"添加文件夹…":d.tab==1?L"添加桌面文件夹…":L"添加桌面项目…");
 ShowWindow(GetDlgItem(h,5011),d.tab==0?SW_SHOW:SW_HIDE);
 SetDlgItemTextW(h,5005,d.tab==0?L"自建入口按组管理；同组有单独打开方式时先恢复默认再删。原目录不删，内置入口保留。":L"删除选中只撤销本工具设置并恢复原图标，保留真实文件夹和快捷方式。");
 selection(h,d);
}
HWND control(HWND h,HFONT f,UINT dpi,const wchar_t*cls,const wchar_t*text,DWORD style,int x,int y,int w,int height,int id){
 auto px=[&](int n){return MulDiv(n,dpi,96);};HWND c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(w),px(height),h,(HMENU)(INT_PTR)id,instance,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)f,TRUE);return c;
}
void sizeDialog(HWND h,int width,int height){UINT dpi=GetDpiForWindow(h);RECT r{0,0,MulDiv(width,dpi,96),MulDiv(height,dpi,96)};AdjustWindowRectExForDpi(&r,(DWORD)GetWindowLongPtrW(h,GWL_STYLE),FALSE,(DWORD)GetWindowLongPtrW(h,GWL_EXSTYLE),dpi);RECT parent{};GetWindowRect(GetParent(h),&parent);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&mi);int x=parent.left+(parent.right-parent.left-(r.right-r.left))/2,y=parent.top+(parent.bottom-parent.top-(r.bottom-r.top))/2;x=std::max((int)mi.rcWork.left,std::min(x,(int)(mi.rcWork.right-(r.right-r.left))));y=std::max((int)mi.rcWork.top,std::min(y,(int)(mi.rcWork.bottom-(r.bottom-r.top))));SetWindowPos(h,nullptr,x,y,r.right-r.left,r.bottom-r.top,SWP_NOZORDER);}
struct Picker {HFONT font;int tab;std::wstring root,selected;std::vector<std::wstring> paths;};
INT_PTR CALLBACK pickerProc(HWND h,UINT m,WPARAM w,LPARAM l){
 auto*d=(Picker*)GetWindowLongPtrW(h,DWLP_USER);
 if(m==WM_INITDIALOG){d=(Picker*)l;SetWindowLongPtrW(h,DWLP_USER,l);SetWindowTextW(h,L"选择桌面项目");sizeDialog(h,720,460);UINT dpi=GetDpiForWindow(h);
  auto make=[&](const wchar_t*c,const wchar_t*t,DWORD s,int x,int y,int cw,int ch,int id){return control(h,d->font,dpi,c,t,s,x,y,cw,ch,id);};
  make(L"STATIC",d->tab==1?L"仅列出当前桌面第一层的真实文件夹。":d->tab==0?L"选择已有的文件夹快捷方式，在原位置加蓝色角标。":L"选择桌面上的文件夹或.lnk快捷方式，启用单击直接打开。",0,20,16,680,42,0);
  make(L"EDIT",d->root.c_str(),ES_READONLY|ES_AUTOHSCROLL|WS_BORDER,20,62,680,28,5102);
  HWND list=make(L"LISTBOX",L"",WS_BORDER|WS_VSCROLL|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY|WS_TABSTOP,20,102,680,285,5101);
  WIN32_FIND_DATAW fd{};HANDLE search=FindFirstFileW((d->root+L"\\*").c_str(),&fd);
  if(search!=INVALID_HANDLE_VALUE){do{if(!wcscmp(fd.cFileName,L".")||!wcscmp(fd.cFileName,L"..")||(fd.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))continue;bool folder=(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0;if(d->tab==1&&!folder)continue;if(d->tab==0&&folder)continue;if(!folder&&_wcsicmp(PathFindExtensionW(fd.cFileName),L".lnk"))continue;auto path=d->root+L"\\"+fd.cFileName;auto info=DesktopModes::inspect(path);if(!info.supported||(d->tab==0&&(!info.canPanel||info.managedEntry)))continue;d->paths.push_back(path);}while(FindNextFileW(search,&fd));FindClose(search);}
  std::sort(d->paths.begin(),d->paths.end(),[](const auto&a,const auto&b){return _wcsicmp(a.c_str(),b.c_str())<0;});for(const auto&p:d->paths)SendMessageW(list,LB_ADDSTRING,0,(LPARAM)PathFindFileNameW(p.c_str()));
  if(!d->paths.empty())SendMessageW(list,LB_SETCURSEL,0,0);else make(L"STATIC",L"当前桌面没有符合条件的项目。",0,20,406,430,32,0);
  make(L"BUTTON",L"添加选中",BS_DEFPUSHBUTTON|WS_TABSTOP,478,406,104,32,IDOK);EnableWindow(GetDlgItem(h,IDOK),!d->paths.empty());make(L"BUTTON",L"取消",BS_PUSHBUTTON|WS_TABSTOP,594,406,104,32,IDCANCEL);return TRUE;
 }
 if(m==WM_COMMAND){int id=LOWORD(w);if(id==IDCANCEL){EndDialog(h,IDCANCEL);return TRUE;}if(id==IDOK||(id==5101&&HIWORD(w)==LBN_DBLCLK)){int i=(int)SendDlgItemMessageW(h,5101,LB_GETCURSEL,0,0);if(i>=0&&i<(int)d->paths.size()){d->selected=d->paths[i];EndDialog(h,IDOK);}return TRUE;}}
 if(m==WM_CLOSE){EndDialog(h,IDCANCEL);return TRUE;}return FALSE;
}
bool chooseDesktop(HWND h,Manager&d,std::wstring&out){loadConfig();std::wstring root,error;if(!DesktopScope::resolve(cfg.desktopPath,root,error)){status(h,error);return false;}Picker picker{d.font,d.tab,root};alignas(DWORD) WORD raw[12]{};auto*t=(DLGTEMPLATE*)raw;t->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;t->cx=400;t->cy=280;auto result=DialogBoxIndirectParamW(instance,t,h,pickerProc,(LPARAM)&picker);out=picker.selected;return result==IDOK&&!out.empty();}
bool chooseFolder(HWND owner,std::wstring&out){IFileDialog*dialog=nullptr;if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))return false;DWORD opts;dialog->GetOptions(&opts);dialog->SetOptions(opts|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM);dialog->SetTitle(L"选择文件夹，在桌面创建蓝色面板快捷方式");if(SUCCEEDED(dialog->Show(owner))){IShellItem*item=nullptr;if(SUCCEEDED(dialog->GetResult(&item))){PWSTR p=nullptr;if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&p))){out=p;CoTaskMemFree(p);}item->Release();}}dialog->Release();return !out.empty();}
INT_PTR CALLBACK managerProc(HWND h,UINT m,WPARAM w,LPARAM l){
 auto*d=(Manager*)GetWindowLongPtrW(h,DWLP_USER);
 if(m==WM_INITDIALOG){d=(Manager*)l;SetWindowLongPtrW(h,DWLP_USER,l);SetWindowTextW(h,L"桌面入口管理");sizeDialog(h,820,580);UINT dpi=GetDpiForWindow(h);d->font=CreateFontW(-MulDiv(17,dpi,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  auto make=[&](const wchar_t*c,const wchar_t*t,DWORD s,int x,int y,int cw,int ch,int id){return control(h,d->font,dpi,c,t,s,x,y,cw,ch,id);};
  make(L"STATIC",L"桌面入口管理",0,20,16,500,28,0);make(L"STATIC",L"操作即时生效；主设置中的取消不会撤销这里的操作。",0,20,51,780,28,0);
  HWND tabs=make(WC_TABCONTROLW,L"",WS_TABSTOP,20,90,780,40,5000);const wchar_t*names[]={L"蓝色 · 面板快捷方式",L"橙色 · 原位展开",L"绿色 · 单击打开"};for(int i=0;i<3;++i){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=(wchar_t*)names[i];SendMessageW(tabs,TCM_INSERTITEMW,i,(LPARAM)&item);}
  make(L"STATIC",L"",0,20,143,780,42,5002);make(L"LISTBOX",L"",WS_BORDER|WS_VSCROLL|WS_HSCROLL|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY|WS_TABSTOP,20,190,780,205,5001);SendDlgItemMessageW(h,5001,LB_SETHORIZONTALEXTENT,MulDiv(1400,dpi,96),0);
  make(L"EDIT",L"",ES_READONLY|ES_AUTOHSCROLL|WS_BORDER|WS_TABSTOP,20,405,780,28,5003);
  make(L"BUTTON",L"添加文件夹…",BS_PUSHBUTTON|WS_TABSTOP,20,447,158,32,5010);make(L"BUTTON",L"添加桌面快捷方式…",BS_PUSHBUTTON|WS_TABSTOP,188,447,174,32,5011);
  make(L"BUTTON",L"删除选中",BS_PUSHBUTTON|WS_TABSTOP,372,447,112,32,5012);make(L"BUTTON",L"重加角标",BS_PUSHBUTTON|WS_TABSTOP,494,447,112,32,5013);make(L"BUTTON",L"更改打开方式…",BS_PUSHBUTTON|WS_TABSTOP,616,447,184,32,5014);
  make(L"STATIC",L"",0,20,490,780,40,5005);make(L"STATIC",L"",0,20,537,540,32,5004);make(L"BUTTON",L"刷新",BS_PUSHBUTTON|WS_TABSTOP,574,536,104,32,5015);make(L"BUTTON",L"关闭",BS_DEFPUSHBUTTON|WS_TABSTOP,690,536,110,32,IDCANCEL);refresh(h,*d);return TRUE;
 }
 if(m==WM_NOTIFY&&((NMHDR*)l)->idFrom==5000&&((NMHDR*)l)->code==TCN_SELCHANGE){d->tab=TabCtrl_GetCurSel(GetDlgItem(h,5000));refresh(h,*d);status(h,L"");return TRUE;}
 if(m==WM_CONFIG){refresh(h,*d);return TRUE;}
 if(m==WM_COMMAND&&d){int id=LOWORD(w);if(id==IDCANCEL){EndDialog(h,IDOK);return TRUE;}if(id==5001){selection(h,*d);return TRUE;}if(id==5015){refresh(h,*d);status(h,L"名单已刷新。");return TRUE;}
  if(id==5010||id==5011){std::wstring path,error,output;bool ok=false;
   if(d->tab==0&&id==5010){if(!chooseFolder(h,path))return TRUE;std::wstring root;if(DesktopScope::resolve(cfg.desktopPath,root,error))ok=EntryActions::createDesktopEntry(path,root,output,error);}
   else{if(!chooseDesktop(h,*d,path))return TRUE;ok=DesktopModes::set(path,d->tab==2?DesktopModes::Mode::Direct:DesktopModes::Mode::Panel,error);}
   refresh(h,*d,path);if(ok)notify();status(h,ok?L"已添加并应用角标。":L"操作未完成。");if(!ok)MessageBoxW(h,error.c_str(),L"未完成",MB_OK|MB_ICONWARNING);return TRUE;
  }
  if(id==5012||id==5013||id==5014){int i=(int)SendDlgItemMessageW(h,5001,LB_GETCURSEL,0,0);if(i<0||i>=(int)d->rows.size())return TRUE;Row row=d->rows[i];std::wstring error,warning;bool ok=false;
   if(id==5014){DesktopModes::showDialog(h,row.path);refresh(h,*d,row.path);return TRUE;}
   if(id==5012){if(row.builtin)return TRUE;auto text=row.id.empty()?L"撤销所选项目的打开方式并恢复原图标？\n保留文件夹、快捷方式和其中的文件。":L"删除本工具生成的桌面快捷方式及入口配置？\n保留原始文件夹和其中的文件。";if(MessageBoxW(h,text,L"删除选中",MB_YESNO|MB_ICONQUESTION)!=IDYES)return TRUE;}
   loadConfig();if(!row.id.empty()){size_t n=0;for(;n<entries.size();++n)if(entries[n].id==row.id)break;if(n==entries.size())error=L"入口已变化，请刷新。";else if(id==5012)ok=EntryActions::removeDesktopEntry(n,warning,error);else{size_t updated=0;ok=EntryMaintenance::repairShortcutBadge(n,updated,error);}}
   else if(id==5012)ok=DesktopModes::set(row.path,DesktopModes::Mode::Native,error);else ok=DesktopModes::repairBadge(row.path,error);
   refresh(h,*d,row.path);if(ok)notify();status(h,ok?L"操作已完成。":L"操作未完成。");if(!ok||!warning.empty())MessageBoxW(h,(ok?warning:error).c_str(),L"操作结果",MB_OK|MB_ICONWARNING);return TRUE;
  }
 }
 if(m==WM_CLOSE){EndDialog(h,IDOK);return TRUE;}return FALSE;
}
}
void showEntryManager(HWND owner){INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_TAB_CLASSES};InitCommonControlsEx(&controls);Manager d;alignas(DWORD) WORD raw[12]{};auto*t=(DLGTEMPLATE*)raw;t->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;t->cx=470;t->cy=350;auto result=DialogBoxIndirectParamW(instance,t,owner,managerProc,(LPARAM)&d);if(d.font)DeleteObject(d.font);if(result==-1)MessageBoxW(owner,L"无法创建入口管理窗口。",L"FolderIcon",MB_OK|MB_ICONERROR);}
