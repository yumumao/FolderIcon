# 桌面打开方式 / Desktop opening modes

2026-10-17，适用于v2.0.0-rc.3。旧版rc.2不包含本页新增功能。

在设置中启用右键菜单后，右键桌面图标，选择FolderIcon打开方式…。对话框会选中当前模式；选择后点保存，取消不修改配置或图标。

| 模式 | 适用项目 | 右上角标 | 后台运行时 |
| --- | --- | --- | --- |
| 展开面板 | 桌面真实文件夹、文件夹快捷方式 | 橙色／蓝色四格 | 单击展开内容 |
| 单击直接打开 | 桌面真实文件夹、`.lnk`快捷方式 | 绿色白箭头 | 单击打开目录或按快捷方式原参数启动 |
| 恢复默认 | 已设置的项目 | 恢复原图标 | 恢复该项目原有行为 |

绿色角标沿用四格角标的右上位置、圆角和白色描边，不替换原图标主体，不改Windows的快捷方式小箭头。软件或文件快捷方式不能展开面板；普通文件本体、此电脑、回收站不在支持范围。桌面位置采用设置中自动识别或指定的第一层目录。

绿色单击在松开时立即发起打开，不再等待Windows双击判定。快速连点只执行一次，第二次按下和松开被忽略；要拖动时直接按住再移动，Ctrl／Shift多选和右键不触发单击打开。目标程序自身启动耗时仍由Windows及目标程序决定。系统已经设为单击打开时，不额外启动第二次。退出后台只结束本次单击监听，不改变启动项，图标仍保留以便下次使用。

`desktop-modes.ini`记录每个项目的模式，`mode-backups/`保存快捷方式原件与恢复快照，`folder-badges/`保存真实目录图标恢复材料。升级应保留这些文件。移除功能或删除程序目录前，先逐项恢复默认。快捷方式在设置后被其它程序修改时，程序会拒绝直接覆盖并保留恢复材料；极短的并发替换也保全被替换原件。遇到冲突时按提示检查入口，不要手工删除恢复文件。

模式按路径记录。移动或重命名前先恢复默认，再在新位置重新设置；不会扫描整个磁盘追踪。管理窗口的重加角标保留原件备份；生成入口仍存在单独模式时，先从对应条目恢复默认再删除生成入口。

## 多显示器跟随

后台处理绿色入口的单击时，以点击位置确定显示器。本次新建的窗口和重新唤起的已有窗口都跟随该屏幕；真实文件夹按实际目录匹配，快捷方式保留原参数、工作目录和显示方式。

正常窗口尽量保留尺寸与相对位置，过大的窗口收进目标屏幕可用区域；最大化保持最大化，最小化窗口的还原位置也更新。每次打开只移动一次；用户随后移动窗口时不会被拉回，也不把同程序的所有窗口一起搬走。

这是短期识别本次启动窗口后的位置调整，并非强制约束所有软件。使用启动器／代理进程、管理员权限、特殊窗口、窗口出现过晚或无法可靠识别归属时，可能只正常打开而不移动。系统本身已开启单击、后台未运行，或未完成身份识别前原生双击已执行的打开沿用Windows行为。本轮只验收Windows10的实际双屏组合，未覆盖全部DPI和第三方软件。

## 统一入口管理

设置 → 管理桌面入口，三个选项卡集中管理，操作即时生效：

- 蓝色：选择任意文件夹创建桌面面板快捷方式，或添加现有桌面文件夹快捷方式；可删除自建入口、重加角标。前三个内置入口保留。
- 橙色：添加桌面第一层真实文件夹；删除选中撤销原位展开并恢复原图标；可重加角标和更改打开方式。
- 绿色：添加桌面文件夹或快捷方式；删除选中恢复原图标及默认点击方式；可重加居中的绿色箭头和更改打开方式。

原有文件夹和普通快捷方式不会因删除管理项而被删除；蓝色自建入口删除时只移除程序生成的链接。主设置的外观草稿继续保留；修改桌面位置后须先保存再打开管理器。

## English

These features are included in v2.0.0-rc.3; older rc.2 downloads do not contain them. Enable the context menu in settings, right-click a first-level desktop folder or `.lnk`, and choose Panel, Single-click open, or Restore default. The dialog shows the current mode; Cancel leaves it unchanged.

Panel mode is available only for folders and folder shortcuts. Direct mode preserves shortcut arguments, working directory and show state, and adds a green arrow using the existing badge geometry. It needs the resident listener and launches on release without waiting for the system double-click interval. Rapid repeated clicks launch once; the duplicate press/release is ignored. Use a normal press-and-drag to move an item. Right-click and Ctrl/Shift selection retain native behavior. Ordinary files and virtual desktop icons are excluded.

Keep `desktop-modes.ini`, `mode-backups/` and `folder-badges/` during upgrades. Restore each entry before uninstalling. External shortcut edits are preserved and conflicts stop the operation with recovery files retained. Modes are path-based: restore the default before moving or renaming an entry, then configure its new location.

The direct-open worker also moves the newly opened or reactivated window to the clicked icon's monitor, once per launch. It retains size and maximized/minimized state where possible and fits oversized windows into the work area. Other same-app windows are not moved as a group. Elevated apps, launcher/broker processes, late or ambiguous windows, and native Windows activation outside the direct-open worker may retain their original placement.
