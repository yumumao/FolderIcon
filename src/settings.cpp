#include "app.h"
#include "icon_cache.h"
#include "folder_badge.h"
#include "entry_actions.h"
#include "entry_maintenance.h"
#include "context_menu.h"
#include "desktop_scope.h"
#include "desktop_modes.h"
#include "entry_manager.h"
HWND settingsWindow=nullptr;
static HFONT font,helpFont;static HBRUSH helpBrush;static HWND backgroundHelp,backgroundHelpTitle,backgroundHelpBody;static HWND fields[9],follow,followFont,autoStart,backgroundStatus,backgroundToggle,contextMenu,dotHidden,showHidden;
static HWND titleLabel,metricLabels[9],backgroundNote,cacheNote,delayNote;
static COLORREF chosen;static FolderRules::Rules draftRules,draftBaseRules;
static HWND desktopLabel,desktopLocation,desktopNote;
static std::wstring draftDesktopPath,baseDesktopPath;
static constexpr int settingsHeight=960;
static int settingsScroll=0;
static void refreshDesktopLocation();
static constexpr wchar_t backgroundHelpText[]=L"蓝色角标：快捷方式\n专用入口双击展开，普通链接照常。\n\n橙色角标：真实桌面文件夹\n双击用资源管理器打开。\n\n绿色箭头：单击直接打开\n恢复系统原有点击方式。\n\n退出仅停本次后台，保留启动设置。";
bool confirmStopBackground(HWND owner){
 return MessageBoxW(owner,(std::wstring(L"确定退出后台？\n\n")+backgroundHelpText).c_str(),L"确认退出后台",MB_OKCANCEL|MB_ICONINFORMATION|MB_DEFBUTTON2|MB_SETFOREGROUND)==IDOK;
}
static void refreshBackgroundState(){
 bool running=FindWindowW(CONTROL_CLASS,nullptr)!=nullptr;
 SetWindowTextW(backgroundStatus,running?L"后台运行中":L"后台已停止");
 SetWindowTextW(backgroundToggle,running?L"退出后台":L"启动后台");
 EnableWindow(backgroundToggle,TRUE);
}
static void scrollSettings(int requested){
 RECT rc{};GetClientRect(settingsWindow,&rc);int next=clampValue(requested,0,std::max(0,settingsHeight-(int)rc.bottom)),delta=settingsScroll-next;
 settingsScroll=next;
 if(delta)EnumChildWindows(settingsWindow,[](HWND child,LPARAM offset)->BOOL{
  if(GetParent(child)!=settingsWindow)return TRUE;
  RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,settingsWindow,(POINT*)&r,2);
  SetWindowPos(child,nullptr,r.left,r.top+(int)offset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);return TRUE;
 },delta);
 SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS,0,settingsHeight-1,(UINT)rc.bottom,next,0};SetScrollInfo(settingsWindow,SB_VERT,&info,TRUE);
}
void ensureSettingsFocusVisible(){
 HWND focus=GetFocus();if(!settingsWindow||!focus||!IsChild(settingsWindow,focus))return;
 RECT rc{},view{};GetWindowRect(focus,&rc);MapWindowPoints(nullptr,settingsWindow,(POINT*)&rc,2);GetClientRect(settingsWindow,&view);
 if(rc.top<8)scrollSettings(settingsScroll+rc.top-8);
 else if(rc.bottom>view.bottom-8)scrollSettings(settingsScroll+rc.bottom-view.bottom+8);
}
static void placeControl(HWND c,int x,int y,int w,int h){if(c)SetWindowPos(c,nullptr,x,y-settingsScroll,w,h,SWP_NOZORDER|SWP_NOACTIVATE);}
static void layoutSettings(int clientWidth){
 int width=std::max(840,clientWidth),margin=24,gap=24,available=width-2*margin;
 int column=(available-gap)/2,left=margin,right=left+column+gap;
 int fieldWidth=124,fieldX=left+column-fieldWidth,labelWidth=std::max(120,fieldX-left-10);
 placeControl(titleLabel,left,16,width-2*margin,28);placeControl(follow,left,52,width-2*margin,25);placeControl(followFont,left,82,width-2*margin,25);
 for(int i=0;i<9;i++){placeControl(metricLabels[i],left,114+i*30,labelWidth,26);placeControl(fields[i],fieldX,110+i*30,fieldWidth,28);}
 placeControl(backgroundHelp,right,110,column,300);placeControl(backgroundHelpTitle,right+16,126,column-32,28);placeControl(backgroundHelpBody,right+16,166,column-32,230);
 placeControl(GetDlgItem(settingsWindow,120),left,388,200,32);placeControl(backgroundNote,left+210,394,std::max(100,column-210),28);
 placeControl(autoStart,left,432,column,28);placeControl(backgroundStatus,right,432,column-170,28);placeControl(backgroundToggle,right+column-160,428,160,32);
 placeControl(contextMenu,left,468,width-2*margin,25);placeControl(dotHidden,left,498,width-2*margin,25);placeControl(showHidden,left,528,width-2*margin,25);
 placeControl(desktopLabel,left,560,available,24);placeControl(desktopLocation,left,586,available,28);
 placeControl(GetDlgItem(settingsWindow,220),left,622,160,30);placeControl(GetDlgItem(settingsWindow,221),left+170,622,160,30);placeControl(desktopNote,left+342,622,available-342,44);
 placeControl(GetDlgItem(settingsWindow,230),left,682,270,38);placeControl(GetDlgItem(settingsWindow,231),left+286,684,available-286,42);
 placeControl(GetDlgItem(settingsWindow,125),left,740,155,32);placeControl(cacheNote,left+166,743,available-166,32);
 placeControl(GetDlgItem(settingsWindow,214),left,782,200,32);placeControl(GetDlgItem(settingsWindow,216),left+212,782,available-212,40);
 placeControl(delayNote,left,832,available,60);
 int cancelX=width-margin-112,saveX=cancelX-12-135;placeControl(GetDlgItem(settingsWindow,130),saveX,905,135,36);placeControl(GetDlgItem(settingsWindow,131),cancelX,905,112,36);
}
static const wchar_t* labels[]={L"图标大小（像素）",L"横向网格（像素）",L"纵向网格（像素）",L"每行图标数量",L"背景不透明度（10～100％）",L"圆角大小（0为直角）",L"文件名字号（8～36磅）",L"图标与文字间距（0～24像素）",L"面板展开等待（0～500毫秒）"};
static HWND control(const wchar_t* cls,const wchar_t* label,DWORD style,int x,int y,int w,int h,int id){HWND c=CreateWindowExW(!wcscmp(cls,L"EDIT")?WS_EX_CLIENTEDGE:0,cls,label,WS_CHILD|WS_VISIBLE|style,x,y,w,h,settingsWindow,(HMENU)(INT_PTR)id,instance,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;}
static void enableMetrics(){BOOL on=SendMessageW(follow,BM_GETCHECK,0,0)!=BST_CHECKED;for(int i=0;i<3;i++)EnableWindow(fields[i],on);EnableWindow(fields[6],SendMessageW(followFont,BM_GETCHECK,0,0)!=BST_CHECKED);}
static bool ruleFlags(const FolderRules::Rules& rules,const std::wstring& path,bool& panel,bool& desktop){
 panel=false;desktop=false;auto normalized=FolderRules::normalize(path);if(normalized.empty())return false;
 for(const auto& r:rules)if(!_wcsicmp(r.path.c_str(),normalized.c_str())){panel=r.panel;desktop=r.desktop;return true;}return false;
}
static void mergeExternalRulesIntoDraft(){
 auto beforeDraft=draftRules,beforeBase=draftBaseRules;bool dirty=false;
 if(beforeDraft.size()!=beforeBase.size())dirty=true;else for(const auto& r:beforeBase){bool bp=false,bd=false,dp=false,dd=false;ruleFlags(beforeBase,r.path,bp,bd);ruleFlags(beforeDraft,r.path,dp,dd);if(bp!=dp||bd!=dd){dirty=true;break;}}
 loadConfig();
 draftRules=folderRules;
 if(dirty){
  std::vector<std::wstring> paths;for(const auto& r:beforeBase)paths.push_back(r.path);for(const auto& r:beforeDraft)paths.push_back(r.path);
  for(const auto& path:paths){bool bp=false,bd=false,dp=false,dd=false;ruleFlags(beforeBase,path,bp,bd);ruleFlags(beforeDraft,path,dp,dd);if(bp!=dp||bd!=dd)FolderRules::set(draftRules,path,dp,dd);}
 }
 draftBaseRules=folderRules;
 if(draftDesktopPath==baseDesktopPath)draftDesktopPath=cfg.desktopPath;
 baseDesktopPath=cfg.desktopPath;refreshDesktopLocation();
}
static bool chooseFolder(HWND owner,std::wstring& out,const std::wstring& initial=L"",const wchar_t* title=L"请选择一个真实文件夹");
static void refreshDesktopLocation(){
 std::wstring root,error;bool ok=DesktopScope::resolve(draftDesktopPath,root,error);
 auto label=draftDesktopPath.empty()?L"桌面位置（系统自动识别）":L"桌面位置（手动指定）";SetWindowTextW(desktopLabel,label);
 auto text=ok?root:error;SetWindowTextW(desktopLocation,text.c_str());
}
static void chooseDesktopLocation(HWND owner,bool automatic){
 std::wstring next,root,error;
 if(!automatic&&!chooseFolder(owner,next,draftDesktopPath,L"指定FolderIcon使用的桌面位置（不移动文件）"))return;
 if(!DesktopScope::resolve(next,root,error)){MessageBoxW(owner,error.c_str(),L"桌面位置不可用",MB_OK|MB_ICONWARNING);return;}
 draftDesktopPath=next;refreshDesktopLocation();
}
static bool chooseFolder(HWND owner,std::wstring& out,const std::wstring& initial,const wchar_t* title){out.clear();IFileDialog* dialog=nullptr;if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))return false;DWORD options=0;dialog->GetOptions(&options);dialog->SetOptions(options|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM);dialog->SetTitle(title);IShellItem* start=nullptr;if(!initial.empty()&&SUCCEEDED(SHCreateItemFromParsingName(initial.c_str(),nullptr,IID_PPV_ARGS(&start)))){dialog->SetFolder(start);start->Release();}IShellItem* computer=nullptr;if(SUCCEEDED(SHCreateItemInKnownFolder(FOLDERID_ComputerFolder,0,nullptr,IID_PPV_ARGS(&computer)))){dialog->SetDefaultFolder(computer);computer->Release();}HRESULT hr=dialog->Show(owner);if(SUCCEEDED(hr)){IShellItem* item=nullptr;if(SUCCEEDED(dialog->GetResult(&item))){PWSTR path=nullptr;if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&path))){out=FolderRules::normalize(path);CoTaskMemFree(path);}item->Release();}}dialog->Release();return !out.empty()&&EntryActions::realFolder(out);}
static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:{
  settingsWindow=h;settingsScroll=0;font=CreateFontW(-17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,DEFAULT_QUALITY,0,L"Microsoft YaHei UI");chosen=cfg.background;draftRules=folderRules;draftBaseRules=folderRules;draftDesktopPath=cfg.desktopPath;baseDesktopPath=cfg.desktopPath;
  SendMessageW(h,WM_SETICON,ICON_BIG,(LPARAM)applicationIcon(GetSystemMetrics(SM_CXICON)));SendMessageW(h,WM_SETICON,ICON_SMALL,(LPARAM)applicationIcon(GetSystemMetrics(SM_CXSMICON)));
  helpFont=CreateFontW(-18,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,DEFAULT_QUALITY,0,L"Microsoft YaHei UI");helpBrush=CreateSolidBrush(RGB(234,244,255));
  backgroundHelp=control(L"STATIC",L"",WS_CLIPSIBLINGS,432,110,384,300,180);SetWindowPos(backgroundHelp,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  backgroundHelpTitle=control(L"STATIC",L"退出后台后，仍可按需使用",SS_NOPREFIX,448,126,352,28,181);SendMessageW(backgroundHelpTitle,WM_SETFONT,(WPARAM)(helpFont?helpFont:font),TRUE);
  backgroundHelpBody=control(L"STATIC",backgroundHelpText,SS_NOPREFIX,448,166,352,230,182);
  titleLabel=control(L"STATIC",L"文件夹面板 · 外观与运行方式",0,24,16,620,28,0);
  follow=control(L"BUTTON",L"图标大小和网格跟随当前桌面",BS_AUTOCHECKBOX|WS_TABSTOP,24,52,560,25,101);SendMessageW(follow,BM_SETCHECK,cfg.followDesktop?BST_CHECKED:BST_UNCHECKED,0);
  followFont=control(L"BUTTON",L"文件名字体和字号跟随桌面（可独立取消）",BS_AUTOCHECKBOX|WS_TABSTOP,24,82,560,25,102);SendMessageW(followFont,BM_SETCHECK,cfg.followFont?BST_CHECKED:BST_UNCHECKED,0);
  int vals[]={cfg.icon,cfg.cellW,cfg.cellH,cfg.columns,cfg.opacity,cfg.radius,cfg.fontSize,cfg.textGap,cfg.expandDelay};for(int i=0;i<9;i++){metricLabels[i]=control(L"STATIC",labels[i],0,24,114+i*30,330,26,0);fields[i]=control(L"EDIT",std::to_wstring(vals[i]).c_str(),ES_NUMBER|WS_TABSTOP,370,110+i*30,128,28,110+i);}
  control(L"BUTTON",L"选择背景颜色…",BS_PUSHBUTTON|WS_TABSTOP,24,388,200,32,120);backgroundNote=control(L"STATIC",L"单层半透明（不实时磨砂）",0,240,394,275,28,121);
  autoStart=control(L"BUTTON",L"开机静默启动（内置启动项）",BS_AUTOCHECKBOX|WS_TABSTOP,24,432,380,28,122);SendMessageW(autoStart,BM_SETCHECK,cfg.autoStart?BST_CHECKED:BST_UNCHECKED,0);
  backgroundStatus=control(L"STATIC",L"",0,432,432,180,28,127);backgroundToggle=control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,640,428,160,32,128);refreshBackgroundState();
  contextMenu=control(L"BUTTON",L"在文件夹和快捷方式右键菜单中添加“FolderIcon打开方式…”",BS_AUTOCHECKBOX|WS_TABSTOP,24,468,620,25,215);SendMessageW(contextMenu,BM_SETCHECK,cfg.contextMenu?BST_CHECKED:BST_UNCHECKED,0);dotHidden=control(L"BUTTON",L"将点号开头的文件和文件夹视为隐藏项",BS_AUTOCHECKBOX|WS_TABSTOP,24,498,580,25,123);SendMessageW(dotHidden,BM_SETCHECK,cfg.dotHidden?BST_CHECKED:BST_UNCHECKED,0);
  showHidden=control(L"BUTTON",L"显示隐藏文件及文件夹（含系统隐藏项）",BS_AUTOCHECKBOX|WS_TABSTOP,24,528,580,25,124);SendMessageW(showHidden,BM_SETCHECK,cfg.showHidden?BST_CHECKED:BST_UNCHECKED,0);
  desktopLabel=control(L"STATIC",L"桌面位置",0,24,560,790,24,0);
  desktopLocation=control(L"EDIT",L"",ES_READONLY|ES_AUTOHSCROLL|WS_TABSTOP,24,586,790,28,222);
  control(L"BUTTON",L"指定桌面位置…",BS_PUSHBUTTON|WS_TABSTOP,24,622,160,30,220);control(L"BUTTON",L"恢复自动识别",BS_PUSHBUTTON|WS_TABSTOP,194,622,160,30,221);
  desktopNote=control(L"STATIC",L"保存后生效；只供本工具使用，不移动文件或修改系统桌面。",0,366,622,448,44,0);refreshDesktopLocation();
  control(L"BUTTON",L"管理桌面入口…",BS_PUSHBUTTON|WS_TABSTOP,24,682,270,38,230);
  control(L"STATIC",L"蓝色面板快捷方式 / 橙色原位展开 / 绿色单击打开\n在同一个窗口中添加、删除和重加角标。",0,310,684,504,42,231);
  control(L"BUTTON",L"重建图标缓存",BS_PUSHBUTTON|WS_TABSTOP,24,740,155,32,125);cacheNote=control(L"STATIC",L"缓存位于程序旁的cache文件夹",0,190,743,624,32,126);
  control(L"BUTTON",L"清理失效配置及孤立角标",BS_PUSHBUTTON|WS_TABSTOP,24,782,200,32,214);control(L"STATIC",L"桌面删除入口后自动清理；无后台时下次打开设置清理。",0,236,782,578,40,216);
  delayNote=control(L"STATIC",L"面板等待只控制展开；绿色单击打开松开即执行，无双击等待。\n后台按钮即时生效；退出仅停止本次监听，保留开机启动设置。",0,24,832,790,60,0);
  control(L"BUTTON",L"保存并应用",BS_DEFPUSHBUTTON|WS_TABSTOP,430,1020,135,36,130);control(L"BUTTON",L"取消",BS_PUSHBUTTON|WS_TABSTOP,575,1020,112,36,131);layoutSettings(840);enableMetrics();SetTimer(h,32,1500,nullptr);return 0;}
 case WM_COMMAND:{int id=LOWORD(w);
  if(id==101||id==102){if(id==102&&SendMessageW(followFont,BM_GETCHECK,0,0)==BST_CHECKED)SetWindowTextW(fields[6],std::to_wstring(desktopFont().points).c_str());enableMetrics();return 0;}
  if(id==120){static COLORREF custom[16];CHOOSECOLORW cc={sizeof(cc)};cc.hwndOwner=h;cc.rgbResult=chosen;cc.lpCustColors=custom;cc.Flags=CC_FULLOPEN|CC_RGBINIT;if(ChooseColorW(&cc))chosen=cc.rgbResult;return 0;}
  if(id==220||id==221){chooseDesktopLocation(h,id==221);return 0;}
  if(id==230){if(draftDesktopPath!=cfg.desktopPath){MessageBoxW(h,L"桌面位置尚未保存，请先保存并应用，再管理入口。",L"请先保存桌面位置",MB_OK|MB_ICONINFORMATION);return 0;}showEntryManager(h);mergeExternalRulesIntoDraft();return 0;}
  if(id==214){mergeExternalRulesIntoDraft();auto result=EntryMaintenance::reconcile();std::wstring text=L"已清理配置："+std::to_wstring(result.removed)+L"；恢复孤立角标："+std::to_wstring(result.restored)+L"；保留待核验："+std::to_wstring(result.deferred);SetWindowTextW(GetDlgItem(h,216),text.c_str());if(!result.warning.empty())MessageBoxW(h,result.warning.c_str(),L"部分项目已安全跳过",MB_OK|MB_ICONINFORMATION);HWND other=FindWindowW(CONTROL_CLASS,nullptr);if(other&&other!=controller)PostMessageW(other,WM_CONFIG,0,0);return 0;}
  if(id==125){if(panelWindow)closePanel();bool ok=IconCache::rebuild(baseDir+L"\\cache");SetWindowTextW(GetDlgItem(h,126),ok?L"已清除，下次展开时自动重建。":L"重建失败，请检查权限或稍后重试。");return 0;}
  if(id==130){DesktopModes::Guard modeGuard;if(!modeGuard.held){MessageBoxW(h,L"打开方式正在保存，请稍后重试。",L"保存",MB_OK);return 0;}mergeExternalRulesIntoDraft();Config next=cfg;int v[9];for(int i=0;i<9;i++){wchar_t b[32];GetWindowTextW(fields[i],b,32);v[i]=_wtoi(b);}next.followDesktop=SendMessageW(follow,BM_GETCHECK,0,0)==BST_CHECKED;next.followFont=SendMessageW(followFont,BM_GETCHECK,0,0)==BST_CHECKED;auto metrics=next.followDesktop?desktopMetrics():normalizeMetrics(v[0],v[1],v[2]);if(!next.followDesktop&&(metrics.icon!=v[0]||metrics.cellW!=v[1]||metrics.cellH!=v[2])){MessageBoxW(h,L"图标范围24～256；横向网格至少图标+16，纵向至少图标+44。",L"请检查尺寸",MB_ICONWARNING);return 0;}if(v[3]<2||v[3]>10||v[4]<10||v[4]>100||v[5]<0||v[5]>80||(!next.followFont&&(v[6]<8||v[6]>36))||v[7]<0||v[7]>24||v[8]<0||v[8]>500){MessageBoxW(h,L"请按各项标注的范围填写。每行2～10个；圆角0～80。",L"请检查设置",MB_ICONWARNING);return 0;}
   next.icon=metrics.icon;next.cellW=metrics.cellW;next.cellH=metrics.cellH;next.columns=v[3];next.opacity=v[4];next.radius=v[5];next.fontSize=next.followFont?desktopFont().points:v[6];next.textGap=v[7];next.expandDelay=v[8];next.contextMenu=SendMessageW(contextMenu,BM_GETCHECK,0,0)==BST_CHECKED;next.dotHidden=SendMessageW(dotHidden,BM_GETCHECK,0,0)==BST_CHECKED;next.showHidden=SendMessageW(showHidden,BM_GETCHECK,0,0)==BST_CHECKED;next.background=chosen;next.autoStart=SendMessageW(autoStart,BM_GETCHECK,0,0)==BST_CHECKED;
   next.desktopPath=draftDesktopPath;std::wstring root,error;
   for(const auto&r:draftRules)if(r.desktop&&DesktopModes::direct(r.path)){MessageBoxW(h,L"有目录已设为单击直接打开，请通过右键打开方式切换为面板。",L"打开方式冲突",MB_OK|MB_ICONINFORMATION);return 0;}
   if(!DesktopScope::resolve(next.desktopPath,root,error)){MessageBoxW(h,error.c_str(),L"桌面位置不可用",MB_OK|MB_ICONWARNING);return 0;}
   auto nextRules=EntryActions::mergeDesktopRules(folderRules,draftRules);size_t removed=DesktopScope::prune(nextRules,root);
   if(removed){auto prompt=L"有"+std::to_wstring(removed)+L"个原位展开规则不属于当前桌面的直接子文件夹。\n保存会撤销这些原位规则并恢复原图标；独立的面板内展开设置保留，真实目录不动。\n已生成的蓝色快捷方式保留原位置，不会搬动。\n\n继续保存？";if(MessageBoxW(h,prompt.c_str(),L"桌面范围变更",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;}
   Config old=cfg;auto oldRules=folderRules;auto oldEntries=entries;
   std::wstring oldRoot,ignored;if(old.desktopPath!=next.desktopPath){if(DesktopScope::resolve(old.desktopPath,oldRoot,ignored))DesktopScope::rememberEntryLocations(entries,oldRoot);else if(!old.desktopPath.empty())DesktopScope::rememberEntryLocations(entries,old.desktopPath);}
   cfg=next;folderRules=std::move(nextRules);
   if(!saveConfig()){cfg=old;folderRules=oldRules;entries=oldEntries;MessageBoxW(h,configSaveError.c_str(),L"保存失败",MB_ICONERROR);return 0;}
    if(next.contextMenu||next.contextMenu!=old.contextMenu){std::wstring menuError;if(!ContextMenu::sync(next.contextMenu!=0,exePath,menuError)){cfg=old;folderRules=oldRules;entries=oldEntries;std::wstring rollbackError;ContextMenu::sync(old.contextMenu!=0,exePath,rollbackError);saveConfig();MessageBoxW(h,menuError.c_str(),L"右键菜单未应用",MB_ICONERROR);return 0;}}
   auto badgeFailures=EntryActions::syncDesktopBadges(oldRules,folderRules);
   if(!badgeFailures.empty())MessageBoxW(h,(L"设置已保存，但以下目录的角标添加或恢复未完成；移除的目录也可通过恢复原图标按钮重新选择后重试：\n\n"+badgeFailures).c_str(),L"角标未完成",MB_OK|MB_ICONWARNING);
   if(panelWindow)closePanel();HWND other=FindWindowW(CONTROL_CLASS,nullptr);if(other&&other!=controller)PostMessageW(other,WM_CONFIG,0,0);DestroyWindow(h);return 0;}
  if(id==128){HWND other=FindWindowW(CONTROL_CLASS,nullptr);if(other){if(!confirmStopBackground(h))return 0;if(IsWindow(other))PostMessageW(other,WM_CLOSE,0,0);}else launchSelf(L"--resident");EnableWindow(backgroundToggle,FALSE);return 0;}
  if(id==131){DestroyWindow(h);return 0;}break;}
 case WM_TIMER:if(w==32){refreshBackgroundState();HWND other=FindWindowW(CONTROL_CLASS,nullptr);if(!other&&!residentMode&&EntryMaintenance::tick())PostMessageW(h,WM_CONFIG,0,0);}return 0;
 case WM_CONFIG:mergeExternalRulesIntoDraft();return 0;
 case WM_GETMINMAXINFO:{auto* info=reinterpret_cast<MINMAXINFO*>(l);RECT minRect{0,0,840,0};AdjustWindowRectEx(&minRect,(DWORD)GetWindowLongPtrW(h,GWL_STYLE),FALSE,(DWORD)GetWindowLongPtrW(h,GWL_EXSTYLE));info->ptMinTrackSize.x=minRect.right-minRect.left;info->ptMinTrackSize.y=400;return 0;}
 case WM_SIZE:{RECT rc{};GetClientRect(h,&rc);layoutSettings(rc.right);scrollSettings(settingsScroll);return 0;}
 case WM_MOUSEWHEEL:scrollSettings(settingsScroll-GET_WHEEL_DELTA_WPARAM(w)/WHEEL_DELTA*60);return 0;
 case WM_VSCROLL:{RECT rc{};GetClientRect(h,&rc);int next=settingsScroll;switch(LOWORD(w)){
  case SB_TOP:next=0;break;case SB_BOTTOM:next=settingsHeight;break;case SB_LINEUP:next-=30;break;case SB_LINEDOWN:next+=30;break;
  case SB_PAGEUP:next-=rc.bottom;break;case SB_PAGEDOWN:next+=rc.bottom;break;
  case SB_THUMBTRACK:case SB_THUMBPOSITION:{SCROLLINFO i{sizeof(i),SIF_TRACKPOS};GetScrollInfo(h,SB_VERT,&i);next=i.nTrackPos;break;}
 }scrollSettings(next);return 0;}
 case WM_CTLCOLORSTATIC:if((HWND)l==backgroundHelp||(HWND)l==backgroundHelpTitle||(HWND)l==backgroundHelpBody){SetTextColor((HDC)w,RGB(27,66,109));SetBkColor((HDC)w,RGB(234,244,255));return (LRESULT)(helpBrush?helpBrush:GetSysColorBrush(COLOR_WINDOW));}break;
 case WM_CLOSE:DestroyWindow(h);return 0;
 case WM_DESTROY:settingsWindow=nullptr;if(helpFont){DeleteObject(helpFont);helpFont=nullptr;}if(helpBrush){DeleteObject(helpBrush);helpBrush=nullptr;}if(font){DeleteObject(font);font=nullptr;}if(!residentMode&&!panelWindow)PostQuitMessage(0);return 0;}
 return DefWindowProcW(h,m,w,l);}
