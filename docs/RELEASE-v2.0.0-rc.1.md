# FolderIconCN v2.0.0-rc.1 · 中文版公开预览 / Public preview

## 中文

这是独立中文增强Fork的首个公开预览版本，不是上游v1.x的原版C程序；当前界面为简体中文。

### 主要功能

- 原生Win32文件夹面板：按需模式零常驻，增强模式一个静默登录启动的共用监听器。
- 任意目录生成蓝色角标桌面快捷方式；已有桌面真实文件夹原地展开并加橙色角标。
- 面板内右键选择哪些子文件夹继续展开，带角标、返回按钮和空白处双击返回。
- 可选右键菜单先选择操作，不误创建替身；桌面位置自动识别或手动指定。
- 换过图案的快捷方式可重加蓝色角标，保留私人ICO与启动参数。
- 自动核对已删除入口、恢复有记录的历史孤立角标；提供手动清理，离线和冲突保守处理。
- 修复同名文件夹/快捷方式打开错误、图标倒置、设置入口恢复、圆角、删除持久化及响应式布局问题；提供图标缓存和按目录排序。
- 新增中英文图文README，配图仅为合成演示目录。

### 下载与升级

下载`FolderIconCN-v2.0.0-rc.1-win-x64.zip`并解压。提供独立EXE与`SHA256SUMS.txt`供核对；初次使用直接运行EXE打开设置。

升级前退出监听并备份整个程序目录；只替换EXE，保留已有INI、图标、排序和恢复记录。安装包不含运行时个人配置，不会自动迁移你的目录。

### 验证与限制

Windows10 x64 + Zig0.13.0的构建、核心回归、隔离GUI及安装版设置检查通过；用户已确认候选可用。Windows11、所有DPI/多显示器、Explorer重启和极端并发尚未完整验收，因此标为预览。程序未签名。上游README声明MIT但缺少链接的LICENSE文件，许可状态见`docs/UPSTREAM.md`，不将其表述为已完成许可审计。

## English

First public preview of this independent Chinese-language Win32 fork, not the upstream v1.x C binary. The UI is Simplified Chinese; an illustrated English README is included.

Highlights: on-demand or single-listener operation; blue-badged launchers for arbitrary folders; orange-badged in-place expansion for real desktop folders; opt-in nested panels with back navigation; a two-step Explorer context action; configurable desktop location; custom-icon-preserving blue badge repair; conservative cleanup of deleted launchers and recorded orphan badges; caching, per-folder sorting, responsive settings, and correctness fixes.

Download and extract `FolderIconCN-v2.0.0-rc.1-win-x64.zip`. A standalone EXE and `SHA256SUMS.txt` are also provided. For upgrades, exit the listener, back up the application directory, replace the EXE, and keep all runtime configuration, icons, sorting, and recovery records.

Validated on Windows 10 x64 with Zig 0.13.0. Windows 11 and complete multi-monitor/DPI, Explorer-restart, and concurrent-writer coverage remain open. The executable is unsigned. Upstream's declared MIT license has a missing linked license file; see `docs/UPSTREAM.md`. This is not a claim of completed license clearance.