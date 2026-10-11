#include "app.h"
#include "icon_cache.h"
#include "sort_settings.h"
#include "folder_navigation.h"
#include "entry_actions.h"
#include "badge_visual.h"
#include "panel_interaction.h"
using namespace Gdiplus;
static PanelInteraction::ClickGuard panelClicks;static bool backHover=false;
HWND panelWindow=nullptr;static int width,height,cols,scrollRow=0,hover=-1,selected=-1;static std::wstring folder;static FolderNavigation navigation;static POINT location;static bool closing=false,menuActive=false;static HHOOK outsideHook=nullptr;static int cellHeight,labelPixels=16,labelGap=3;static std::wstring labelFace=L"Microsoft YaHei UI";

// Exists only while a panel is visible. Never consume another application's click.
static LRESULT CALLBACK outsideClick(int code,WPARAM w,LPARAM l){
 if(code==HC_ACTION&&panelWindow&&!menuActive&&(w==WM_LBUTTONDOWN||w==WM_RBUTTONDOWN||w==WM_MBUTTONDOWN||w==WM_XBUTTONDOWN)){
  HWND hit=WindowFromPoint(((MSLLHOOKSTRUCT*)l)->pt),root=GetAncestor(hit,GA_ROOT);
  DWORD pid=0;GetWindowThreadProcessId(root,&pid);
  if(root!=panelWindow&&root!=settingsWindow&&pid!=GetCurrentProcessId())PostMessageW(panelWindow,WM_CLOSE,0,0);
 }
 return CallNextHookEx(outsideHook,code,w,l);
}

