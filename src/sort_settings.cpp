#include "app.h"
#include "sort_settings.h"
namespace {
struct Editor {
 HWND window=nullptr,list=nullptr,mode=nullptr,direction=nullptr;HFONT font=nullptr;
 std::wstring folder;const std::vector<FolderSort::Item>& items;
 FolderSort::Preferences value;std::vector<size_t> order;bool saved=false,dragging=false;int source=-1;POINT start{};
 Editor(const std::wstring& f,const std::vector<FolderSort::Item>& i):folder(f),items(i),value(FolderSort::load(baseDir+L"\\layout",f)){}
 bool custom()const{return value.mode==FolderSort::Mode::Custom;}
 void remember(){std::vector<std::wstring> visible;for(auto i:order)visible.push_back(items[i].id);value.custom=FolderSort::mergeCustom(value.custom,visible);}
 void refresh(int selected=0){
  order=FolderSort::indices(items,value);SendMessageW(list,WM_SETREDRAW,FALSE,0);SendMessageW(list,LB_RESETCONTENT,0,0);
  for(auto i:order){auto label=(items[i].directory?L"[文件夹] ":L"")+items[i].name;SendMessageW(list,LB_ADDSTRING,0,(LPARAM)label.c_str());}
  if(!order.empty())SendMessageW(list,LB_SETCURSEL,clampValue(selected,0,(int)order.size()-1),0);
  SendMessageW(list,WM_SETREDRAW,TRUE,0);InvalidateRect(list,nullptr,TRUE);
  EnableWindow(direction,!custom());EnableWindow(GetDlgItem(window,204),custom());EnableWindow(GetDlgItem(window,205),custom());
 }
 void reorder(int from,int to){if(custom()&&FolderSort::move(order,from,to)){remember();refresh(to);}}
};
HWND control(Editor& e,const wchar_t* cls,const wchar_t* title,DWORD style,int x,int y,int w,int h,int id){
 HWND c=CreateWindowExW(!wcscmp(cls,L"LISTBOX")?WS_EX_CLIENTEDGE:0,cls,title,WS_CHILD|WS_VISIBLE|style,x,y,w,h,e.window,(HMENU)(INT_PTR)id,instance,nullptr);
 SendMessageW(c,WM_SETFONT,(WPARAM)e.font,TRUE);return c;
}
LRESULT CALLBACK listProc(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data){
 auto&e=*(Editor*)data;
 if(m==WM_LBUTTONDOWN&&e.custom()){
  // Own the complete custom-drag gesture. Native listbox button-down handling
  // also manages capture and otherwise cancels our drag during SetCapture.
  DWORD hit=(DWORD)SendMessageW(h,LB_ITEMFROMPOINT,0,l);
  SetFocus(h);e.source=HIWORD(hit)?-1:(int)LOWORD(hit);
  e.start={GET_X_LPARAM(l),GET_Y_LPARAM(l)};e.dragging=false;
  if(e.source>=0){SendMessageW(h,LB_SETCURSEL,e.source,0);SetCapture(h);}return 0;
 }
 if(m==WM_MOUSEMOVE&&e.custom()&&e.source>=0&&(w&MK_LBUTTON)){
  int x=GET_X_LPARAM(l),y=GET_Y_LPARAM(l);
  if(abs(x-e.start.x)>GetSystemMetrics(SM_CXDRAG)||abs(y-e.start.y)>GetSystemMetrics(SM_CYDRAG))e.dragging=true;
  RECT rc;GetClientRect(h,&rc);int top=(int)SendMessageW(h,LB_GETTOPINDEX,0,0);
  if(y<0)SendMessageW(h,LB_SETTOPINDEX,std::max(0,top-1),0);else if(y>=rc.bottom)SendMessageW(h,LB_SETTOPINDEX,top+1,0);
  if(e.dragging&&!e.order.empty()){int to=LOWORD(SendMessageW(h,LB_ITEMFROMPOINT,0,MAKELPARAM(clampValue(x,0,rc.right-1),clampValue(y,0,rc.bottom-1))));SendMessageW(h,LB_SETCURSEL,to,0);}return 0;
 }
 if(m==WM_LBUTTONUP&&e.source>=0){int from=e.source,to=(int)SendMessageW(h,LB_GETCURSEL,0,0);bool dragged=e.dragging;e.source=-1;e.dragging=false;ReleaseCapture();if(dragged)e.reorder(from,to);return 0;}
 // SetCapture can notify the same listbox; reset only when capture really leaves it.
 if(m==WM_CAPTURECHANGED&&(HWND)l!=h){e.source=-1;e.dragging=false;}
 if(m==WM_KEYDOWN&&e.custom()&&(GetKeyState(VK_MENU)&0x8000)&&(w==VK_UP||w==VK_DOWN)){
  int from=(int)SendMessageW(h,LB_GETCURSEL,0,0);e.reorder(from,from+(w==VK_UP?-1:1));return 0;
 }
 return DefSubclassProc(h,m,w,l);
}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_NCCREATE){auto e=(Editor*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)e);e->window=h;}
 auto e=(Editor*)GetWindowLongPtrW(h,GWLP_USERDATA);if(!e)return DefWindowProcW(h,m,w,l);
 switch(m){
 case WM_CREATE:
  e->font=CreateFontW(-17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,DEFAULT_QUALITY,0,L"Microsoft YaHei UI");
  SendMessageW(h,WM_SETICON,ICON_BIG,(LPARAM)applicationIcon(32));
  control(*e,L"STATIC",L"当前文件夹排序（每个文件夹单独保存）",0,18,14,530,28,0);
  e->mode=control(*e,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,18,52,250,150,201);
  for(auto label:{L"按名称（默认）",L"按修改时间",L"自定义排序"})SendMessageW(e->mode,CB_ADDSTRING,0,(LPARAM)label);
  SendMessageW(e->mode,CB_SETCURSEL,(int)e->value.mode,0);
  e->direction=control(*e,L"BUTTON",L"倒序（名称Z→A／最新在前）",BS_AUTOCHECKBOX|WS_TABSTOP,280,52,290,28,202);
  SendMessageW(e->direction,BM_SETCHECK,e->value.descending?BST_CHECKED:BST_UNCHECKED,0);
  control(*e,L"STATIC",L"自定义：拖动下面的列表，或选中后用上移／下移。\n名称和时间排序保留文件夹优先；不会移动或改名真实文件。",0,18,94,550,48,0);
  e->list=control(*e,L"LISTBOX",L"",LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|WS_VSCROLL|WS_TABSTOP,18,150,552,270,203);
  SetWindowSubclass(e->list,listProc,1,(DWORD_PTR)e);
  control(*e,L"BUTTON",L"上移",BS_PUSHBUTTON|WS_TABSTOP,18,434,85,34,204);
  control(*e,L"BUTTON",L"下移",BS_PUSHBUTTON|WS_TABSTOP,115,434,85,34,205);
  control(*e,L"BUTTON",L"保存并应用",BS_DEFPUSHBUTTON|WS_TABSTOP,305,434,145,34,206);
  control(*e,L"BUTTON",L"取消",BS_PUSHBUTTON|WS_TABSTOP,465,434,105,34,207);e->refresh();return 0;
 case WM_COMMAND:{int id=LOWORD(w);
  if(id==201&&HIWORD(w)==CBN_SELCHANGE){auto next=(FolderSort::Mode)SendMessageW(e->mode,CB_GETCURSEL,0,0);
   if(next==FolderSort::Mode::Custom&&e->value.custom.empty())e->remember();e->value.mode=next;e->refresh();return 0;}
  if(id==202){e->value.descending=SendMessageW(e->direction,BM_GETCHECK,0,0)==BST_CHECKED;e->refresh();return 0;}
  if(id==204||id==205){int from=(int)SendMessageW(e->list,LB_GETCURSEL,0,0);e->reorder(from,from+(id==204?-1:1));return 0;}
  if(id==206||id==IDOK){if(e->custom())e->remember();
   if(!FolderSort::save(baseDir+L"\\layout",e->folder,e->value)){MessageBoxW(h,L"排序保存失败，原有排序未被替换。请检查程序目录权限。",L"保存失败",MB_ICONERROR);return 0;}
   e->saved=true;DestroyWindow(h);return 0;}
  if(id==207||id==IDCANCEL){DestroyWindow(h);return 0;}break;
 }
 case WM_CLOSE:DestroyWindow(h);return 0;
 case WM_DESTROY:if(e->font){DeleteObject(e->font);e->font=nullptr;}return 0;
 }return DefWindowProcW(h,m,w,l);
}
}
bool showSortSettings(HWND owner,const std::wstring& folder,const std::vector<FolderSort::Item>& items){
 Editor state(folder,items);WNDCLASSW wc{};wc.hInstance=instance;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=applicationIcon(32);
 wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpfnWndProc=proc;wc.lpszClassName=L"FolderIconCN.SortSettings";RegisterClassW(&wc);
 RECT bounds{0,0,590,490};DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;AdjustWindowRectEx(&bounds,style,FALSE,0);
 HWND h=CreateWindowExW(WS_EX_DLGMODALFRAME,wc.lpszClassName,L"排序与整理",style,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,owner,nullptr,instance,&state);
 if(!h)return false;EnableWindow(owner,FALSE);ShowWindow(h,SW_SHOW);SetForegroundWindow(h);MSG message{};bool quit=false;int exitCode=0;
 while(IsWindow(h)){BOOL result=GetMessageW(&message,nullptr,0,0);if(result<=0){quit=result==0;exitCode=(int)message.wParam;break;}
  if(!IsDialogMessageW(h,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
 if(IsWindow(h))DestroyWindow(h);if(IsWindow(owner)){EnableWindow(owner,TRUE);SetForegroundWindow(owner);}if(quit)PostQuitMessage(exitCode);return state.saved;
}