static void revealSettings(){
 if(!settingsWindow)return;
 ShowWindow(settingsWindow,IsIconic(settingsWindow)?SW_RESTORE:SW_SHOW);
 // A hidden launcher can override the process's first ShowWindow call.
 // This is an explicit settings request, not the silent startup path.
 if(!IsWindowVisible(settingsWindow)||IsIconic(settingsWindow))
  ShowWindow(settingsWindow,IsIconic(settingsWindow)?SW_RESTORE:SW_SHOW);
 SetForegroundWindow(settingsWindow);
}
void showSettings(){
 if(settingsWindow){revealSettings();return;}
 if(EntryMaintenance::tick(true)){HWND other=FindWindowW(CONTROL_CLASS,nullptr);if(other&&other!=controller)PostMessageW(other,WM_CONFIG,0,0);}
 WNDCLASSW wc{};wc.hInstance=instance;wc.hIcon=applicationIcon(GetSystemMetrics(SM_CXICON));wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpfnWndProc=proc;wc.lpszClassName=L"FolderIconCN.Settings";RegisterClassW(&wc);
 POINT p{};GetCursorPos(&p);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(p,MONITOR_DEFAULTTONEAREST),&mi);
 DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_THICKFRAME|WS_VSCROLL;
 RECT rc{0,0,840,std::min(settingsHeight,std::max(240,(int)(mi.rcWork.bottom-mi.rcWork.top)-100))};AdjustWindowRectEx(&rc,style,FALSE,0);
 settingsWindow=CreateWindowExW(0,wc.lpszClassName,L"文件夹面板设置",style,mi.rcWork.left+24,mi.rcWork.top+24,rc.right-rc.left,rc.bottom-rc.top,nullptr,nullptr,instance,nullptr);
 revealSettings();
}
