# 隐私与发布规则 / Privacy and publishing

## 只发布明确允许的文件

- 源码、测试、构建/打包/检查脚本、无私人路径的示例配置。
- 中英文README、发布说明、验证和来源文档。
- 原创应用图标、明确审核的合成演示截图、经用户明确授权且名称已脱敏的真实目录截图，以及保留的上游截图。
- 发行附件仅包含公开源码对应的EXE、公开文档及图片、示例配置和SHA256校验文件。

不发布：`FolderIcon.ini`、私人ICO、桌面快捷方式、文件列表、个人路径、恢复记录、缓存/排序数据、访问令牌、账号密码、AI协作记录或本地部署证据。`dist/`及工具侧目录保持忽略；EXE以单独审核的Release附件提供，不入Git源码历史。

## 真实目录截图的授权与脱敏

`docs/images/real-desktop-orange.png`和`docs/images/real-shortcut-blue.png`是两张明确授权的文档配图，不是整个桌面的截图。现有入口图案单独预览，展开面板由发布版程序的隔离副本打开真实目录后截取；不修改真实配置、桌面入口、图标资源或源文件。

采集前用完全不含原字形的模糊遮罩替换面板标题与所有条目名称，不是可依稀辨认的轻度模糊。不保存未脱敏截图，不公开个人路径、目录列表、原始ICO资源或本地证据。公开图片逐一视觉复核；PNG不携带路径或文本元数据。授权覆盖这两张已审核图片及其本次文档主图复用，不自动扩大到其它真实目录。

## 首屏操作示意

`docs/images/hero-click-expand.png`复用已公开蓝色入口图案和已脱敏真实面板，以HTML/CSS排版展示桌面入口、点击动作与展开结果。入口标签、选择框、鼠标及快捷方式箭头为示意，右侧面板保持真实截图内容；不是完整桌面实拍，也没有生成式重绘UI。没有重新采集私人目录或读取原始ICO。所有真实标题和条目名称继续遮蔽。

## 防误提交

根目录默认拒绝式`.gitignore`；`scripts/check-publish.ps1`按精确白名单扫描**暂存区实际内容**，并检查敏感模式、文件大小、换行和忽略规则。新增公开文件必须同时更新白名单。图片由人/工具视觉复核，仅放行明确文件名，不开放整个截图目录。

```powershell
.\scripts\check-publish.ps1
```

`.gitattributes`负责换行与archive导出规则，不阻止文件进入历史。不要用`git add -f`绕过忽略规则，也不要认为删除工作目录中的文件能清除已提交的秘密。

## 构建与打包

本地构建脚本不读取个人配置；`scripts/package-release.ps1`只复制精确公开文件清单，不递归打包`dist/`。发布前核对ZIP清单、EXE哈希、文档图片链接和真实图像内容。示例配置保留`.example.ini`后缀，升级时不会冒充现有运行配置。

任何凭据只通过GitHub CLI的正常认证途径使用，不写入仓库、发布脚本、命令参数、截图或日志。不公开真实桌面的操作录像。隐私检查器是防误提交措施，不是对全部未知秘密格式的绝对保证。

## English

Only explicitly allowlisted source, tests, tooling, example configuration, public documentation, the application icon, reviewed synthetic screenshots, and explicitly authorized, privacy-masked real-folder views are eligible for Git. The staged index is inspected, not just working-tree files.

Release packaging copies an exact public file list, never an entire local deployment folder. Runtime INI files, personal icons and shortcuts, recovery records, caches, file listings, credentials, and local collaboration/evidence files are excluded. Screenshots are visually reviewed. This is defense in depth, not a guarantee that every possible secret format can be detected.

The two real-folder views use irreversible live name masks before screen capture. No unmasked screenshot is saved. Entry artwork is previewed separately from the actual panel. Only the two reviewed real-view PNGs and their reuse in the reviewed click-flow hero are public; original icon resources, folder listings, personal paths, and local evidence remain excluded. The hero is an HTML/CSS composition: its pointer, desktop selection, label, and shortcut arrow are illustrative; its actual panel comes from the already-masked screenshot without generative UI edits or new private capture.
