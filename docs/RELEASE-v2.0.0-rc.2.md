# FolderIconCN v2.0.0-rc.2

发布：2026-10-12 · Windows x64 · 公开预览 / Public preview

## 这次改了什么

- **不再设置运行模式**：后台是否运行就是当前状态。退出后台后自然按需使用，不会再因为选择按需而打不开设置。
- **修复静默启动**：`--resident`明确启动后台，不弹设置，也不受旧`Resident=0`配置阻挡。直接运行EXE或`--settings`始终打开设置。
- **后台启停和开机自启分开**：启动/退出只影响本次运行；开机静默启动勾选框只管理程序自己的Run启动项。旧配置可迁移，保存保留旧键兼容。
- **退出前后都有提示**：参数右侧增加浅蓝说明卡；设置按钮和托盘菜单退出时使用同一确认框，默认取消。
- **说明退出后的区别**：蓝色角标专用快捷方式仍可按系统点击方式（通常双击）展开面板；真实桌面文件夹不再监听单击，恢复Windows原有打开方式。退出不删除入口或目录，也不改变开机启动。
- 同步中英文README、新版设置及确认配图、验证说明、隐私说明和发行打包白名单；包含此前默认分支对英文环境中文路径测试夹具的UTF-16修正。

## 下载与升级

- 推荐：`FolderIconCN-v2.0.0-rc.2-win-x64.zip`，包含EXE、双语说明、示例配置和公开配图。
- 已安装用户也可只下载`FolderIconCN.exe`。下载后核对`SHA256SUMS.txt`；EXE未做代码签名，不建议关闭系统安全防护。
- 先退出后台（确认退出），备份原程序目录，再替换EXE。**保留原程序位置、`FolderIcon.ini`、`icons/`、`folder-badges/`和排序/恢复记录**；不要用示例INI覆盖个人配置。
- **现有启动文件夹快捷方式无需修改**，保持EXE目标和`--resident`参数即可。手工Startup快捷方式和内置Run启动项任选一种；取消内置选项不会删除手工快捷方式。
- 管理脚本使用的`--stop`仍然直接停止后台，不显示交互确认。

rc.1历史标签和附件保留不变；本版是新标签、新下载包，不覆盖旧版。

## 验证与边界

Windows10 x64 + Zig0.13.0完整原生测试与构建；退出提示候选通过73项合成GUI检查，安装后22项部署核验通过，配置、原Startup链接和原Run值保持不变。涵盖设置/托盘退出确认与取消、静默单实例启动、设置可达、状态与自启分离、无后台按需展开。云端构建结果以本标签/提交的[GitHub Actions](https://github.com/yumumao/FolderIcon/actions/workflows/build-release.yml)为准。

未实际注销或重启验证，不宣称Windows11实机、多DPI/多显示器、长期压力和全桌面点击组合已完整通过。界面目前仅简体中文。上游许可文件缺失的披露保持不变；详见[来源与许可状态](https://github.com/yumumao/FolderIcon/blob/v2.0.0-rc.2/docs/UPSTREAM.md)及[验证范围](https://github.com/yumumao/FolderIcon/blob/v2.0.0-rc.2/docs/VALIDATION.md)。

## English summary

- Removes the stored on-demand/enhanced mode selector. Running the listener is a state; stopping it naturally returns to on-demand use.
- Fixes silent `--resident` startup, including legacy `Resident=0` configurations. Running the EXE or `--settings` always opens settings.
- Separates the built-in login-startup preference from the current background session. Manual Startup-folder shortcuts remain untouched.
- Adds an explanation card beside appearance controls and a shared, cancel-by-default exit confirmation in settings and the tray menu. `--stop` remains non-interactive.
- Dedicated blue-badged launchers still open panels without a background listener. Real desktop folders are no longer intercepted and open normally in Windows. Stopping does not delete entries or change login startup.
- Updates bilingual documentation, synthetic settings/confirmation screenshots, privacy boundaries, packaging, and the earlier UTF-16 test-fixture portability fix.

**Upgrade:** stop the listener, back up the application directory, and replace the EXE without relocating it. Keep your runtime INI, icons, badge recovery records, and sorting data. Existing Startup shortcuts with `--resident` need no change. The built-in startup checkbox does not remove manually created shortcuts.

The release includes a portable ZIP, standalone EXE, and SHA256SUMS.txt. The binary is unsigned and the UI remains Simplified Chinese. rc.1 history and downloads are not replaced.

Validation includes the native Windows 10 x64/Zig 0.13.0 test/build path, 73 synthetic GUI checks, and 22 local deployment checks. Actual logout/reboot, hands-on Windows 11, all DPI/monitor combinations, and long-term stress remain outside this round. See the linked validation scope and the tag's Actions results; this is still a preview.
