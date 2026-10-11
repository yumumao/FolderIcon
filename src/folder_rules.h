#pragma once
#include <string>
#include <vector>
namespace FolderRules {
struct Rule { std::wstring path; bool panel=false; bool desktop=false; };
using Rules=std::vector<Rule>;
std::wstring normalize(const std::wstring& path);
bool contains(const Rules& rules,const std::wstring& path,bool desktop);
bool set(Rules& rules,const std::wstring& path,bool panel,bool desktop);
bool load(const std::wstring& file,Rules& out);
bool save(const std::wstring& file,const Rules& rules);
std::wstring identity(const std::wstring& path);
bool canEnter(const std::vector<std::wstring>& ancestors,const std::wstring& next);
}
