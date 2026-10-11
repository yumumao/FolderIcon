#pragma once
#include "folder_sort.h"
// Modal editor; returns true only after an atomic, successful preferences save.
bool showSortSettings(HWND owner,const std::wstring& folder,const std::vector<FolderSort::Item>& items);
