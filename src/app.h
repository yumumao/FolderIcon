#pragma once
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <oleacc.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <algorithm>
#include "core.h"
struct Config{int icon=96,cellW=128,cellH=144,columns=4,opacity=88,radius=20,blur=1,resident=0,followDesktop=1;COLORREF background=RGB(35,40,48);};
struct Entry{std::wstring id,name,target,icon;};
extern Config cfg;extern std::vector<Entry> entries;extern std::wstring baseDir,exePath,iniPath;extern HWND controller,panelWindow,settingsWindow;extern bool residentMode,quitting;extern HINSTANCE instance;
constexpr UINT WM_TRAY=WM_APP+1,WM_HOOK=WM_APP+2,WM_CONFIG=WM_APP+3,WM_LAUNCH=WM_APP+4;
constexpr UINT TIMER_CLICK=11;constexpr wchar_t CONTROL_CLASS[]=L"FolderIconCN.Controller";
void loadConfig();bool saveConfig();Metrics desktopMetrics(bool* actual=nullptr);int desktopEntryAt(POINT pt);HBITMAP shellBitmap(const std::wstring& path,int size);void openEntry(int id);void showPanel(const std::wstring& path,POINT anchor);void closePanel();void showSettings();void launchSelf(const std::wstring& args);bool applyStartup(bool enabled);void setupEntries();bool isDesktopPoint(POINT pt);void diagnose(const std::wstring& file);
