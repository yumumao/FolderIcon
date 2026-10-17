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
#include "folder_rules.h"
struct Config{int icon=96,cellW=128,cellH=144,columns=4,opacity=88,radius=20,blur=1,autoStart=0,contextMenu=0,followDesktop=1,followFont=1,fontSize=12,textGap=3,expandDelay=80,dotHidden=1,showHidden=0;COLORREF background=RGB(35,40,48);std::wstring desktopPath;};
struct DesktopFont{std::wstring face;int pixels,points;};
DesktopFont desktopFont(UINT dpi=0);UINT monitorDpi(POINT pt);
HICON applicationIcon(int size);
struct Entry{std::wstring id,name,target,icon,desktop;};
extern Config cfg;extern std::vector<Entry> entries;extern std::wstring baseDir,exePath,iniPath;extern HWND controller,panelWindow,settingsWindow;extern bool residentMode,quitting;extern HINSTANCE instance;
constexpr UINT WM_TRAY=WM_APP+1,WM_HOOK=WM_APP+2,WM_CONFIG=WM_APP+3,WM_LAUNCH=WM_APP+4;
constexpr UINT TIMER_CLICK=11;constexpr wchar_t CONTROL_CLASS[]=L"FolderIconCN.Controller";
extern FolderRules::Rules folderRules;extern std::wstring folderRulesPath;
struct DesktopHit{bool direct=false,modeOverride=false,openDirectory=false;std::wstring modeTarget;bool managed=false;bool realFolder=false;int entryId=-1;std::wstring path;};
void loadConfig();bool saveConfig(const wchar_t* runSubkey=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");void loadFolderRules();extern std::wstring configSaveError;void ensureSettingsFocusVisible();Metrics desktopMetrics(bool* actual=nullptr);DesktopHit desktopHitAt(POINT pt);int desktopEntryAt(POINT pt);HBITMAP shellBitmap(const std::wstring& path,int size);void openEntry(int id);void showPanel(const std::wstring& path,POINT anchor,bool preview=false);void closePanel();void showSettings();
bool confirmStopBackground(HWND owner);void launchSelf(const std::wstring& args);bool applyStartup(bool enabled);void setupEntries();bool isDesktopPoint(POINT pt);void diagnose(const std::wstring& file);

bool openShellPath(HWND parent,const std::wstring& path,bool directory,bool waitForDispatch=false,HMONITOR monitor=nullptr,HANDLE* process=nullptr);
