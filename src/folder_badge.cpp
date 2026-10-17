#include "folder_badge.h"
#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include "badge_visual.h"
#include <vector>
#include <array>
#include <algorithm>
#include <cwctype>
#include <cstring>
namespace FolderBadge {
namespace {
using Bytes=std::vector<BYTE>;
const wchar_t* keys[]={L"IconResource",L"IconFile",L"IconIndex"};
struct Doc {std::wstring text;int encoding=1;}; // 1=UTF16LE, 2=UTF8 BOM, 0=ACP
struct Value {bool present=false;std::wstring text;};
struct State {DWORD magic=0x31424746,folderAttrs=0,iniAttrs=0,hadIni=0,phase=1;};
struct Location {std::wstring folder,ini,dir,info,original,applied,icon,id;DWORD attrs=0;};
struct Guard {HANDLE h=nullptr;bool held=false;~Guard(){if(held)ReleaseMutex(h);if(h)CloseHandle(h);}};
static bool fail(std::wstring& e,const wchar_t* msg){e=msg;return false;}
static std::wstring trim(std::wstring s){auto a=s.find_first_not_of(L" \t\r\n");if(a==std::wstring::npos)return {};return s.substr(a,s.find_last_not_of(L" \t\r\n")-a+1);}
static bool same(const std::wstring&a,const std::wstring&b){return _wcsicmp(a.c_str(),b.c_str())==0;}
static bool exists(const std::wstring&p){return GetFileAttributesW(p.c_str())!=INVALID_FILE_ATTRIBUTES;}
static bool read(const std::wstring& p,Bytes& b,bool& had,DWORD* attrs=nullptr){
 b.clear();DWORD a=GetFileAttributesW(p.c_str());had=a!=INVALID_FILE_ATTRIBUTES;if(!had)return GetLastError()==ERROR_FILE_NOT_FOUND||GetLastError()==ERROR_PATH_NOT_FOUND;
 if(a&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))return false;
 HANDLE h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(h==INVALID_HANDLE_VALUE)return false;
 BY_HANDLE_FILE_INFORMATION i{};bool ok=GetFileInformationByHandle(h,&i)&&i.nNumberOfLinks==1&&i.nFileSizeHigh==0&&i.nFileSizeLow<=1024*1024;
 if(ok){b.resize(i.nFileSizeLow);DWORD n=0;ok=ReadFile(h,b.data(),(DWORD)b.size(),&n,nullptr)&&n==b.size();if(attrs)*attrs=i.dwFileAttributes;}
 CloseHandle(h);return ok;
}
static bool writeNew(const std::wstring&p,const Bytes&b){HANDLE h=CreateFileW(p.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return false;DWORD n=0;bool ok=WriteFile(h,b.data(),(DWORD)b.size(),&n,nullptr)&&n==b.size()&&FlushFileBuffers(h);CloseHandle(h);if(!ok)DeleteFileW(p.c_str());return ok;}
// A sibling temp name must not grow with the icon filename (MAX_PATH on Windows 10).
static std::wstring tempName(const std::wstring&p){GUID id{};if(FAILED(CoCreateGuid(&id)))return {};wchar_t token[40]{};if(!StringFromGUID2(id,token,40))return {};auto slash=p.find_last_of(L"\\/");return (slash==std::wstring::npos?L"":p.substr(0,slash+1))+L".fi-"+token+L".tmp";}
static bool replace(const std::wstring&p,const Bytes&b,bool had){auto tmp=tempName(p);if(tmp.empty()||!writeNew(tmp,b))return false;bool ok=had?ReplaceFileW(p.c_str(),tmp.c_str(),nullptr,0,nullptr,nullptr)!=FALSE:MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;if(!ok)DeleteFileW(tmp.c_str());return ok;}
static bool snapshot(const std::wstring&p,const Bytes&b){return replace(p,b,exists(p));}
static bool decode(const Bytes&b,Doc&d){
 d={};if(b.empty())return true;
 if(b.size()>=2&&b[0]==0xff&&b[1]==0xfe){if(b.size()%2)return false;d.encoding=1;d.text.assign((const wchar_t*)(b.data()+2),(b.size()-2)/2);}
 else {if(b.size()>=2&&b[0]==0xfe&&b[1]==0xff)return false;size_t offset=b.size()>=3&&b[0]==0xef&&b[1]==0xbb&&b[2]==0xbf?3:0;d.encoding=offset?2:0;UINT cp=offset?CP_UTF8:CP_ACP;int count=MultiByteToWideChar(cp,offset?MB_ERR_INVALID_CHARS:0,(const char*)b.data()+offset,(int)(b.size()-offset),nullptr,0);if(!count&&b.size()>offset)return false;d.text.resize(count);if(count)MultiByteToWideChar(cp,offset?MB_ERR_INVALID_CHARS:0,(const char*)b.data()+offset,(int)(b.size()-offset),d.text.data(),count);}
 return d.text.find(L'\0')==std::wstring::npos;
}
static Bytes encode(const Doc&d){
 if(d.encoding==1){Bytes b(2+d.text.size()*2);b[0]=0xff;b[1]=0xfe;if(!d.text.empty())memcpy(b.data()+2,d.text.data(),d.text.size()*2);return b;}
 BOOL used=FALSE;UINT cp=d.encoding==2?CP_UTF8:CP_ACP;int n=WideCharToMultiByte(cp,0,d.text.data(),(int)d.text.size(),nullptr,0,nullptr,cp==CP_UTF8?nullptr:&used);
 if(used){Doc wide=d;wide.encoding=1;return encode(wide);}Bytes b((d.encoding==2?3:0)+n);size_t start=0;if(d.encoding==2){b[0]=0xef;b[1]=0xbb;b[2]=0xbf;start=3;}if(n)WideCharToMultiByte(cp,0,d.text.data(),(int)d.text.size(),(char*)b.data()+start,n,nullptr,nullptr);return b;
}
struct Line {size_t begin,end;std::wstring text;};
static std::vector<Line> lines(const std::wstring&s){std::vector<Line> v;for(size_t p=0;p<s.size();){auto e=s.find(L'\n',p);e=e==std::wstring::npos?s.size():e+1;v.push_back({p,e,trim(s.substr(p,e-p))});p=e;}return v;}
static bool values(const std::wstring&s,std::array<Value,3>&out){out={};bool active=false;int sections=0;for(const auto& l:lines(s)){if(l.text.empty()||l.text[0]==L';'||l.text[0]==L'#')continue;if(l.text[0]==L'['){active=same(l.text,L"[.ShellClassInfo]");if(active&&++sections>1)return false;continue;}if(!active)continue;auto e=l.text.find(L'=');if(e==std::wstring::npos)continue;auto k=trim(l.text.substr(0,e));for(int n=0;n<3;++n)if(same(k,keys[n])){if(out[n].present)return false;out[n]={true,trim(l.text.substr(e+1))};}}return true;}
static bool equalValues(const std::array<Value,3>&a,const std::array<Value,3>&b){for(int i=0;i<3;++i)if(a[i].present!=b[i].present||a[i].text!=b[i].text)return false;return true;}
static void setValue(std::wstring&s,const wchar_t*key,const Value&v){bool active=false,section=false;size_t insert=s.size();for(const auto&l:lines(s)){if(!l.text.empty()&&l.text[0]==L'['){if(active){insert=l.begin;active=false;}if(same(l.text,L"[.ShellClassInfo]")){active=true;section=true;insert=l.end;}continue;}if(!active)continue;insert=l.end;auto e=l.text.find(L'=');if(e!=std::wstring::npos&&same(trim(l.text.substr(0,e)),key)){std::wstring end=s.substr(l.begin,l.end-l.begin);std::wstring suffix=end.size()>=2&&end.substr(end.size()-2)==L"\r\n"?L"\r\n":!end.empty()&&end.back()==L'\n'?L"\n":L"";s.replace(l.begin,l.end-l.begin,v.present?std::wstring(key)+L"="+v.text+suffix:L"");return;}}
 if(!v.present)return;if(!section){if(!s.empty()&&s.back()!=L'\n')s+=L"\r\n";s+=L"[.ShellClassInfo]\r\n";insert=s.size();}else if(insert&&s[insert-1]!=L'\n'){s.insert(insert,L"\r\n");insert+=2;}s.insert(insert,std::wstring(key)+L"="+v.text+L"\r\n");
}
static bool locate(const std::wstring&store,const std::wstring&folder,Location&l,Guard&g,std::wstring&e){
 wchar_t full[32768]{};DWORD n=GetFullPathNameW(folder.c_str(),32768,full,nullptr);if(!n||n>=32768)return fail(e,L"目录路径无效。");l.folder=full;while(l.folder.size()>3&&l.folder.back()==L'\\')l.folder.pop_back();l.attrs=GetFileAttributesW(l.folder.c_str());if(l.folder.size()<=3||l.attrs==INVALID_FILE_ATTRIBUTES||!(l.attrs&FILE_ATTRIBUTE_DIRECTORY)||(l.attrs&FILE_ATTRIBUTE_REPARSE_POINT))return fail(e,L"请选择存在的真实文件夹；磁盘根目录和目录链接不支持改角标。");
 HANDLE h=CreateFileW(l.folder.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(h==INVALID_HANDLE_VALUE)return fail(e,L"无法读取目录身份。");BY_HANDLE_FILE_INFORMATION i{};bool ok=GetFileInformationByHandle(h,&i)!=FALSE;CloseHandle(h);if(!ok)return fail(e,L"无法读取目录身份。");l.id=std::to_wstring(i.dwVolumeSerialNumber)+L"-"+std::to_wstring(i.nFileIndexHigh)+L"-"+std::to_wstring(i.nFileIndexLow);
 g.h=CreateMutexW(nullptr,FALSE,(L"Local\\FolderIcon.Badge."+l.id).c_str());if(!g.h)return fail(e,L"无法建立角标操作锁。");DWORD wait=WaitForSingleObject(g.h,0);g.held=wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED;if(!g.held)return fail(e,L"这个文件夹的角标正在由另一个窗口处理。");
 n=GetFullPathNameW(store.c_str(),32768,full,nullptr);if(!n||n>=32768)return fail(e,L"角标存储路径无效。");std::wstring root=full;DWORD attr=GetFileAttributesW(root.c_str());if(attr!=INVALID_FILE_ATTRIBUTES&&(!(attr&FILE_ATTRIBUTE_DIRECTORY)||(attr&FILE_ATTRIBUTE_REPARSE_POINT)))return fail(e,L"角标存储目录不安全。");l.dir=root+L"\\"+l.id;l.ini=l.folder+L"\\desktop.ini";l.info=l.dir+L"\\state.bin";l.original=l.dir+L"\\original.ini";l.applied=l.dir+L"\\applied.ini";l.icon=l.dir+L"\\panel.ico";attr=GetFileAttributesW(l.dir.c_str());if(attr!=INVALID_FILE_ATTRIBUTES&&(attr&FILE_ATTRIBUTE_REPARSE_POINT))return fail(e,L"角标备份目录不能是链接。");return true;
}
static bool loadState(const Location&l,State&s,Bytes&before,Bytes&after){Bytes b;bool had=false,hadBefore=false,hadAfter=false;if(!read(l.info,b,had)||!had||b.size()!=sizeof(s))return false;memcpy(&s,b.data(),sizeof(s));return s.magic==0x31424746&&s.hadIni<=1&&(s.phase>=1&&s.phase<=4)&&read(l.original,before,hadBefore)&&hadBefore&&read(l.applied,after,hadAfter)&&hadAfter;}
static bool saveState(const Location&l,const State&s){Bytes b(sizeof(s));memcpy(b.data(),&s,sizeof(s));return snapshot(l.info,b);}
static void notify(const Location&l){WritePrivateProfileStringW(nullptr,nullptr,nullptr,l.ini.c_str());SHChangeNotify(SHCNE_UPDATEITEM,SHCNF_PATHW|SHCNF_FLUSHNOWAIT,l.folder.c_str(),nullptr);}
#pragma pack(push,1)
struct IconHeader {WORD reserved=0,type=1,count=0;};
struct IconEntry {BYTE width,height,colors=0,reserved=0;WORD planes=1,bits=32;DWORD bytes,offset;};
#pragma pack(pop)
static HBITMAP resourceBitmap(const std::wstring& source,int index,int size){
 wchar_t expanded[32768]{};DWORD n=ExpandEnvironmentStringsW(source.c_str(),expanded,32768);if(!n||n>=32768)return nullptr;
 HICON icon=nullptr;if(FAILED(SHDefExtractIconW(expanded,index,0,&icon,nullptr,MAKELONG(size,0)))||!icon)return nullptr;
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
 void* bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bm=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);
 if(bm){auto old=SelectObject(dc,bm);memset(bits,0,size*size*4);bool ok=DrawIconEx(dc,0,0,icon,size,size,0,nullptr,DI_NORMAL)!=FALSE;
  // Legacy icons use an AND mask rather than alpha. Draw the mask separately.
  BYTE* p=(BYTE*)bits;bool alpha=false;for(int i=0;i<size*size;++i)if(p[i*4+3]){alpha=true;break;}
  if(ok&&!alpha){Bytes color(p,p+size*size*4);DrawIconEx(dc,0,0,icon,size,size,0,nullptr,DI_MASK);for(int i=0;i<size*size;++i)color[i*4+3]=p[i*4]?0:255;memcpy(bits,color.data(),color.size());}
  SelectObject(dc,old);if(!ok){DeleteObject(bm);bm=nullptr;}
 }DeleteDC(dc);DestroyIcon(icon);return bm;
}
static bool generate(const Location&l,bool shortcut=false,const std::wstring& resource=L"",int resourceIndex=0,bool direct=false,bool plain=false){
 const int sizes[]={16,20,24,32,40,48,64,96,128,256};std::vector<Bytes> frames;std::vector<IconEntry> entries;
 IShellItemImageFactory* factory=nullptr;if(resource.empty()&&FAILED(SHCreateItemFromParsingName(l.folder.c_str(),nullptr,IID_PPV_ARGS(&factory))))return false;
 bool ok=true;DWORD offset=sizeof(IconHeader)+sizeof(IconEntry)*10;
 for(int size:sizes){HBITMAP hb=nullptr;if(factory)factory->GetImage({size,size},(SIIGBF)(SIIGBF_ICONONLY|SIIGBF_BIGGERSIZEOK),&hb);else hb=resourceBitmap(resource,resourceIndex,size);if(!hb){ok=false;break;}
  BITMAP bm{};GetObjectW(hb,sizeof(bm),&bm);int w=bm.bmWidth,h=abs(bm.bmHeight);Bytes pixels((size_t)w*h*4);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;HDC dc=GetDC(nullptr);int got=GetDIBits(dc,hb,0,h,pixels.data(),&bi,DIB_RGB_COLORS);ReleaseDC(nullptr,dc);DeleteObject(hb);if(got!=h){ok=false;break;}
  Gdiplus::Bitmap source(w,h,w*4,PixelFormat32bppPARGB,pixels.data()),canvas(size,size,PixelFormat32bppARGB);
  {Gdiplus::Graphics g(&canvas);g.Clear(Gdiplus::Color(0,0,0,0));g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);g.DrawImage(&source,0,0,size,size);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);float d=(float)BadgeVisual::extent(size);if(!plain)BadgeVisual::draw(g,size-d,0,size,direct?BadgeVisual::Kind::DirectOpen:shortcut?BadgeVisual::Kind::Shortcut:BadgeVisual::Kind::DesktopFolder);}

  Gdiplus::BitmapData bd{};Gdiplus::Rect rect(0,0,size,size);if(canvas.LockBits(&rect,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&bd)!=Gdiplus::Ok){ok=false;break;}
  DWORD maskStride=((size+31)/32)*4;Bytes frame(sizeof(BITMAPINFOHEADER)+size*size*4+maskStride*size,0);BITMAPINFOHEADER head{};head.biSize=sizeof(head);head.biWidth=size;head.biHeight=size*2;head.biPlanes=1;head.biBitCount=32;head.biSizeImage=size*size*4;memcpy(frame.data(),&head,sizeof(head));
  for(int row=0;row<size;++row){auto src=(BYTE*)bd.Scan0+row*bd.Stride;auto dst=frame.data()+sizeof(head)+(size-1-row)*size*4;memcpy(dst,src,size*4);auto mask=frame.data()+sizeof(head)+size*size*4+(size-1-row)*maskStride;for(int col=0;col<size;++col)if(src[col*4+3]==0)mask[col/8]|=(BYTE)(0x80>>(col%8));}canvas.UnlockBits(&bd);
  IconEntry ie{};ie.width=ie.height=(BYTE)size;ie.bytes=(DWORD)frame.size();ie.offset=offset;offset+=ie.bytes;entries.push_back(ie);frames.push_back(std::move(frame));
 }
 if(factory)factory->Release();if(!ok||frames.size()!=10)return false;IconHeader hdr;hdr.count=10;Bytes ico(offset);memcpy(ico.data(),&hdr,sizeof(hdr));memcpy(ico.data()+sizeof(hdr),entries.data(),entries.size()*sizeof(IconEntry));for(size_t i=0;i<frames.size();++i)memcpy(ico.data()+entries[i].offset,frames[i].data(),frames[i].size());return snapshot(l.icon,ico);
}
// Overlay directly on our saved ARGB frames: no Shell extraction or resampling.
static bool badgeFromBase(const std::wstring&base,const std::wstring&destination,bool direct,bool shortcut=false){
 Bytes ico;bool had=false;if(!read(base,ico,had)||!had||ico.size()<sizeof(IconHeader))return false;
 IconHeader header{};memcpy(&header,ico.data(),sizeof(header));
 if(header.reserved||header.type!=1||header.count!=10||ico.size()<sizeof(header)+10*sizeof(IconEntry))return false;
 for(int i=0;i<10;++i){IconEntry entry{};memcpy(&entry,ico.data()+sizeof(header)+i*sizeof(entry),sizeof(entry));
  int size=entry.width?entry.width:256;size_t bytes=size*size*4,offset=entry.offset;
  if(entry.height!=entry.width||offset+sizeof(BITMAPINFOHEADER)+bytes>ico.size())return false;
  BITMAPINFOHEADER head{};memcpy(&head,ico.data()+offset,sizeof(head));
  if(head.biSize!=sizeof(head)||head.biWidth!=size||head.biHeight!=size*2||head.biBitCount!=32||head.biCompression!=BI_RGB)return false;
  Bytes pixels(bytes);auto frame=ico.data()+offset+sizeof(head);
  for(int y=0;y<size;++y)memcpy(pixels.data()+y*size*4,frame+(size-1-y)*size*4,size*4);
  {Gdiplus::Bitmap canvas(size,size,size*4,PixelFormat32bppARGB,pixels.data());Gdiplus::Graphics g(&canvas);
   g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);float d=(float)BadgeVisual::extent(size);
   BadgeVisual::draw(g,size-d,0,size,direct?BadgeVisual::Kind::DirectOpen:shortcut?BadgeVisual::Kind::Shortcut:BadgeVisual::Kind::DesktopFolder);g.Flush(Gdiplus::FlushIntentionSync);}
  size_t stride=((size+31)/32)*4;if(offset+sizeof(head)+bytes+stride*size>ico.size())return false;
  for(int y=0;y<size;++y){auto row=pixels.data()+y*size*4;memcpy(frame+(size-1-y)*size*4,row,size*4);
   auto mask=frame+bytes+(size-1-y)*stride;memset(mask,0,stride);for(int x=0;x<size;++x)if(!row[x*4+3])mask[x/8]|=(BYTE)(0x80>>(x%8));}
 }return snapshot(destination,ico);
}
static bool writableMetadata(const Location&l,Bytes&b,bool&had,DWORD&attr,std::wstring&e){if(!read(l.ini,b,had,&attr))return fail(e,L"无法安全读取desktop.ini（可能是链接、被占用或格式过大）。");if(had&&(attr&FILE_ATTRIBUTE_READONLY))return fail(e,L"desktop.ini设为只读，未修改。请先确认该目录的原有保护设置。");return true;}
static bool putTarget(const Location&l,const Bytes&expected,bool expectedHad,const Bytes&next,bool nextHad){Bytes current;bool had;DWORD attrs=0;if(!read(l.ini,current,had,&attrs)||had!=expectedHad||current!=expected||(had&&(attrs&FILE_ATTRIBUTE_READONLY)))return false;if(!nextHad)return !had||DeleteFileW(l.ini.c_str())!=FALSE;return replace(l.ini,next,had);}
static bool restoreImpl(const Location&l,std::wstring&e){
 if(!exists(l.info))return true;State s;Bytes original,applied,current;bool had;DWORD attr=0;if(!loadState(l,s,original,applied))return fail(e,L"恢复记录不完整，已停止，未覆盖目录配置。");if(!writableMetadata(l,current,had,attr,e))return false;
 if(s.phase==1&&had==(s.hadIni!=0)&&current==original){if(!SetFileAttributesW(l.folder.c_str(),(l.attrs&~FILE_ATTRIBUTE_READONLY)|(s.folderAttrs&FILE_ATTRIBUTE_READONLY)))return fail(e,L"恢复目录属性失败，恢复记录已保留。");if(!DeleteFileW(l.info.c_str()))return fail(e,L"无法完成恢复记录收尾，请稍后重试。");notify(l);return true;}
 Bytes restored;bool keep=s.hadIni!=0;
 if(s.phase>=3){
  // Persisted recovery targets make retry safe even after the bytes were already
  // restored. Refuse a newer external edit rather than guessing from icon keys.
  Bytes from;bool hasFrom=false,hasTo=false;keep=s.phase==4;
  if(!read(l.dir+L"\\restoring-from.ini",from,hasFrom)||!hasFrom||
     !read(l.dir+L"\\restoring-to.ini",restored,hasTo)||!hasTo)
   return fail(e,L"中断恢复记录不完整；未覆盖目录配置。");
  bool alreadyDone=had==keep&&current==restored;
  if(!alreadyDone){
   if(!had||current!=from)return fail(e,L"中断恢复后目录配置又被修改，已停止以保护新内容。");
   if(!putTarget(l,current,had,restored,keep))return fail(e,L"继续恢复写入失败，恢复记录已保留。");
  }
 }else{
  if(current==applied&&had){restored=original;}
  else {
   Doc oldDoc,appliedDoc,now;std::array<Value,3>oldValues,ours,newValues;
   if(!had||!decode(original,oldDoc)||!decode(applied,appliedDoc)||!decode(current,now)||
      !values(oldDoc.text,oldValues)||!values(appliedDoc.text,ours)||!values(now.text,newValues)||!equalValues(ours,newValues))
    return fail(e,L"目录图标在加角标后被修改过；为保护新设置，已停止恢复。原图标备份仍保留。");
   for(int i=0;i<3;++i)setValue(now.text,keys[i],oldValues[i]);restored=encode(now);keep=true;
  }
  if(!snapshot(l.dir+L"\\restoring-from.ini",current)||!snapshot(l.dir+L"\\restoring-to.ini",restored))
   return fail(e,L"无法保存恢复进度；目录配置未修改。");
  s.phase=keep?4:3;
  if(!saveState(l,s))return fail(e,L"无法保存恢复阶段；目录配置未修改。");
  if(!putTarget(l,current,had,restored,keep))return fail(e,L"恢复写入失败或目录配置正在被其他程序修改；恢复记录已保留。");
 }
 if(keep){DWORD now=GetFileAttributesW(l.ini.c_str());DWORD mask=FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM;DWORD wanted=s.hadIni?s.iniAttrs:attr;if(!SetFileAttributesW(l.ini.c_str(),(now&~mask)|(wanted&mask)))return fail(e,L"图标已恢复，但文件属性未恢复；恢复记录已保留。");}
 if(!SetFileAttributesW(l.folder.c_str(),(GetFileAttributesW(l.folder.c_str())&~FILE_ATTRIBUTE_READONLY)|(s.folderAttrs&FILE_ATTRIBUTE_READONLY)))return fail(e,L"图标已恢复，但目录属性未恢复；恢复记录已保留。");
 if(!DeleteFileW(l.info.c_str()))return fail(e,L"图标已恢复，但无法完成恢复记录收尾。");notify(l);return true;
}
} // namespace
static bool applyInternal(const std::wstring&store,const std::wstring&folder,bool direct,std::wstring&e){
 e.clear();Location l;Guard lock;if(!locate(store,folder,l,lock,e))return false;
 if(exists(l.info)){State s;Bytes original,applied,current;bool had;Doc now,ours;std::array<Value,3>a,b;if(!loadState(l,s,original,applied)||!read(l.ini,current,had)||!had||!decode(current,now)||!decode(applied,ours)||!values(now.text,a)||!values(ours.text,b)||!equalValues(a,b))return fail(e,L"已有角标恢复记录，但当前图标不同。请先恢复，避免覆盖你修改的新图标。");if(s.phase!=2)return fail(e,L"上次角标操作未完成，请先点击恢复原图标。");return true;}
 State s;s.folderAttrs=l.attrs;Bytes original;bool had;DWORD attr=0;if(!writableMetadata(l,original,had,attr,e))return false;s.hadIni=had;s.iniAttrs=attr;Doc doc;std::array<Value,3>v;if(!decode(original,doc)||!values(doc.text,v))return fail(e,L"desktop.ini编码不支持或图标键重复，未修改。");
 int created=SHCreateDirectoryExW(nullptr,l.dir.c_str(),nullptr);if(created!=ERROR_SUCCESS&&created!=ERROR_ALREADY_EXISTS)return fail(e,L"无法创建程序旁的角标备份目录。");
 Bytes pathBytes((BYTE*)l.folder.data(),(BYTE*)l.folder.data()+l.folder.size()*sizeof(wchar_t));if(!snapshot(l.dir+L"\\folder.path",pathBytes))return fail(e,L"无法保存目录定位记录，原目录未修改。");
 Location clean=l;clean.icon=l.dir+L"\\base.ico";
 if(!generate(clean,false,L"",0,false,true)||!badgeFromBase(clean.icon,l.icon,direct))return fail(e,L"无法生成目录图标，原配置未修改。");setValue(doc.text,L"IconResource",{});setValue(doc.text,L"IconFile",{true,l.icon});setValue(doc.text,L"IconIndex",{true,L"0"});Bytes applied=encode(doc);
 if(!snapshot(l.original,original)||!snapshot(l.applied,applied)||!saveState(l,s))return fail(e,L"无法完整保存恢复记录，原目录配置未修改。");
 if(!putTarget(l,original,had,applied,true)){std::wstring ignored;restoreImpl(l,ignored);return fail(e,L"写入目录外观失败或配置被同时修改；未强行覆盖。");}
 bool attrsOk=SetFileAttributesW(l.ini.c_str(),(had?attr:FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)&&SetFileAttributesW(l.folder.c_str(),l.attrs|FILE_ATTRIBUTE_READONLY);s.phase=2;
 if(!attrsOk||!saveState(l,s)){std::wstring rollback;bool reverted=restoreImpl(l,rollback);e=reverted?L"设置角标未完成，已恢复原外观。":L"设置角标未完成，恢复记录已保留。请点击恢复原图标。";return false;}
 notify(l);return true;
}
bool apply(const std::wstring&store,const std::wstring&folder,std::wstring&e){return applyInternal(store,folder,false,e);}
bool applyMode(const std::wstring&store,const std::wstring&folder,bool direct,std::wstring&e){
 Location l;Guard guard;if(!locate(store,folder,l,guard,e))return false;
 if(exists(l.info)){
  State s;Bytes original,applied,current;bool had=false;
  if(!loadState(l,s,original,applied)||s.phase!=2||!read(l.ini,current,had)||!had||current!=applied)return fail(e,L"目录图标已修改或恢复记录未完成，请先恢复原图标。");
  auto clean=l.dir+L"\\base.ico";
  if(!exists(clean)){
   Doc doc;std::array<Value,3> valuesBefore;if(!decode(original,doc)||!values(doc.text,valuesBefore))return fail(e,L"原图标记录无法读取。");
   std::wstring source;int index=0;
   if(valuesBefore[0].present){source=valuesBefore[0].text;auto comma=source.find_last_of(L',');if(comma!=std::wstring::npos){index=_wtoi(source.substr(comma+1).c_str());source.resize(comma);}}
   else if(valuesBefore[1].present){source=valuesBefore[1].text;if(valuesBefore[2].present)index=_wtoi(valuesBefore[2].text.c_str());}
   if(source.size()>1&&source.front()==L'"'&&source.back()==L'"')source=source.substr(1,source.size()-2);
   if(source.empty()){SHSTOCKICONINFO stock{sizeof(stock)};if(FAILED(SHGetStockIconInfo(SIID_FOLDER,SHGSI_ICONLOCATION,&stock)))return fail(e,L"无法读取原文件夹图标。");source=stock.szPath;index=stock.iIcon;}
   else if(PathIsRelativeW(source.c_str()))source=l.folder+L"\\"+source;
   Location base=l;base.icon=clean;if(!generate(base,false,source,index,false,true))return fail(e,L"无法生成原图标副本。");
  }
  if(!badgeFromBase(clean,l.icon,direct))return fail(e,L"无法切换角标。");notify(l);return true;
 }
 if(guard.held){ReleaseMutex(guard.h);guard.held=false;}
 return applyInternal(store,folder,direct,e);
}
bool modeIcon(const std::wstring&path,const std::wstring&source,int index,const std::wstring&destination,bool direct,std::wstring&e){
 e.clear();if(exists(destination))return fail(e,L"图标目标已存在。");Location l;l.folder=path;l.icon=destination;
 if(index==0&&!source.empty()&&badgeFromBase(source,destination,direct,true))return true;
 return generate(l,true,source,index,direct)||fail(e,L"无法读取原图标，未修改入口。");
}
bool shortcutIcon(const std::wstring&folder,const std::wstring&destination,std::wstring&e){
 e.clear();if(exists(destination))return fail(e,L"图标目标已存在，未覆盖。");Location l;l.folder=folder;l.icon=destination;
 if(!generate(l,true))return fail(e,L"无法生成快捷入口图标，源目录未修改。");return true;
}
bool restore(const std::wstring&store,const std::wstring&folder,std::wstring&e){e.clear();Location l;Guard lock;if(!locate(store,folder,l,lock,e))return false;return restoreImpl(l,e);}
bool resourceIcon(const std::wstring&source,int index,const std::wstring&destination,std::wstring&e){
 e.clear();if(exists(destination))return fail(e,L"图标目标已存在，未覆盖。");Location l;l.icon=destination;
 if(source.empty()||!generate(l,true,source,index))return fail(e,L"无法读取快捷方式当前图标；未恢复成默认图标，也未修改快捷方式。");return true;
}
std::vector<std::wstring> recordedFolders(const std::wstring&store,size_t&unresolved){
 unresolved=0;std::vector<std::wstring> result;
 DWORD a=GetFileAttributesW(store.c_str());if(a==INVALID_FILE_ATTRIBUTES)return result;
 if(!(a&FILE_ATTRIBUTE_DIRECTORY)||(a&FILE_ATTRIBUTE_REPARSE_POINT)){++unresolved;return result;}
 WIN32_FIND_DATAW fd{};HANDLE find=FindFirstFileW((store+L"\\*").c_str(),&fd);if(find==INVALID_HANDLE_VALUE){++unresolved;return result;}
 struct Volume{DWORD serial;HANDLE hint;};std::vector<Volume> volumes;bool opened=false;
 auto openVolumes=[&](){if(opened)return;opened=true;DWORD mask=GetLogicalDrives();for(int i=0;i<26;++i)if(mask&(1u<<i)){wchar_t root[]={wchar_t(L'A'+i),L':',L'\\',0};if(GetDriveTypeW(root)!=DRIVE_FIXED)continue;
  HANDLE h=CreateFileW(root,0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);BY_HANDLE_FILE_INFORMATION info{};
  if(h!=INVALID_HANDLE_VALUE){if(GetFileInformationByHandle(h,&info))volumes.push_back({info.dwVolumeSerialNumber,h});else CloseHandle(h);}
 }};
 size_t visited=0;do{
  if(fd.cFileName[0]==L'.'||!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(fd.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))continue;
  if(++visited>4096){++unresolved;break;}std::wstring id=fd.cFileName,dir=store+L"\\"+id;
  if(!exists(dir+L"\\state.bin"))continue;
  std::wstring path;Bytes raw;bool had=false;if(read(dir+L"\\folder.path",raw,had)&&had&&!raw.empty()&&raw.size()%2==0&&raw.size()<65536)path.assign((wchar_t*)raw.data(),raw.size()/2);
  auto valid=[&](const std::wstring&p){Location loc;Guard guard;std::wstring e;return !p.empty()&&locate(store,p,loc,guard,e)&&loc.id==id;};
  if(!valid(path)){
   path.clear();unsigned long serial=0,high=0,low=0;wchar_t extra=0;
   if(swscanf(id.c_str(),L"%lu-%lu-%lu%c",&serial,&high,&low,&extra)==3){openVolumes();for(const auto&v:volumes)if(v.serial==serial){
    FILE_ID_DESCRIPTOR desc{};desc.dwSize=sizeof(desc);desc.Type=FileIdType;desc.FileId.HighPart=high;desc.FileId.LowPart=low;
    HANDLE h=OpenFileById(v.hint,&desc,0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT);
    if(h==INVALID_HANDLE_VALUE)continue;wchar_t full[32768]{};DWORD n=GetFinalPathNameByHandleW(h,full,32768,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);CloseHandle(h);
    if(n&&n<32768){std::wstring candidate=full;if(candidate.rfind(L"\\\\?\\",0)==0)candidate.erase(0,4);if(valid(candidate)){path=candidate;break;}}
   }}
  }
  if(path.empty())++unresolved;else result.push_back(path);
 }while(FindNextFileW(find,&fd));FindClose(find);for(auto&v:volumes)CloseHandle(v.hint);return result;
}

}
