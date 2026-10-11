# 隐私与发布规则 / Privacy and publishing

## 只发布明确允许的文件

- 源码、测试、构建/打包/检查脚本、无私人路径的示例配置。
- 中英文README、发布说明、验证和来源文档。
- 原创应用图标、明确审核的合成演示截图，以及保留的上游截图。
- 发行附件仅包含公开源码对应的EXE、公开文档及图片、示例配置和SHA256校验文件。

不发布：`FolderIcon.ini`、私人ICO、桌面快捷方式、文件列表、个人路径、恢复记录、缓存/排序数据、访问令牌、账号密码、AI协作记录或本地部署证据。`dist/`及工具侧目录保持忽略；EXE以单独审核的Release附件提供，不入Git源码历史。

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

Only explicitly allowlisted source, tests, tooling, example configuration, public documentation, the application icon, and reviewed synthetic screenshots are eligible for Git. The staged index is inspected, not just working-tree files.

Release packaging copies an exact public file list, never an entire local deployment folder. Runtime INI files, personal icons and shortcuts, recovery records, caches, file listings, credentials, and local collaboration/evidence files are excluded. Screenshots are visually reviewed. This is defense in depth, not a guarantee that every possible secret format can be detected.