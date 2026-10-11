#pragma once
#include "app.h"
namespace EntryMaintenance {
struct Result {size_t removed=0,restored=0,deferred=0;std::wstring warning;};
Result reconcile(bool history=true);
bool repairShortcutBadge(size_t index,size_t& updated,std::wstring& error);
// Polls only change-notification handles while idle. No directory enumeration on idle ticks.
bool tick(bool force=false);
void shutdown();
}
