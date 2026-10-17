# FolderIconCN v2.0.0-rc.3

发布：2026-10-17 · Windows x64 · 公开预览 / Public preview

## 这次新增

- **逐个设置打开方式**：右键桌面文件夹或快捷方式，选择展开面板、单击直接打开或恢复默认；软件和文件快捷方式只提供直接打开。
- **三组选项卡统一管理**：设置 → 管理桌面入口，集中管理蓝色面板快捷方式、橙色原位展开和绿色单击入口，支持添加、删除选中、重加角标及模式切换。
- **绿色单击即时发起打开**：松开鼠标即交给Windows启动，不等双击时间；快速第二击不重复打开。直接按住拖动、右键和Ctrl／Shift多选保留。目标程序自身启动仍可能耗时。
- **窗口跟随图标所在屏幕**：本次新建或唤起的旧窗口移到图标所在显示器，尽量保留大小和最大化／最小化状态；不同时搬走同程序其它窗口，也不持续拉回手动移动的窗口。
- **同风格绿色角标**：箭头更靠近角标中心，沿用现有右上位置、圆角、白描边和比例。
- **保留原件与恢复能力**：保留快捷方式参数、工作目录和显示方式；恢复默认时还原原图标，外部修改冲突保留恢复材料。修复旧计时消息、同ID入口副本和构建目标重复登记等问题。

## 升级与使用

退出后台，备份整个程序目录，再替换同路径EXE。保留`FolderIcon.ini`、`desktop-modes.ini`、`mode-backups/`、`folder-badges/`、`icons/`及排序记录。现有Startup快捷方式无需改动；打开设置后按需启动后台。单击接管与显示器跟随需要后台运行。普通文件先建快捷方式；此电脑、回收站等虚拟图标不在范围。

模式按路径保存，移动或重命名前先恢复默认，再配置新位置。删除管理项不会删除真实文件夹或普通快捷方式；蓝色自建入口只删除本工具生成的链接。移除程序前先恢复各项目。

## 验证与范围

Windows10 x64／Zig0.13.0完整原生测试和构建通过，含147项模式、211项角标和8项几何检查。最终同一EXE通过63项管理GUI、41项真实快捷方式、64项双屏桌面和27项本机部署检查；窗口模块另通过37项状态集成与11项重叠唤起检查，用户已确认可以使用。详见[验证范围](VALIDATION.md)。

特殊启动器、代理进程、管理员窗口、出现过晚或归属不明确的窗口可能保持程序原位置；尚未覆盖全部第三方软件、Windows11、DPI组合、注销／重启与长期压力。不承诺零闪动或固定启动延迟。

附件为便携ZIP、独立`FolderIconCN.exe`及`SHA256SUMS.txt`；界面为简体中文。EXE为441856字节，SHA256：`d47c7f5d8de83b9910d6ddbe7ea33b30440cc143131cfbf3950acf206dbf6efa`。未签名，旧rc.1／rc.2标签和附件保留。

## English

- Choose Panel, Single-click open or Restore default for each desktop folder or shortcut from its context menu. Application/file shortcuts open directly rather than expanding.
- Manage blue panel launchers, orange real folders and green direct-open entries in three tabs, with add/remove, badge repair and mode switching.
- Green entries launch on release without the system double-click wait. Duplicate clicks are suppressed; normal dragging, right-click and Ctrl/Shift selection remain available. Shell and target startup still take time.
- New or reactivated windows follow the clicked icon's monitor. Size and maximized/minimized state are retained where possible. Placement happens once, without moving every window of the same application.
- Centers the green badge arrow and retains shortcut arguments, working directory, show state and original-icon recovery. Protects external changes and fixes stale-timer, duplicate-entry and build-registration issues.

**Upgrade:** stop the listener, back up the application directory, and replace the EXE in place. Keep runtime configuration, mode/icon recovery files and sorting data. Existing Startup shortcuts remain valid. Restore modes before renaming/moving entries or uninstalling.

The accepted Windows10 binary passes the native suite, 63 manager GUI checks, 41 real shortcut checks, 64 dual-monitor desktop checks and 27 deployment checks. The window module also passes 37 state checks and 11 overlapping-activation checks. Broker/launcher processes, elevated or ambiguous windows, all Windows/DPI combinations and long-term stress are not fully covered. See [validation](VALIDATION.md) and [opening modes](DESKTOP-MODES.md).

Portable ZIP, standalone EXE and SHA256 checksums are provided. The UI is Simplified Chinese and the binary is unsigned. Existing rc.1 and rc.2 tags and assets are retained.