struct Item{std::wstring name,path,id;bool dir;HBITMAP image=nullptr;bool attempted=false;unsigned long long modified=0;bool realDir=false;};static std::vector<Item> items;
static std::vector<FolderSort::Item> sortItems(){std::vector<FolderSort::Item> values;for(const auto&i:items)values.push_back({i.id,i.name,i.dir,i.modified});return values;}
static void applyOrder(){auto order=FolderSort::indices(sortItems(),FolderSort::load(baseDir+L"\\layout",folder));std::vector<Item> next;next.reserve(items.size());for(auto i:order)next.push_back(items[i]);items.swap(next);}
static void clearItems(){for(auto&i:items)if(i.image)DeleteObject(i.image);items.clear();}
static void rounded(GraphicsPath& p,float x,float y,float w,float h,float r){r=std::min(r,std::min(w,h)/2);if(r<1){p.AddRectangle(RectF(x,y,w,h));return;}float d=2*r;p.AddArc(x,y,d,d,180,90);p.AddArc(x+w-d,y,d,d,270,90);p.AddArc(x+w-d,y+h-d,d,d,0,90);p.AddArc(x,y+h-d,d,d,90,90);p.CloseFigure();}
static void drawBitmap(Graphics& g,HBITMAP hb,int x,int y,int size){BITMAP bm;GetObjectW(hb,sizeof(bm),&bm);if(bm.bmWidth<=0||bm.bmHeight<=0)return;BITMAPINFO bi={};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=bm.bmWidth;bi.bmiHeader.biHeight=-bm.bmHeight;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;std::vector<BYTE> pixels(bm.bmWidth*bm.bmHeight*4);HDC dc=GetDC(nullptr);GetDIBits(dc,hb,0,bm.bmHeight,pixels.data(),&bi,DIB_RGB_COLORS);ReleaseDC(nullptr,dc);bool alpha=false;for(size_t i=3;i<pixels.size();i+=4)if(pixels[i]){alpha=true;break;}if(!alpha)for(size_t i=3;i<pixels.size();i+=4)pixels[i]=255;Bitmap image(bm.bmWidth,bm.bmHeight,bm.bmWidth*4,PixelFormat32bppPARGB,pixels.data());g.DrawImage(&image,Rect(x,y,size,size));}
static void text(Graphics& g,const std::wstring&s,RectF rc,float size,Color color,bool center=false,bool label=false){Font font(label?labelFace.c_str():L"Microsoft YaHei UI",size,FontStyleRegular,UnitPixel);SolidBrush brush(color);StringFormat fmt;fmt.SetAlignment(center?StringAlignmentCenter:StringAlignmentNear);fmt.SetLineAlignment(label?StringAlignmentNear:StringAlignmentCenter);fmt.SetTrimming(StringTrimmingEllipsisCharacter);g.DrawString(s.c_str(),-1,&font,rc,&fmt,&brush);}
static int rowCount(){return ((int)items.size()+cols-1)/cols;}
static int visibleRows(){return std::max(1,(height-82)/cellHeight);}
static void paintPanel(){if(!panelWindow)return;HDC screen=GetDC(nullptr),mem=CreateCompatibleDC(screen);BITMAPINFO bi={};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;void* data=nullptr;HBITMAP dib=CreateDIBSection(screen,&bi,DIB_RGB_COLORS,&data,nullptr,0);HGDIOBJ old=SelectObject(mem,dib);{
 Bitmap b(width,height,width*4,PixelFormat32bppPARGB,(BYTE*)data);Graphics g(&b);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetPixelOffsetMode(PixelOffsetModeHalf);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);GraphicsPath outline;rounded(outline,0,0,(float)width,(float)height,(float)cfg.radius);SolidBrush bg(Color((BYTE)(cfg.opacity*255/100),GetRValue(cfg.background),GetGValue(cfg.background),GetBValue(cfg.background)));g.FillPath(&bg,&outline);g.SetClip(&outline);
 int lum=(GetRValue(cfg.background)*299+GetGValue(cfg.background)*587+GetBValue(cfg.background)*114)/1000;Color fg=lum>155?Color(255,25,28,32):Color(255,246,247,250);Color muted=lum>155?Color(255,65,70,78):Color(255,193,199,210);
 std::wstring title=PathFindFileNameW(folder.c_str());if(title.empty())title=folder;
 int step=PanelInteraction::actionStep(width);float titleLeft=18;
 if(navigation.hasParent()){
  auto r=PanelInteraction::backRect(width);GraphicsPath button;rounded(button,(float)r.x,(float)r.y,(float)r.w,(float)r.h,8);
  SolidBrush fill(backHover?Color(255,40,126,202):Color(255,29,106,177));g.FillPath(&fill,&button);
  text(g,r.w>=132?L"← 返回上一级":r.w>=68?L"← 返回":L"←",RectF((float)r.x,(float)r.y,(float)r.w,(float)r.h),15,Color(255,255,255,255),true);titleLeft=(float)(r.x+r.w+10);
 }
 float titleWidth=std::max(0.f,width-3*step-titleLeft-6);if(titleWidth>0)text(g,title,RectF(titleLeft,4,titleWidth,42),18,fg);
 text(g,L"排序",RectF((float)width-3*step,4,(float)step-4,42),14,fg,true);text(g,L"设置",RectF((float)width-2*step,4,(float)step-4,42),14,fg,true);text(g,L"关闭",RectF((float)width-step,4,(float)step-4,42),14,fg,true);
 int start=scrollRow*cols,end=std::min((int)items.size(),start+visibleRows()*cols);for(int i=start;i<end;i++){int col=(i-start)%cols,row=(i-start)/cols;int x=12+col*cfg.cellW,y=50+row*cellHeight;if(i==hover||i==selected){GraphicsPath p;rounded(p,(float)x+3,(float)y+2,(float)cfg.cellW-6,(float)cellHeight-5,10);SolidBrush h(Color(52,128,155,195));g.FillPath(&h,&p);}auto& it=items[i];if(it.image)drawBitmap(g,it.image,x+(cfg.cellW-cfg.icon)/2,y+5,cfg.icon);if(it.realDir&&FolderRules::contains(folderRules,it.path,false)){int d=BadgeVisual::extent(cfg.icon);BadgeVisual::draw(g,(float)(x+(cfg.cellW+cfg.icon)/2-d),(float)y+5,cfg.icon,BadgeVisual::Kind::NestedFolder);}text(g,it.name,RectF((float)x+5,(float)labelTop(y,cfg.icon,labelGap),(float)cfg.cellW-10,(float)cellHeight-cfg.icon-labelGap-10),(float)labelPixels,fg,true,true);}
 std::wstring footer=std::to_wstring(items.size())+(navigation.hasParent()?L"项 · 双击空白返回上一级":L"项 · 单击打开 · 滚轮翻页");text(g,footer,RectF(16,(float)height-30,(float)width-32,24),12,muted);if(rowCount()>visibleRows()){float track=(float)(height-88),bar=std::max(18.f,track*visibleRows()/rowCount()),y=52+(track-bar)*scrollRow/std::max(1,rowCount()-visibleRows());SolidBrush sb(Color(140,140,150,165));g.FillRectangle(&sb,(float)width-6,y,3.f,bar);}
 }
 SIZE sz{width,height};POINT zero{};BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};UpdateLayeredWindow(panelWindow,screen,&location,&sz,mem,&zero,0,&blend,ULW_ALPHA);SelectObject(mem,old);DeleteObject(dib);DeleteDC(mem);ReleaseDC(nullptr,screen);
}
// Warm icons are ready before the first paint (also when scrolling). File lists stay live.
static void loadVisibleCache(){
 int start=scrollRow*cols,end=std::min((int)items.size(),start+visibleRows()*cols);
 for(int i=start;i<end;i++)if(!items[i].attempted){
  items[i].image=IconCache::load(baseDir+L"\\cache",items[i].path,cfg.icon);
  if(items[i].image)items[i].attempted=true;
 }
}
// Cold icons are extracted one per message-loop turn; no polling once ready.
// No worker survives panel dismissal and no polling runs when all icons are ready.
static void loadNextIcon(){
 if(!panelWindow)return;
 int start=scrollRow*cols,end=std::min((int)items.size(),start+visibleRows()*cols);
 for(int i=start;i<end;i++)if(!items[i].attempted){
  items[i].attempted=true;items[i].image=shellBitmap(items[i].path,cfg.icon);if(items[i].image)IconCache::store(baseDir+L"\\cache",items[i].path,cfg.icon,items[i].image);paintPanel();return;
 }
 KillTimer(panelWindow,22);
}
static int hitItem(int x,int y){if(x<12||x>=12+cols*cfg.cellW||y<50||y>=height-32)return -1;int row=(y-50)/cellHeight;if(row>=visibleRows())return -1;int i=(scrollRow+row)*cols+(x-12)/cfg.cellW;return i<(int)items.size()?i:-1;}
static void showPanelPath(const std::wstring& path,POINT anchor,bool preview,FolderNavigation next);
static POINT panelAnchorPoint{};
static void reportFolderUnavailable(){
 // A modal error must not trigger the panel's outside-click/deactivation close.
 bool wasMenuActive=menuActive;menuActive=true;
 MessageBoxW(panelWindow,L"目标文件夹不存在或不可访问。",L"文件夹面板",MB_ICONERROR);
 menuActive=wasMenuActive;
 if(!panelWindow&&!residentMode&&!settingsWindow)PostQuitMessage(1);
}
static bool enterFolder(const std::wstring& path){
 if(!FolderRules::contains(folderRules,path,false))return false;
 auto identity=FolderRules::identity(path);
 if(identity.empty()){reportFolderUnavailable();return true;}
 auto next=navigation;if(!next.enter(path,identity))return false;
 showPanelPath(path,panelAnchorPoint,false,std::move(next));return true;
}
static void goBack(){
 auto next=navigation;if(!next.back())return;
 auto path=next.path();showPanelPath(path,panelAnchorPoint,false,std::move(next));
}
static void openItem(int n){
 if(n<0||n>=(int)items.size())return;
 auto p=items[n].path;
 if(items[n].realDir&&FolderRules::contains(folderRules,p,false)){if(enterFolder(p))return;}
 bool opened=openShellPath(panelWindow,p,items[n].dir);
 if(opened)closePanel();
 else MessageBoxW(panelWindow,L"无法打开这个项目，请检查目标是否存在。",L"文件夹面板",MB_ICONERROR);
}
static void editOrder(){menuActive=true;bool changed=showSortSettings(panelWindow,folder,sortItems());menuActive=false;if(changed&&panelWindow){applyOrder();scrollRow=0;selected=hover=-1;loadVisibleCache();paintPanel();SetTimer(panelWindow,22,10,nullptr);}}
static void showContextMenu(HWND h,int item){
 std::wstring target;bool eligible=item>=0&&item<(int)items.size()&&items[item].dir&&EntryActions::realFolder(items[item].path);
 if(eligible){target=items[item].path;selected=item;paintPanel();}
 HMENU menu=CreatePopupMenu();bool enabled=eligible&&FolderRules::contains(folderRules,target,false);
 if(eligible){AppendMenuW(menu,MF_STRING|(enabled?MF_CHECKED:0),5,L"在面板内展开");AppendMenuW(menu,MF_STRING,6,L"在资源管理器中打开此文件夹");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);}
 AppendMenuW(menu,MF_STRING,4,L"排序与整理…");AppendMenuW(menu,MF_STRING,1,L"设置外观与后台…");AppendMenuW(menu,MF_STRING,2,L"在资源管理器中打开当前文件夹");AppendMenuW(menu,MF_STRING,3,L"关闭");
 POINT p;GetCursorPos(&p);menuActive=true;int id=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,0,h,nullptr);DestroyMenu(menu);menuActive=true;
 if(id==5&&eligible){std::wstring error;if(!EntryActions::setPanelExpansion(target,!enabled,error))MessageBoxW(h,error.c_str(),L"设置未保存",MB_OK|MB_ICONERROR);else{HWND other=FindWindowW(CONTROL_CLASS,nullptr);if(other&&other!=controller)SendMessageW(other,WM_CONFIG,0,0);}paintPanel();}
 if(id==6&&eligible){if(openShellPath(h,target,true))closePanel();}
 if(id==1){showSettings();closePanel();}if(id==2){openShellPath(h,folder,true);closePanel();}if(id==3)closePanel();if(id==4)editOrder();menuActive=false;
}
static bool backAt(int x,int y){return navigation.hasParent()&&PanelInteraction::backRect(width).contains(x,y);}
static bool blankAt(int x,int y){return navigation.hasParent()&&PanelInteraction::blank(x,y,width,height,cfg.radius,hitItem(x,y),rowCount()>visibleRows());}
static LRESULT CALLBACK panelProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_TIMER:if(w==22)loadNextIcon();return 0;case WM_HOTKEY:if(w==1)closePanel();return 0;case WM_ENTERMENULOOP:menuActive=true;return 0;case WM_EXITMENULOOP:menuActive=false;return 0;case WM_ACTIVATE:if(LOWORD(w)==WA_INACTIVE&&!settingsWindow&&!closing&&!menuActive)PostMessageW(h,WM_CLOSE,0,0);return 0;case WM_MOUSEMOVE:{int x=GET_X_LPARAM(l),y=GET_Y_LPARAM(l),n=hitItem(x,y);bool bh=backAt(x,y);SetCursor(LoadCursorW(nullptr,bh?IDC_HAND:IDC_ARROW));TRACKMOUSEEVENT tracking{sizeof(tracking),TME_LEAVE,h,0};TrackMouseEvent(&tracking);if(n!=hover||bh!=backHover){hover=n;backHover=bh;paintPanel();}return 0;}case WM_MOUSELEAVE:hover=-1;backHover=false;paintPanel();return 0;
 case WM_LBUTTONDOWN:panelClicks.down(blankAt(GET_X_LPARAM(l),GET_Y_LPARAM(l)));return 0;
 case WM_LBUTTONDBLCLK:if(panelClicks.doubleClick(blankAt(GET_X_LPARAM(l),GET_Y_LPARAM(l))))goBack();return 0;case WM_MOUSEWHEEL:panelClicks.navigate();scrollRow=clampValue(scrollRow-GET_WHEEL_DELTA_WPARAM(w)/WHEEL_DELTA,0,std::max(0,rowCount()-visibleRows()));loadVisibleCache();paintPanel();SetTimer(h,22,10,nullptr);return 0;case WM_LBUTTONUP:{if(panelClicks.consumeUp())return 0;int x=GET_X_LPARAM(l),y=GET_Y_LPARAM(l);if(y<48){int step=PanelInteraction::actionStep(width);if(x>=width-step)closePanel();else if(x>=width-2*step){showSettings();closePanel();}else if(x>=width-3*step){editOrder();}else if(backAt(x,y)){goBack();}else{openShellPath(h,folder,true);closePanel();}}else openItem(hitItem(x,y));return 0;}case WM_RBUTTONUP:{panelClicks.navigate();showContextMenu(h,hitItem(GET_X_LPARAM(l),GET_Y_LPARAM(l)));return 0;}case WM_KEYDOWN:panelClicks.navigate();if(w==VK_ESCAPE){closePanel();return 0;}if(w==VK_BACK&&navigation.hasParent()){goBack();return 0;}if(w==VK_RETURN){openItem(selected<0?0:selected);return 0;}if(w==VK_RIGHT||w==VK_LEFT||w==VK_UP||w==VK_DOWN){int n=selected<0?0:selected+(w==VK_RIGHT?1:w==VK_LEFT?-1:w==VK_UP?-cols:cols);selected=clampValue(n,0,std::max(0,(int)items.size()-1));scrollRow=clampValue(selected/cols-visibleRows()+1,0,std::max(0,rowCount()-visibleRows()));loadVisibleCache();paintPanel();SetTimer(h,22,10,nullptr);return 0;}break;case WM_CLOSE:closePanel();return 0;}return DefWindowProcW(h,m,w,l);}
