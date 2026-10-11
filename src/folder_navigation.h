#pragma once
#include <string>
#include <vector>
// Only the visited chain is retained. No recursive directory enumeration.
class FolderNavigation {
 struct Frame { std::wstring path, identity; };
 std::vector<Frame> chain;
public:
 void clear(){chain.clear();}
 void reset(const std::wstring& path,const std::wstring& identity){chain={{path,identity}};}
 bool hasParent() const{return chain.size()>1;}
 std::wstring path() const{return chain.empty()?L"":chain.back().path;}
 bool enter(const std::wstring& path,const std::wstring& identity){
  if(chain.empty()||chain.back().identity.empty()||identity.empty()||chain.size()>=32)return false;
  for(const auto& frame:chain)if(frame.identity==identity)return false;
  chain.push_back({path,identity});return true;
 }
 bool back(){if(!hasParent())return false;chain.pop_back();return true;}
};
