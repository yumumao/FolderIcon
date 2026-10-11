# 来源与许可状态 / Upstream and license status

- 上游：[ltrudu/FolderIcon](https://github.com/ltrudu/FolderIcon)。
- 本Fork：[yumumao/FolderIcon](https://github.com/yumumao/FolderIcon)。
- 起点：上游master提交`363f4b4`（Better script files management）。
- 原版C源码、工程文件和提交历史保留；当前中文实现位于`src/`，可执行文件名为`FolderIconCN.exe`。
- 本Fork不是原作者官方中文版，不将中文版功能、截图或测试结果归给上游。

## 许可核对

2026-10-12重新核对：上游README写明MIT，但其链接的master分支`LICENSE`返回404；此前检查的v1.2.0树也缺少该文件。本仓库没有补造许可正文、版权主体或替原作者确认再分发范围。

这次用户授权发布独立Fork的公开预览，不等于补全了上游许可文件。发布页与中英文README明确保留此问题。需要再分发或商业使用时，请先核实完整授权；不要把公开Fork、可下载成品或MIT字样误当成已经完成的许可审计。

## 图像与构建来源

- `docs/dark.png`、`docs/light.png`是保留的上游原版截图，不代表当前中文版界面。
- `docs/images/`包含本Fork的合成演示截图、两张经明确授权且名称已脱敏的真实目录图，以及复用已审核素材的点击示意图；截图来源和边界见PRIVACY.md。
- `assets/app.ico`为本Fork的应用图标；私人ICO不进入仓库。
- 旧`.sln`、`.vcxproj`构建原版C程序。当前GitHub工作流调用`scripts/build.ps1`测试并构建中文版，工件发布与旧标签区分。

## English

This independent fork retains the upstream C implementation and history, while the Chinese Win32 application is implemented in `src/`. It is not an official upstream release.

On October 12, 2026, upstream's README still stated MIT, but its linked `LICENSE` file on master returned HTTP 404. Earlier inspection also found no such file in v1.2.0. No license text or copyright attribution has been invented to fill that gap. Publishing this fork does not resolve it; verify full permissions before redistribution or commercial use.

Current documentation includes synthetic demo screenshots, two explicitly authorized and privacy-masked real-folder views, and a click-flow illustration reusing reviewed assets. See PRIVACY.md for capture boundaries. The inherited `dark.png` and `light.png` are upstream screenshots, not current UI validation. Personal configurations, icons, desktop listings, and local audit records are excluded from source and release artifacts.