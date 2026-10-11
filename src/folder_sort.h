#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlwapi.h>
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace FolderSort {
enum class Mode { Name, Modified, Custom };
struct Preferences { Mode mode=Mode::Name; bool descending=false; std::vector<std::wstring> custom; };
struct Item { std::wstring id,name; bool directory=false; std::uint64_t modified=0; };
inline int compareName(const Item& a,const Item& b) {
 int n=StrCmpLogicalW(a.name.c_str(),b.name.c_str());
 if(!n)n=CompareStringOrdinal(a.id.c_str(),-1,b.id.c_str(),-1,FALSE)-CSTR_EQUAL;
 return n;
}
inline std::vector<size_t> indices(const std::vector<Item>& items,const Preferences& p) {
 std::vector<size_t> result(items.size());std::iota(result.begin(),result.end(),0);
 std::unordered_map<std::wstring,size_t> ranks;
 for(size_t i=0;i<p.custom.size();++i)ranks.emplace(p.custom[i],i);
 std::sort(result.begin(),result.end(),[&](size_t ai,size_t bi){
  const auto&a=items[ai];const auto&b=items[bi];
  if(p.mode==Mode::Custom){
   auto ar=ranks.find(a.id),br=ranks.find(b.id);
   if(ar!=ranks.end()||br!=ranks.end()){
    if(ar==ranks.end())return false;if(br==ranks.end())return true;
    if(ar->second!=br->second)return ar->second<br->second;
   }
  }
  // Ascending or Custom: folders first; Descending (Name/Modified): files first (full reverse).
  if(a.directory!=b.directory){
   bool foldersFirst=(p.mode==Mode::Custom||!p.descending);
   return foldersFirst?a.directory:!a.directory;
  }
  if(p.mode==Mode::Modified&&a.modified!=b.modified)return p.descending?a.modified>b.modified:a.modified<b.modified;
  int n=compareName(a,b);return p.mode==Mode::Name&&p.descending?n>0:n<0;
 });return result;
}
inline std::vector<std::wstring> mergeCustom(const std::vector<std::wstring>& old,const std::vector<std::wstring>& visible){
 std::unordered_set<std::wstring> shown(visible.begin(),visible.end()),added;std::vector<std::wstring> out;size_t next=0;
 auto append=[&](const std::wstring&s){if(added.insert(s).second)out.push_back(s);};
 // Filtered-out items keep their saved slots; new items append at the end.
 for(const auto&s:old){if(shown.count(s)){if(next<visible.size())append(visible[next++]);}else append(s);}
 while(next<visible.size())append(visible[next++]);return out;
}
template<class T> bool move(std::vector<T>& values,int from,int to){
 if(from<0||to<0||from>=(int)values.size()||to>=(int)values.size()||from==to)return false;
 auto value=std::move(values[from]);values.erase(values.begin()+from);values.insert(values.begin()+to,std::move(value));return true;
}
std::wstring filePath(const std::wstring& root,const std::wstring& folder);
Preferences load(const std::wstring& root,const std::wstring& folder);
bool save(const std::wstring& root,const std::wstring& folder,const Preferences& value);
}
