#include "../src/context_menu.h"
#include <cstdio>
#include <cstdlib>

static int count = 0;
static void check(bool value, const char* message) {
    std::printf("%s %s\n", value ? "PASS" : "FAIL", message);
    if (!value) std::exit(1);
    ++count;
}

int wmain() {
    check(std::wstring(ContextMenu::registrySubKey()) ==
              L"Software\\Classes\\Directory\\shell\\FolderIconCN",
          "current-user registry path");
    check(ContextMenu::commandLine(L"C:\\Program Files\\FolderIcon\\FolderIconCN.exe", L"create") ==
              L"\"C:\\Program Files\\FolderIcon\\FolderIconCN.exe\" --context-folder create \"%1\"",
          "create command quotes executable and folder argument");
    check(ContextMenu::commandLine(L"C:\\Program Files\\FolderIcon\\FolderIconCN.exe", L"convert") ==
              L"\"C:\\Program Files\\FolderIcon\\FolderIconCN.exe\" --context-folder convert \"%1\"",
          "convert command quotes executable and folder argument");
    check(ContextMenu::commandLine(L"C:\\Program Files\\FolderIcon\\FolderIconCN.exe") ==
              ContextMenu::commandLine(L"C:\\Program Files\\FolderIcon\\FolderIconCN.exe", L"choose"),
          "default command opens the chooser");
    const std::wstring desktop = L"C:\\Synthetic\\Test\\Desktop";
    check(ContextMenu::isDirectDesktopChild(L"C:\\Synthetic\\Test\\Desktop\\Work", desktop),
          "direct desktop child");
    check(ContextMenu::isDirectDesktopChild(L"c:/synthetic/test/desktop/work/", L"C:\\Synthetic\\Test\\Desktop\\"),
          "case and slash normalization");
    check(ContextMenu::isDirectDesktopChild(L"\\\\?\\C:\\Synthetic\\Test\\Desktop\\Work", desktop),
          "extended path prefix normalization");
    check(!ContextMenu::isDirectDesktopChild(L"C:\\Synthetic\\Test\\Desktop\\Work\\Nested", desktop),
          "nested desktop folder is excluded");
    check(!ContextMenu::isDirectDesktopChild(L"C:\\Synthetic\\Test\\Desktop2\\Work", desktop),
          "similar desktop prefix is excluded");
    check(!ContextMenu::isDirectDesktopChild(desktop, desktop),
          "desktop itself is excluded");
    check(!ContextMenu::isDirectDesktopChild(L"D:\\Synthetic\\Test\\Desktop\\Work", desktop),
          "different drive is excluded");
    std::printf("context menu: %d checks PASS\n", count);
    return 0;
}
