#pragma once
#include <string>
#include <vector>
namespace FolderBadge {
// Appearance operations and bounded enumeration of this application's recovery records.
bool apply(const std::wstring& store,const std::wstring& folder,std::wstring& error);
bool shortcutIcon(const std::wstring& folder,const std::wstring& destination,std::wstring& error);
bool resourceIcon(const std::wstring& source,int index,const std::wstring& destination,std::wstring& error);
std::vector<std::wstring> recordedFolders(const std::wstring& store,size_t& unresolved);
bool applyMode(const std::wstring& store,const std::wstring& folder,bool direct,std::wstring& error);
bool modeIcon(const std::wstring& path,const std::wstring& source,int index,const std::wstring& destination,bool direct,std::wstring& error);
bool restore(const std::wstring& store,const std::wstring& folder,std::wstring& error);
}
