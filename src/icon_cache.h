#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
namespace IconCache {
// root is the application's own cache directory. Returned bitmaps belong to caller.
std::wstring filePath(const std::wstring& root,const std::wstring& source,int size);
HBITMAP load(const std::wstring& root,const std::wstring& source,int size);
bool store(const std::wstring& root,const std::wstring& source,int size,HBITMAP image);
bool rebuild(const std::wstring& root);
}