void closePanel(){if(closing)return;if(outsideHook){UnhookWindowsHookEx(outsideHook);outsideHook=nullptr;}if(panelWindow){UnregisterHotKey(panelWindow,1);KillTimer(panelWindow,22);}closing=true;HWND p=panelWindow;panelWindow=nullptr;if(p)DestroyWindow(p);clearItems();navigation.clear();panelClicks.clear();panelAnchorPoint={};closing=false;if(!residentMode&&!settingsWindow&&!quitting)PostQuitMessage(0);}
static void showPanelPath(const std::wstring& path,POINT anchor,bool preview,FolderNavigation next){
 // Validate before changing the visible panel, anchor or visited chain.
 DWORD a=GetFileAttributesW(path.c_str());
 if(a==INVALID_FILE_ATTRIBUTES||!(a&FILE_ATTRIBUTE_DIRECTORY)){reportFolderUnavailable();return;}
 navigation=std::move(next);panelClicks.navigate();backHover=false;panelAnchorPoint=anchor;
 if(outsideHook){UnhookWindowsHookEx(outsideHook);outsideHook=nullptr;}if(panelWindow){UnregisterHotKey(panelWindow,1);closing=true;DestroyWindow(panelWindow);panelWindow=nullptr;clearItems();closing=false;}auto df=desktopFont(monitorDpi(anchor));labelFace=df.face;labelPixels=cfg.followFont?df.pixels:fontPixels(cfg.fontSize,monitorDpi(anchor));labelGap=cfg.textGap;cellHeight=std::max(cfg.cellH,labelCellHeight(cfg.icon,labelPixels,labelGap));folder=path;WIN32_FIND_DATAW f;HANDLE find=FindFirstFileW((path+L"\\*").c_str(),&f);if(find!=INVALID_HANDLE_VALUE){do{if(!visibleItem(f.cFileName,f.dwFileAttributes,cfg.dotHidden,cfg.showHidden))continue;std::wstring name=f.cFileName,id=f.cFileName,full=path+L"\\"+f.cFileName;bool dir=(f.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0;if(!dir&&(!_wcsicmp(PathFindExtensionW(name.c_str()),L".lnk")||!_wcsicmp(PathFindExtensionW(name.c_str()),L".url")))name.resize(name.size()-4);items.push_back({name,full,id,dir,nullptr,false,((unsigned long long)f.ftLastWriteTime.dwHighDateTime<<32)|f.ftLastWriteTime.dwLowDateTime,dir&&!(f.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)});}while(FindNextFileW(find,&f));FindClose(find);}applyOrder();MONITORINFO mi={sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(anchor,MONITOR_DEFAULTTONEAREST),&mi);cols=std::min(cfg.columns,std::max(1,(int)(mi.rcWork.right-mi.rcWork.left-40)/cfg.cellW));width=PanelInteraction::panelWidth(cols*cfg.cellW+24);int maxrows=std::max(1,(int)(mi.rcWork.bottom-mi.rcWork.top-112)/cellHeight);height=std::min(std::max(1,rowCount()),std::min(5,maxrows))*cellHeight+82;auto pos=placePanel(panelAnchorPoint.x,panelAnchorPoint.y,width,height,mi.rcWork.left,mi.rcWork.top,mi.rcWork.right,mi.rcWork.bottom);location={pos.x,pos.y};WNDCLASSW wc{};wc.style=CS_DBLCLKS;wc.hInstance=instance;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.lpfnWndProc=panelProc;wc.lpszClassName=L"FolderIconCN.Panel";RegisterClassW(&wc);panelWindow=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_TOPMOST,L"FolderIconCN.Panel",L"文件夹面板",WS_POPUP,location.x,location.y,width,height,nullptr,nullptr,instance,nullptr);scrollRow=0;selected=hover=-1;loadVisibleCache();paintPanel();outsideHook=SetWindowsHookExW(WH_MOUSE_LL,outsideClick,instance,0);RegisterHotKey(panelWindow,1,0,VK_ESCAPE);ShowWindow(panelWindow,preview?SW_SHOWNOACTIVATE:SW_SHOW);SetTimer(panelWindow,22,10,nullptr);if(!preview){SetForegroundWindow(panelWindow);SetFocus(panelWindow);}}

void showPanel(const std::wstring& path,POINT anchor,bool preview){FolderNavigation next;next.reset(path,FolderRules::identity(path));showPanelPath(path,anchor,preview,std::move(next));}
