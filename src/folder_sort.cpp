#include "folder_sort.h"
#include <bcrypt.h>
#include <cstring>
namespace FolderSort {
namespace {
constexpr DWORD MaxBytes=16*1024*1024,MaxItems=100000;
std::wstring canonical(const std::wstring& path){
 DWORD n=GetFullPathNameW(path.c_str(),0,nullptr,nullptr);if(!n||n>32768)return {};
 std::wstring full(n,L'\0');DWORD got=GetFullPathNameW(path.c_str(),n,&full[0],nullptr);if(!got||got>=n)return {};
 full.resize(got);while(full.size()>3&&(full.back()==L'\\'||full.back()==L'/'))full.pop_back();
 for(auto&c:full)if(c==L'/')c=L'\\';
 int len=LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_UPPERCASE,full.data(),(int)full.size(),nullptr,0,nullptr,nullptr,0);
 if(!len)return {};std::wstring upper(len,L'\0');
 if(!LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_UPPERCASE,full.data(),(int)full.size(),&upper[0],len,nullptr,nullptr,0))return {};
 return upper;
}
std::wstring hashed(const std::wstring& name){
 if(name.empty())return {};BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;DWORD size=0,got=0;BYTE digest[32];std::wstring out;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
 if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&size,sizeof(size),&got,0)>=0){
  std::vector<BYTE> object(size);
  if(BCryptCreateHash(alg,&hash,object.data(),size,nullptr,0,0)>=0){
   if(BCryptHashData(hash,(PUCHAR)name.data(),(ULONG)(name.size()*sizeof(wchar_t)),0)>=0&&BCryptFinishHash(hash,digest,32,0)>=0){
    const wchar_t* digits=L"0123456789abcdef";for(BYTE b:digest){out+=digits[b>>4];out+=digits[b&15];}
   }BCryptDestroyHash(hash);
  }
 }BCryptCloseAlgorithmProvider(alg,0);return out;
}
void number(std::vector<BYTE>& bytes,DWORD n){for(int i=0;i<4;++i)bytes.push_back((BYTE)(n>>(i*8)));}
bool text(std::vector<BYTE>& bytes,const std::wstring& s){
 if(s.size()>32768||bytes.size()+4+s.size()*2>MaxBytes)return false;
 number(bytes,(DWORD)s.size());const auto* p=(const BYTE*)s.data();bytes.insert(bytes.end(),p,p+s.size()*2);return true;
}
struct Reader {
 const std::vector<BYTE>& bytes;size_t at=0;bool ok=true;
 DWORD number(){if(at+4>bytes.size()){ok=false;return 0;}DWORD n=0;for(int i=0;i<4;++i)n|=(DWORD)bytes[at++]<<(i*8);return n;}
 std::wstring text(){DWORD n=number();if(!ok||n>32768||at+(size_t)n*2>bytes.size()){ok=false;return {};}
  std::wstring s(n,L'\0');if(n)memcpy(&s[0],bytes.data()+at,n*2);at+=n*2;return s;}
};
}
std::wstring filePath(const std::wstring& root,const std::wstring& folder){auto key=hashed(canonical(folder));return key.empty()?L"":root+L"\\"+key+L".order";}
Preferences load(const std::wstring& root,const std::wstring& folder){
 Preferences result;auto path=filePath(root,folder);if(path.empty())return result;
 HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(h==INVALID_HANDLE_VALUE)return result;LARGE_INTEGER size{};DWORD got=0;std::vector<BYTE> bytes;
 bool ok=GetFileSizeEx(h,&size)&&size.QuadPart>=20&&size.QuadPart<=MaxBytes;
 if(ok){bytes.resize((size_t)size.QuadPart);ok=ReadFile(h,bytes.data(),(DWORD)bytes.size(),&got,nullptr)&&got==bytes.size();}CloseHandle(h);if(!ok)return result;
 Reader r{bytes};DWORD magic=r.number(),mode=r.number(),desc=r.number(),count=r.number();auto name=r.text();
 if(!r.ok||magic!=0x31534f46||mode>2||desc>1||count>MaxItems||name!=canonical(folder))return result;
 Preferences parsed;parsed.mode=(Mode)mode;parsed.descending=desc!=0;
 for(DWORD i=0;i<count&&r.ok;++i)parsed.custom.push_back(r.text());
 return r.ok&&r.at==bytes.size()?parsed:result;
}
bool save(const std::wstring& root,const std::wstring& folder,const Preferences& p){
 if((unsigned)p.mode>2||p.custom.size()>MaxItems)return false;
 auto name=canonical(folder),path=filePath(root,folder);if(name.empty()||path.empty())return false;
 std::vector<BYTE> bytes;number(bytes,0x31534f46);number(bytes,(DWORD)p.mode);number(bytes,p.descending?1:0);number(bytes,(DWORD)p.custom.size());
 if(!text(bytes,name))return false;for(const auto&s:p.custom)if(!text(bytes,s))return false;
 if(!CreateDirectoryW(root.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;
 auto temp=path+L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L".tmp";
 HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;bool ok=WriteFile(h,bytes.data(),(DWORD)bytes.size(),&written,nullptr)&&written==bytes.size()&&FlushFileBuffers(h);CloseHandle(h);
 if(ok)ok=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 if(!ok)DeleteFileW(temp.c_str());return ok;
}
}
