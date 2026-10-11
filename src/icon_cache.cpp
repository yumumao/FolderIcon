#include "icon_cache.h"
#include <bcrypt.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <cstdint>
namespace IconCache {
namespace {
constexpr char magic[8]={'F','I','C','N','I','C','O','1'};
constexpr uint64_t budget=64ull*1024*1024;
constexpr size_t maxFiles=1024;
struct Header {
 char signature[8];
 uint32_t version,requested,width,height,attributes,bytes;
 uint64_t modified,length;
 BYTE key[32],checksum[32];
};
struct File {
 HANDLE h=INVALID_HANDLE_VALUE;
 explicit File(HANDLE value):h(value){}
 ~File(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
 bool valid()const{return h!=INVALID_HANDLE_VALUE;}
};
bool digest(const void* data,size_t bytes,BYTE out[32]) {
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 if(bytes>UINT32_MAX||BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
 bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
 if(ok)ok=BCryptHashData(hash,(PUCHAR)data,(ULONG)bytes,0)>=0&&BCryptFinishHash(hash,out,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);return ok;
}
std::wstring hex(const BYTE* bytes) {
 const wchar_t* digits=L"0123456789abcdef";std::wstring s;
 for(int i=0;i<32;i++){s+=digits[bytes[i]>>4];s+=digits[bytes[i]&15];}return s;
}
std::wstring absolute(const std::wstring& path) {
 DWORD n=GetFullPathNameW(path.c_str(),0,nullptr,nullptr);if(!n)return {};
 std::vector<wchar_t> b(n);if(!GetFullPathNameW(path.c_str(),n,b.data(),nullptr))return {};
 std::wstring s=b.data();std::transform(s.begin(),s.end(),s.begin(),[](wchar_t c){return (wchar_t)towlower(c);});return s;
}
bool sourceKey(const std::wstring& path,int size,BYTE key[32]) {
 if(size<1||size>256)return false;auto s=absolute(path);if(s.empty())return false;
 s+=L"|"+std::to_wstring(size);return digest(s.data(),s.size()*sizeof(wchar_t),key);
}
uint64_t number(DWORD high,DWORD low){return (uint64_t(high)<<32)|low;}
bool metadata(const std::wstring& path,Header& h) {
 WIN32_FILE_ATTRIBUTE_DATA a{};if(!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&a))return false;
 h.attributes=a.dwFileAttributes;h.modified=number(a.ftLastWriteTime.dwHighDateTime,a.ftLastWriteTime.dwLowDateTime);
 h.length=number(a.nFileSizeHigh,a.nFileSizeLow);return true;
}
bool directory(const std::wstring& p,bool create) {
 DWORD a=GetFileAttributesW(p.c_str());
 if(a==INVALID_FILE_ATTRIBUTES&&create){CreateDirectoryW(p.c_str(),nullptr);a=GetFileAttributesW(p.c_str());}
 return a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_DIRECTORY)&&!(a&FILE_ATTRIBUTE_REPARSE_POINT);
}
bool ready(const std::wstring& root,bool create) {
 return directory(root,create)&&directory(root+L"\\icons-v1",create);
}
bool readAll(HANDLE h,void* p,DWORD n){DWORD got=0;return ReadFile(h,p,n,&got,nullptr)&&got==n;}
bool writeAll(HANDLE h,const void* p,DWORD n){DWORD got=0;return WriteFile(h,p,n,&got,nullptr)&&got==n;}
std::wstring nameFor(const BYTE key[32]){return L"fi-"+hex(key)+L".fic";}
bool ownedName(const wchar_t* name) {
 std::wstring n=name;if(n.size()!=71||n.substr(0,3)!=L"fi-"||n.substr(67)!=L".fic")return false;
 for(int i=3;i<67;i++)if(!((n[i]>=L'0'&&n[i]<=L'9')||(n[i]>=L'a'&&n[i]<=L'f')))return false;
 return true;
}
struct Lock {
 HANDLE handle=nullptr;bool held=false;
 explicit Lock(const std::wstring& root) {
  BYTE hash[32];auto s=absolute(root);if(s.empty()||!digest(s.data(),s.size()*sizeof(wchar_t),hash))return;
  handle=CreateMutexW(nullptr,FALSE,(L"Local\\FolderIconCN.Cache."+hex(hash)).c_str());
  if(handle){DWORD r=WaitForSingleObject(handle,100);held=r==WAIT_OBJECT_0||r==WAIT_ABANDONED;}
 }
 ~Lock(){if(held)ReleaseMutex(handle);if(handle)CloseHandle(handle);}
};
struct CachedFile{std::wstring path;uint64_t bytes,time;};
bool list(const std::wstring& root,std::vector<CachedFile>& files) {
 WIN32_FIND_DATAW f{};auto dir=root+L"\\icons-v1\\";
 HANDLE search=FindFirstFileW((dir+L"fi-*.fic").c_str(),&f);
 if(search==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND;
 do {
  if((f.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!ownedName(f.cFileName))continue;
  // Namespace + format signature + filename/key match, never arbitrary *.fic files.
  File input(CreateFileW((dir+f.cFileName).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
  Header h{};
  if(input.valid()&&readAll(input.h,&h,sizeof(h))&&!memcmp(h.signature,magic,8)&&(h.version==1||h.version==2)&&nameFor(h.key)==f.cFileName)
   files.push_back({dir+f.cFileName,number(f.nFileSizeHigh,f.nFileSizeLow),number(f.ftLastWriteTime.dwHighDateTime,f.ftLastWriteTime.dwLowDateTime)});
 }while(FindNextFileW(search,&f));
 bool ok=GetLastError()==ERROR_NO_MORE_FILES;FindClose(search);return ok;
}
bool makeRoom(const std::wstring& root,const std::wstring& replacing,uint64_t incoming) {
 std::vector<CachedFile> files;if(!list(root,files))return false;uint64_t total=incoming;size_t count=1;
 for(auto& f:files)if(f.path!=replacing){total+=f.bytes;count++;}
 std::sort(files.begin(),files.end(),[](const CachedFile& a,const CachedFile& b){return a.time<b.time;});
 for(auto& f:files){if(total<=budget&&count<=maxFiles)break;if(f.path==replacing)continue;
  if(DeleteFileW(f.path.c_str())){total-=f.bytes;count--;}}
 return total<=budget&&count<=maxFiles;
}
}
std::wstring filePath(const std::wstring& root,const std::wstring& source,int size) {
 BYTE key[32];if(!sourceKey(source,size,key))return {};
 return root+L"\\icons-v1\\"+nameFor(key);
}
HBITMAP load(const std::wstring& root,const std::wstring& source,int size) {
 Header current{};if(!ready(root,false)||!sourceKey(source,size,current.key)||!metadata(source,current))return nullptr;
 auto path=root+L"\\icons-v1\\"+nameFor(current.key);
 DWORD attr=GetFileAttributesW(path.c_str());if(attr==INVALID_FILE_ATTRIBUTES||(attr&FILE_ATTRIBUTE_REPARSE_POINT))return nullptr;
 File input(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
 Header h{};LARGE_INTEGER length{};
 if(!input.valid()||!GetFileSizeEx(input.h,&length)||!readAll(input.h,&h,sizeof(h)))return nullptr;
 if(memcmp(h.signature,magic,8)||h.version!=2||h.requested!=(uint32_t)size||memcmp(h.key,current.key,32)||
    h.modified!=current.modified||h.length!=current.length||h.attributes!=current.attributes||
    h.width<1||h.height<1||h.width>512||h.height>512||h.bytes!=h.width*h.height*4||
    length.QuadPart!=(LONGLONG)(sizeof(h)+h.bytes))return nullptr;
 std::vector<BYTE> pixels(h.bytes);BYTE check[32];
 if(!readAll(input.h,pixels.data(),h.bytes)||!digest(pixels.data(),pixels.size(),check)||memcmp(check,h.checksum,32))return nullptr;
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=h.width;bi.bmiHeader.biHeight=-(LONG)h.height;
 bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;void* bits=nullptr;
 HBITMAP result=CreateDIBSection(nullptr,&bi,DIB_RGB_COLORS,&bits,nullptr,0);
 if(result)memcpy(bits,pixels.data(),pixels.size());return result;
}
bool store(const std::wstring& root,const std::wstring& source,int size,HBITMAP image) {
 Header h{};BITMAP bm{};
 if(!image||!sourceKey(source,size,h.key)||!metadata(source,h)||!GetObjectW(image,sizeof(bm),&bm)||
    bm.bmWidth<1||bm.bmHeight<1||bm.bmWidth>512||bm.bmHeight>512)return false;
 memcpy(h.signature,magic,8);h.version=2;h.requested=size;h.width=bm.bmWidth;h.height=bm.bmHeight;h.bytes=h.width*h.height*4;
 std::vector<BYTE> pixels(h.bytes);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
 bi.bmiHeader.biWidth=h.width;bi.bmiHeader.biHeight=-(LONG)h.height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
 // GetObject(DIBSECTION) reports a positive height even for top-down DIBs.
 // Ask GDI for top-down rows instead of guessing their physical memory order.
 // This preserves the tested 32-bit BGRA/alpha for both DIB orientations.
 GdiFlush();HDC dc=GetDC(nullptr);
 int got=GetDIBits(dc,image,0,h.height,pixels.data(),&bi,DIB_RGB_COLORS);ReleaseDC(nullptr,dc);
 if(got!=(int)h.height)return false;
 if(!digest(pixels.data(),pixels.size(),h.checksum))return false;
 Lock lock(root);if(!lock.held||!ready(root,true))return false;
 auto target=root+L"\\icons-v1\\"+nameFor(h.key);
 if(!makeRoom(root,target,sizeof(h)+h.bytes))return false;
 static LONG serial=0;
 auto tmp=target+L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L"."+std::to_wstring(InterlockedIncrement(&serial))+L".tmp";
 bool ok=false;
 {File output(CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
  if(!output.valid())return false;
  ok=writeAll(output.h,&h,sizeof(h))&&writeAll(output.h,pixels.data(),h.bytes);
 }
 if(ok)ok=MoveFileExW(tmp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING)!=0;
 if(!ok)DeleteFileW(tmp.c_str());return ok;
}
bool rebuild(const std::wstring& root) {
 Lock lock(root);if(!lock.held||!ready(root,true))return false;
 std::vector<CachedFile> files;if(!list(root,files))return false;
 bool ok=true;for(auto& f:files)if(!DeleteFileW(f.path.c_str()))ok=false;return ok;
}
}
