# 上游来源与许可边界

- 原仓库：https://github.com/ltrudu/FolderIcon
- 本Fork：https://github.com/yumumao/FolderIcon
- 中文开发分支：feature/chinese-dual-mode
- 起点：上游master提交363f4b4，Better script files management。
- 初始功能研究参考v1.2.0；原版C源码、工程文件和提交历史保留。

中文版本使用独立的src目录重组为C++/Win32实现，包括中文设置、两种运行模式、图标网格与背景配置。它不是原作者发布的官方版本。

截至2026-10-08，上游README声称MIT，但master及v1.2.0没有所链接的LICENSE文件，GitHub仓库元数据也未识别出许可证。这里如实保留说明，不臆造缺失许可，不替原作者确认授权范围。本轮只分享开发源码分支，不制作二进制Release；进一步再分发前应向原作者确认。

上游docs/dark.png与docs/light.png是原版截图，不是中文版验收截图。原仓库的标签发布工作流仍构建原版，本分支不触发标签发布、不向上游自动发起PR。
