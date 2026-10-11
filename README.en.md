<div align="center">

# FolderIcon · Desktop Folder Panels

**Keep folders where they are. Open their contents in a small desktop panel.**

Native Win32 · No resident process in on-demand mode · Optional single-click expansion

[简体中文](README.md) · [Downloads](https://github.com/yumumao/FolderIcon/releases) · [Release notes](docs/RELEASE-v2.0.0-rc.1.md) · [Validation](docs/VALIDATION.md)

<img src="docs/images/panel.png" width="460" alt="A real FolderIcon panel showing synthetic demo folders and files; expandable subfolders carry a badge">

</div>

FolderIcon opens frequently used folders as compact panels. Create a desktop launcher for a folder elsewhere, or enable panel expansion for an **existing real desktop folder without moving, replacing, or deleting it**.

This is an independent Chinese-language fork of [ltrudu/FolderIcon](https://github.com/ltrudu/FolderIcon), not an official upstream release. **v2.0.0-rc.1 is a public preview.** The application UI is currently Simplified Chinese; this document is an English usage guide, not an English UI announcement. Windows 10/11 x64 are the target platforms; hands-on testing has been on Windows 10. The illustrations include four synthetic demo screenshots and two owner-authorized, privacy-masked views of real folders. Demo settings paths are shortened for readability. Real-view titles and item names are masked; personal paths are not shown.

## Quick start

1. Download `FolderIconCN-v2.0.0-rc.1-win-x64.zip` from [Releases](https://github.com/yumumao/FolderIcon/releases) and extract it to a writable, permanent location. Do not run inside the ZIP.
2. Run `FolderIconCN.exe` to open settings. The default is **on-demand mode**, without a resident listener or context-menu registration.
3. In the left-hand **添加其它文件夹到桌面（蓝色角标）** section, click **选择文件夹并创建…**, select a folder, and confirm.
4. Activate the new blue-badged desktop shortcut using your normal Windows click setting, usually a double-click. The process exits when the panel closes.
5. For single-click expansion or interception of real desktop folders, select **增强：单击展开、双击打开** and click **保存并应用** (Save and apply).

> **Upgrading:** exit the listener, back up the whole application directory, and replace the EXE. Keep `FolderIcon.ini`, `icons/`, `folder-badges/`, and sorting data. Do not relocate the EXE without updating its shortcuts. The release includes only an example INI, never someone's runtime configuration.

## Two kinds of desktop entry

| | Blue badge: panel shortcut | Orange badge: real desktop folder |
| --- | --- | --- |
| Target | Any accessible folder | A direct child of the configured desktop |
| Change | Creates a desktop `.lnk`; leaves the source folder alone | Keeps the real directory in place; no substitute shortcut |
| Without a resident listener | Opens a panel when normally activated | Opens in Explorer as usual |
| Enhanced mode | Single-click panel; double-click Explorer | Single-click panel; double-click Explorer |
| Removal | Removes the managed entry, not its target | Stops interception and safely restores the app-owned folder badge |

Both badges use the same rounded four-cell design with different colors. They **do not remove or replace Windows' shortcut-arrow overlay globally**. Blue badges are part of a shortcut's icon image, not an installed Shell overlay extension.

### Orange badge: expand an existing real desktop folder

This view pairs the existing folder's entry artwork with its actual FolderIcon panel. **Orange identifies a real folder that stays in place.** Desktop click interception requires enhanced mode; without a resident listener, Windows opens the folder normally.

<img src="docs/images/real-desktop-orange.png" width="960" alt="Existing real desktop folder artwork with an orange four-cell badge beside its actual panel; title and all item names are privacy-masked">

### Blue badge: a desktop panel shortcut to a folder elsewhere

This view pairs a desktop panel shortcut with the actual contents of its real target folder. **Blue identifies a separate desktop launcher**, not a moved source directory. Normal Windows activation still opens its panel without a resident listener.

<img src="docs/images/real-shortcut-blue.png" width="960" alt="Desktop panel shortcut artwork with a blue four-cell badge beside its actual panel; title and all item names are privacy-masked">

> Both views are owner-authorized. The left side previews the corresponding existing entry artwork; the right side captures the released application opening a real folder, not an AI-generated UI. Before capture, titles and folder, file, and shortcut names were replaced with irreversible blur-style privacy masks. Icons, badges, controls, and panel layout remain clear. These are the examples' existing icon choices, not artwork automatically applied to every installation.

### Entry settings and desktop location

<img src="docs/images/settings-entries.png" width="980" alt="Entry management settings: blue shortcuts on the left, real desktop folders on the right, with repair and cleanup buttons">

- The **left column** can browse folders on other drives and create desktop launchers.
- The **right column** lists only first-level real folders of the configured desktop, not files, shortcuts, or nested directories.
- Windows' actual desktop location is detected automatically. **指定桌面位置…** selects an override; **恢复自动识别** restores automatic detection.
- An override changes FolderIcon's scope only. It neither moves files nor changes the Windows desktop location, and does not intercept arbitrary Explorer windows.
- Shortcut creation/removal, badge repair, and cleanup take effect **immediately after confirmation**. Canceling settings does not undo them. Appearance, mode, desktop location, and edits to the real-folder list require Save and apply.

## Expand selected subfolders inside the panel

Right-click a real subfolder in the panel and enable **在面板内展开** (Expand inside the panel). Configured subfolders get a visible badge; unconfigured folders continue to open in Explorer.

<img src="docs/images/subfolder.png" width="460" alt="Nested folder panel with a prominent back button in the upper-left corner">

Use the prominent **返回** (Back) button or **double-click blank space inside a nested panel** to go back one level. The root panel does not navigate above its root. The folder context menu can disable nested expansion or open the folder in Explorer temporarily. Files and ordinary shortcuts still use their normal applications.

## Restore the blue badge after changing an icon

1. Change a managed shortcut's icon through its normal Windows Properties dialog.
2. Open FolderIcon settings and select that generated entry in the left-hand list.
3. Click **重加蓝色角标…** (Reapply blue badge) and confirm. No extra Save click is needed.

FolderIcon overlays the badge onto the shortcut's **current custom artwork**, writes an independent icon, and updates the shortcut. It does not revert your artwork, edit your original ICO, or alter the source folder's icon. ICO files and DLL/EXE icon resources are supported. Repeating the operation does not stack badges; unreadable custom icons produce an error rather than a silent fallback.

## Remove entries and clean up orphan badges

Deleting a generated desktop shortcut, either in settings or directly on the desktop, **never deletes the source folder**.

- The enhanced listener reacts to desktop-directory change notifications and reconciles entries after changes settle. It does not scan whole disks or repeatedly enumerate an idle desktop.
- Without the listener, opening settings performs reconciliation. **清理失效配置及孤立角标** runs it manually.
- Cleanup removes custom entries with no surviving managed shortcut, and restores unreferenced legacy folder badges for which this application owns recovery records. Explicit folder-expansion rules are not automatically deleted.
- Renamed shortcuts or surviving copies in the monitored desktop scope retain their entry. Offline directories, unreadable shortcuts, and incomplete enumerations are handled conservatively.
- Later user changes to a folder icon are not forcibly overwritten. Some generated ICOs are retained as recovery assets so copied shortcuts elsewhere do not lose their icons.

## Optional Explorer context menu

Enable **在文件夹右键菜单中添加“添加到FolderIcon面板”** in settings and save. Uncheck and save to unregister it.

Right-click a folder and choose **添加到FolderIcon面板**. A choice dialog appears first:

- Create a separate desktop panel shortcut, including for a folder outside the desktop.
- Enable expansion for the selected existing real desktop folder, without creating a substitute. This option is limited to a direct child of the desktop.

Registration is per user and installs no resident Shell extension. On Windows 11, the command may appear under **Show more options**; full Windows 11 interaction has not been tested hands-on.

## Appearance, sorting, and interaction

<img src="docs/images/settings-appearance.png" width="840" alt="Appearance settings for icon size, grid, font, corners, color, expansion delay, and operation mode">

- Follow desktop icon/grid metrics and, independently, its font; or customize size, columns, and spacing.
- Configure color, opacity, and rounded corners. The current renderer uses a **single translucent layer, not live blur**, avoiding the former two-window corner problem.
- Right-click the panel and choose **排序与整理…** for per-folder sorting, direction, and custom arrangement. Drag rearrangement is supported.
- Dot-prefixed entries are hidden by default. Hidden/system items can be shown explicitly.
- Single-click an item to open it; use the wheel to page; press Esc or click outside to close.
- Settings enforce a minimum width and resize both columns and buttons proportionally. Scroll to the bottom for Save if the window is short.

### Process behavior

| Mode | Behavior |
| --- | --- |
| On-demand, default | Starts when needed; exits when the panel closes |
| Enhanced | One shared listener; silent login startup; single-click expansion and live cleanup |

Running the EXE directly **opens settings**. Login startup uses `--resident` and does not show settings. Saving on-demand mode removes this application's login startup entry and stops its listener. The tray menu also offers settings, mode switching, and exit for the current session.

The default preview delay is 80 ms; Windows' double-click recognition interval is not shortened. Actual response time depends on system load and Shell icon providers.

## Lightweight, with measured limits

Native C++/Win32, without Electron, WebView, or the .NET runtime. Administrator privileges are not required. The validated candidate EXE is approximately 361 KiB.

One Windows 10 idle-listener sample on October 12, 2026 measured **14.24 MiB working set**, **3.89 MiB private memory**, and no measurable CPU-time increase over 15 seconds. This is **one short sample**, not a memory ceiling or a perpetual zero-CPU claim. See [validation and remaining coverage](docs/VALIDATION.md).

## Command line

```powershell
.\FolderIconCN.exe                           # Settings
.\FolderIconCN.exe --settings
.\FolderIconCN.exe --folder "$env:USERPROFILE\Documents"
.\FolderIconCN.exe --resident                # Silent listener if enhanced mode is configured
.\FolderIconCN.exe --stop                    # Stop this session; keep saved startup setting
```

Generated launchers use `--entry <ID>`. The legacy `work`, `coding`, and `services` IDs remain supported. Settings manages new entries automatically; editing the INI is unnecessary. `--diagnose <output.json>` writes mode/desktop-metric diagnostics, not a file listing; keep diagnostics local unless reviewed.

## Build from source

Validated toolchain: Windows x64, PowerShell 7, and **Zig 0.13.0**. Install the compiler yourself; the local build script does not download it.

```powershell
.\scripts\build.ps1 -Zig 'zig' -BuildRoot '<build-directory>' -OutputDirectory '<output-directory>'
.\scripts\build.ps1 -Zig 'zig' -BuildRoot '<build-directory>' -TestsOnly
```

The script runs tests before building `FolderIconCN.exe`. Temporary files and Zig caches go into the build directory; personal INI/icon files are not overwritten. CMake also supplies the application and test targets, but that MSVC/CMake path has not been fully validated locally:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

`main.c`, the old `.sln`, and `.vcxproj` retain the upstream C implementation; **they do not build the current Chinese application**. The GitHub workflow runs the Chinese build/tests and uploads artifacts, rather than accidentally releasing the legacy C executable.

## Local data and uninstalling

`FolderIcon.ini`, generated icons, folder-badge recovery records, caches, and sorting data live beside the EXE. There is no built-in telemetry, account, online updater, or cloud sync.

To stop using the application: remove real-folder expansion rules, remove generated entries, and run badge cleanup; disable the context menu, switch to on-demand mode, then exit. **Do not delete `folder-badges/` before restoring folder icons.** Resolve inaccessible folders or icon conflicts before removing recovery data.

## Scope and attribution

This preview does not claim complete Windows-version, multi-monitor/DPI, Explorer-restart, or concurrent-writer coverage. The EXE is unsigned: obtain it from this repository and verify the supplied SHA256; do not disable system security protections.

Upstream's README declares MIT, but its linked `LICENSE` file is missing from the checked tree. This fork preserves attribution and does not invent a replacement license. Verify the full permissions before redistribution or commercial use. See [upstream and license status](docs/UPSTREAM.md).

[Privacy and publishing](docs/PRIVACY.md) · [Validation](docs/VALIDATION.md) · [简体中文](README.md)