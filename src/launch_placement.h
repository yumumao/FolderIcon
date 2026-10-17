#pragma once
#include <windows.h>
#include <functional>
#include <string>
namespace LaunchPlacement {
// Called on the launch worker, never from the mouse hook. The dispatcher must
// pass the monitor hint to Shell and transfer any process handle to this call.
using Dispatcher=std::function<bool(HMONITOR,HANDLE*)>;
bool open(const std::wstring&path,bool directory,POINT anchor,const Dispatcher&dispatch);
bool moveWindow(HWND window,HMONITOR monitor);
}
