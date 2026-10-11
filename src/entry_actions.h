#pragma once
#include "app.h"
namespace EntryActions {
bool realFolder(const std::wstring& path);
bool setPanelExpansion(const std::wstring& path,bool enabled,std::wstring& error);
bool enableDesktopFolder(const std::wstring& path,bool& alreadyEnabled,std::wstring& warning,std::wstring& error);
FolderRules::Rules mergeDesktopRules(const FolderRules::Rules& live,const FolderRules::Rules& draft);
std::wstring syncDesktopBadges(const FolderRules::Rules& before,const FolderRules::Rules& after);
bool createDesktopEntry(const std::wstring& folder,const std::wstring& desktop,std::wstring& shortcut,std::wstring& error);
bool removeDesktopEntry(size_t index,std::wstring& warning,std::wstring& error);
}