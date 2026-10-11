#pragma once
#include <string>
namespace ContextMenu {
const wchar_t* registrySubKey();
std::wstring commandLine(const std::wstring& executable);
std::wstring commandLine(const std::wstring& executable, const std::wstring& action);
bool isDirectDesktopChild(const std::wstring& folder,const std::wstring& desktop);
bool sync(bool enabled,const std::wstring& executable,std::wstring& error);
}