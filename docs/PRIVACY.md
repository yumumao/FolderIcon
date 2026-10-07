# 隐私与发布规则

## 三道保护

1. `.gitignore`采用根目录默认拒绝，再开放必要源码、测试、构建脚本和公共文档。即使新增临时根文件，普通git add也不会自动纳入。
2. `.gitattributes`规范换行、标记二进制，并给本机协作目录与成品目录加export-ignore。export-ignore只影响git archive，不保护提交历史。
3. `scripts/check-publish.ps1`检查整个暂存区的路径白名单与文本风险。提交前必须通过；不能只检查git status后直接推送。

## 不得上传

- 本地FolderIcon.ini、真实文件夹目标、用户目录与机器名。
- 个人ICO、桌面快捷方式、桌面截图与诊断/测试材料。
- .codex、.claude、.shared、备份、工作流水、会话和接续记录。
- 编译器、缓存、中间产物、EXE/ZIP/调试符号。
- .env、访问令牌、密码、私钥和凭据文件。

只允许无个人目标的config/FolderIcon.example.ini。用户实际配置应保存为EXE同目录下的FolderIcon.ini，并保持未跟踪。代码默认目标从Windows Known Folder API获取，不写死开发者的目录。

## 提交流程

- 仅git add明确列出的公共文件，不使用git add .或git add -f。
- 运行check-publish.ps1，再检查git diff --cached --stat和git diff --cached --check。
- 确认新提交的作者/提交者邮箱为GitHub noreply；不更改上游历史的作者归属。
- 核查新分支的新增提交后再推送；本轮不创建标签、Release或上游PR。

`.gitignore`不影响已经被跟踪的文件。若误入敏感文件，不应认为删掉工作目录中的文件即可解决；先停止推送并检查全部待发布历史。这里的检查器是防误提交措施，不是对未知秘密格式的绝对保证。
