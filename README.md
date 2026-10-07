# FolderIcon中文双模式开发版

轻量的Windows文件夹弹出面板：通过桌面快捷方式展开文件夹，以大图标显示文件、子文件夹和快捷方式。

本分支基于[ltrudu/FolderIcon](https://github.com/ltrudu/FolderIcon)，保留上游C实现，并新增原生C++/Win32中文版本。**当前为开发预览，不是稳定发行版。**

- 开发分支：`feature/chinese-dual-mode`
- 运行平台：Windows10/11 x64；目前仅在Windows10上做过下述有限验证。
- 不使用Electron、WebView或.NET运行时；需要Windows自带的Shell、GDI+和桌面合成服务。
- 本次只发布源码分支，不上传个人配置、图标、桌面截图或本机编译的成品包。

## 两种运行方式

| 模式 | 桌面入口行为 | 面板关闭后 |
| --- | --- | --- |
| 按需模式，默认 | 由资源管理器按系统原有点击设置激活快捷方式，再展开面板；系统为双击打开时就双击 | 本程序退出，不保留监听器 |
| 增强模式，开发中 | 指定的托管桌面快捷方式单击展开、双击打开目标文件夹 | 保留一个共用监听进程 |

增强模式为了区分单击与双击，会等待系统双击判定时间，并非零延迟。它通过鼠标钩子识别托管入口，不修改资源管理器全局的单/双击设置。增强模式的真实桌面交互、重命名与去重等仍需端到端验收，详情见[验证记录](docs/VALIDATION.md)。

在中文设置窗口中切换模式：

- 保存增强模式时，添加当前用户的`FolderIconCN`登录启动项并启动监听器。
- 保存按需模式时，移除本工具的该启动项并通知监听器退出。
- 托盘菜单也提供切回按需模式的入口。
- `--stop`只停止当前监听器，**不删除登录启动项**；永久停用请通过设置切回按需模式。

## 外观设置

直接运行`FolderIconCN.exe`，或点击面板右上角的设置。

- 跟随当前桌面的图标大小、横向和纵向网格。
- 取消跟随后，自定义图标大小、网格间距与每行列数。
- 设置背景颜色、不透明度、圆角和磨砂开关。
- 面板内单击文件或快捷方式即可打开；单击子文件夹在资源管理器打开。
- 滚轮翻页；Esc或点击外部关闭。

网格值指每个图标单元的宽高，不是图标之间的纯空白。默认尝试读取Explorer当前视图；读取失败则使用96像素图标和128×144像素网格。磨砂使用动态加载的Windows合成接口，属于实验效果，不保证不同Windows版本的视觉一致性；可关闭磨砂使用半透明背景。图标和文字不随背景透明度一起变淡。

## 快速开始

1. 按下文构建，得到`dist/FolderIconCN.exe`。
2. 直接运行它打开中文设置；默认是按需模式。
3. 想立即展开任意文件夹，可执行：

   ```powershell
   .\dist\FolderIconCN.exe --folder "$env:USERPROFILE\Documents"
   ```

4. 若使用固定入口，把`config/FolderIcon.example.ini`复制到EXE旁边，改名为`FolderIcon.ini`，在本地填写目标路径。已有配置不要覆盖。
5. 创建指向EXE的桌面快捷方式，并追加参数，例如：

   ```text
   "<程序目录>\FolderIconCN.exe" --entry work
   ```

6. 在快捷方式属性中选择自己的ICO。改名不会改变`--entry`标识；但重命名识别仍待实际桌面验证。

目前提供三个固定入口ID：`work`、`coding`、`services`。它们的目标分别读取本地INI的对应节；目标为空或没有配置时，默认使用系统实际桌面。路径支持`%USERPROFILE%`等环境变量。这不是任意数量入口的管理器，目前也不附带自动改写桌面快捷方式的安装器。

本地配置示意：

```ini
[work]
Target=%USERPROFILE%\Documents
[coding]
Target=%USERPROFILE%\Projects
[services]
Target=%USERPROFILE%\Shortcuts
```

这些目录只是示例，请换成自己存在的文件夹。中文INI建议保存为UTF-16LE，因为当前读写使用Windows Profile API。

## 命令行

| 参数 | 用途 |
| --- | --- |
| 无参数或`--settings` | 打开中文设置 |
| `--folder "路径"` | 展开指定目录 |
| `--entry work` | 使用work入口；另支持coding、services |
| `--resident` | 配置为增强模式时启动共用监听器 |
| `--stop` | 停止本次监听，不改变已保存模式或自启 |
| `--diagnose "输出.json"` | 输出桌面尺寸与模式诊断，不列出文件内容 |

## 从源码构建

### 方式一：Zig0.13.0，已验证

需要Windows x64、PowerShell和Zig0.13.0。编译器需自行安装，不会由脚本下载。

```powershell
.\scripts\build.ps1 -Zig 'zig' -BuildRoot '<构建缓存目录>'
# 只运行核心测试
.\scripts\build.ps1 -Zig 'zig' -BuildRoot '<构建缓存目录>' -TestsOnly
```

`-Zig`也可以传入`zig.exe`的绝对路径。脚本先运行核心测试，再编译程序；构建目录承担中间产物、TEMP/TMP和Zig缓存，成品默认输出到仓库的`dist/`。可以用`-OutputDirectory`修改成品位置。脚本不覆盖本地INI或图标。

### 方式二：CMake与Windows C++工具链，尚未实机验证

需要CMake3.20+、Visual Studio C++桌面开发工具与Windows SDK。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target FolderIconCN foldericon_core_tests
ctest --test-dir build -C Release --output-on-failure
```

`FolderIconC`保留上游原版；`FolderIconCN`是本分支中文版本。上游的`.sln`、`.vcxproj`、批处理与标签发布工作流仍针对原版，不要将其产物误认为中文版。

## 验证状态与资源占用

2026-10-08的一次合成目录验收中：

- 12条核心断言通过，中文EXE构建成功。
- 按需面板和设置窗口能打开，关闭后本程序剩余进程数为0。
- 面板打开时工作集约32.7MiB；2秒空闲采样中进程累计CPU时间增量为0ms。

以上是一次环境相关的采样，不是资源上限或持续零CPU承诺。增强监听、背景效果和模式切换的完整验收尚未完成。请勿据此认定所有功能均已稳定。后续精确状态见[验证记录](docs/VALIDATION.md)。

## 源码布局

```text
src/          中文版Win32源码
  core.h      尺寸约束、颜色检查、点击状态机
  shell.cpp   桌面视图、图标、入口配置、自启
  panel.cpp   弹出面板绘制与交互
  settings.cpp 中文设置窗口
  main.cpp    进程启动、共用监听器与消息循环
tests/        核心断言测试
scripts/      通用构建脚本、提交内容检查器
config/       无个人路径的示例配置
docs/         来源、验证边界、隐私发布规则
main.c        保留的上游C版实现
```

`dist/`、本机配置、个人ICO和开发协作记录只存在于本地，不应提交。

## 隐私与发布

`.gitattributes`统一换行和`git archive`导出规则，**不能阻止文件进入Git历史**。本分支另使用根目录默认拒绝式`.gitignore`、显式文件白名单和暂存区扫描。提交前执行：

```powershell
.\scripts\check-publish.ps1
```

检查器验证**实际暂存区**而不是只看工作目录；新增文件需要同步维护白名单。不应使用`git add -f`绕过规则。详情见[隐私与发布规则](docs/PRIVACY.md)。

## 上游与许可证说明

原项目：[ltrudu/FolderIcon](https://github.com/ltrudu/FolderIcon)。本分支保留其历史与归属。

上游README写明MIT，但检查到的上游master与v1.2.0树中没有其链接的`LICENSE`文件。本仓库没有代替上游补造许可证或版权声明；GitHub上公开Fork及本次源码分享不等于已核实所有再分发授权。进一步分发二进制或商业使用前，请向上游确认完整许可。来源详情见[UPSTREAM.md](docs/UPSTREAM.md)。

---

English summary: Experimental Chinese-language Win32 fork with configurable large icons and optional resident desktop click handling. On-demand mode exits when the panel closes. Native C++ sources live in `src/`; the original C implementation is retained. Local settings and personal assets are excluded. Enhanced-mode end-to-end validation and upstream license clarification remain open.
