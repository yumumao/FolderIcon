# 验证范围 / Validation scope

更新：2026-10-12。本页区分rc.2新增验证与rc.1历史结果；用户已授权发布rc.2源码、文档和下载包。不把旧版用户反馈泛化为新版或所有环境完整验收。

## rc.2运行状态与静默启动验证

- 将运行模式拆分为本次后台监听状态和独立的内置开机启动偏好；原启动文件夹快捷方式保持原目标与`--resident`参数，不自动删除。
- `AutoStart`优先于旧`Resident`，旧键仅在新键缺失时迁移；保存保留旧键镜像以兼容回滚旧程序。
- 使用F盘隔离目录进行配置回归、全量构建和合成GUI测试；证据保留在本机私有测试记录中，不附带真实运行配置或用户目录。
- 新测试覆盖无自启偏好时的显式静默启动、单实例、设置可达、运行状态按钮、自启保存不改变当前后台、Run项及退出偏好持久性、无后台按需面板；真实登录重启、Windows11及全桌面点击矩阵不在本轮验收范围。
- 运行状态拆分阶段的候选通过完整构建回归和39项合成GUI检查；控件ID唯一、运行状态行不重叠，并核对了合成截图。
- 该阶段原位替换EXE后18项部署核验通过：使用未修改的原Startup快捷方式正常静默启动、重复启动保持单实例、设置显式可达；真实INI、原快捷方式与原Run项均保持不变。未执行注销或重启验证。
- 上述39项GUI和18项部署检查是运行状态拆分阶段的历史结果；下列退出提示阶段重新覆盖相关行为，不把两轮计数相加。
### rc.2退出后台说明与确认验证

- 在图标和网格参数右侧加入浅蓝说明卡，明确专用快捷方式仍可双击展开、真实桌面文件夹停止单击监听，退出不删除入口或目录，也不改变开机启动。
- 设置退出按钮及托盘退出菜单使用同一确认说明，默认取消；取消保持后台运行和配置不变，确认后停止本次后台。维护命令`--stop`及内部关闭消息不弹交互确认。
- 旧版分别复现缺少说明卡和退出确认的RED；修复版通过完整构建及73项合成GUI检查，包括最小宽度和增宽布局、文字容纳、控件ID唯一、独立设置/后台内设置/真实托盘菜单三条退出路径、取消后再次确认，以及上一轮启动/配置/单实例/按需面板回归。已人工核对合成截图。
- 原位部署后22项核验通过：使用未修改的Startup快捷方式静默启动并保持单实例；安装版设置可达且新提示可见；真实INI、原快捷方式和原Run值前后保持不变。只替换EXE，备份保留，后台已恢复运行。
- 未执行注销或重启验证；多DPI、多显示器和其他Windows版本不在本轮覆盖范围。以上对应rc.2候选；公开rc.1下载包保持原样。

## rc.1已验证历史（rc.2继续保留原生测试）

- Windows10 x64、Zig0.13.0，公开`build.ps1`完整测试与中文版构建。
- 核心点击/尺寸逻辑、上下文菜单命令、图标缓存、按目录排序、展开规则、导航、配置存储与面板交互回归。
- 桌面范围48项、入口删除与角标恢复43项、入口操作42项、目录角标110项、角标风格198项。
- 失效入口维护34项、蓝色角标修复33条PASS（含当前图案像素、重复不叠加、资源图标与248字符目标路径）、目录通知6条PASS。
- 隔离GUI真实控件操作：更换图案后重加角标、重复执行、手动清理、启动核对、保留源ICO/目录及快捷方式目标参数。
- 安装版设置18项检查：两列响应增宽、最小宽度、按钮不重叠、保存/取消可达、关闭不改配置。
- 部署前后正式INI与真实快捷方式哈希不变；后台单实例、静默启动。
- 2026-10-12用户反馈候选已经可以使用，授权README与发布。

公开配图包括五张合成演示截图（两张面板图、两张设置图和一张退出确认），以及两张经用户授权、名称已脱敏的真实目录截图。后两张在rc.1阶段使用当时发布EXE的隔离副本，只读打开授权目录，采集前替换标题及条目名称；真实INI、EXE、快捷方式与图标资源前后哈希不变。没有保存未脱敏截图。补图不是新增功能验收，不据此扩展桌面点击或右键入口的验证范围；原始测试日志、真实桌面路径、个人INI和部署记录不进入Git或发行包。

## 资源样本

rc.2发布EXE：371200字节，约362.5KiB。以下内存/CPU数字来自此前rc.1的369664字节候选，不是对rc.2重新采样。单个后台空闲样本：工作集14.24MiB、私有内存3.89MiB，15秒CPU时间增量0ms。仅为一次短时测量，不是峰值、内存上限或持续零CPU保证。

## 未覆盖到的范围

- Windows11实机、全部DPI/多显示器组合、长期运行压力、Explorer重启后的完整交互矩阵。
- 所有真实桌面单击/双击/拖动组合的可重复自动化验收；已有用户确认不等同于此矩阵全部通过。
- 极端跨进程配置并发写入压力；当前并未新增完整跨进程事务锁。
- 全部网络/云盘离线、权限、第三方Shell扩展和图标损坏组合。
- MSVC/CMake构建路径的本机完整验收。
- 未提供代码签名；上游许可文件缺失问题仍有披露，见UPSTREAM.md。

构建过程中存在Zig/libunwind的工具链警告；成功退出不能表述为零警告构建。

## 云端构建与测试编码 / Cloud-test portability

发布后的首轮WindowsServer2022云端构建在桌面范围测试失败：测试直接向不存在的INI写入中文路径，导致夹具使用系统ANSI代码页。中文本机掩盖了这一问题，英文环境无法保存同样的路径。默认分支已将该夹具初始化为UTF-16，并增加编码与中文路径往返断言；没有改成英文目录来绕过覆盖。

rc.1发布后的那次后续提交只修正测试和验证说明，不修改产品源码、已发布的`v2.0.0-rc.1`标签或下载附件。发行标签仍保留原来的48项桌面范围检查，修正后的测试增加5项夹具检查。rc.2标签包含这项夹具修正；重建可使用rc.2标签或默认分支，具体构建结果见[GitHub Actions](https://github.com/yumumao/FolderIcon/actions/workflows/build-release.yml)。

The first post-release Windows Server 2022 CI run failed because the desktop-scope test created an ANSI INI containing Chinese paths. The default branch now explicitly initializes that fixture as UTF-16 and checks both its encoding and the exact path round trip. This test-only follow-up does not change application sources, the published tag, or release assets. The rc.2 tag includes the corrected fixture; the original rc.1 tag retains the original fixture.

## Reproduction

Run `scripts/build.ps1` with Windows x64 and Zig 0.13.0. The script executes the native test suite before compiling the application. The original four GUI screenshots and settings checks used isolated synthetic data. Two additional owner-authorized real-folder views were captured using the same release binary in an isolated directory, with irreversible title and item-name masks applied before capture. The README hero subsequently reuses the reviewed blue entry artwork and its masked actual panel in an HTML/CSS click-flow composition; the desktop selection, label, pointer, and shortcut arrow are illustrative. No new private-folder capture or UI redraw is involved. The retained real-folder views do not expand the functional acceptance scope. rc.2 refreshes the synthetic settings views and adds a generic exit confirmation; no new real-folder captures are taken. rc.1 release assets remain unchanged. Local deployment evidence is deliberately not published.

The user accepted the local candidate, but this preview does not claim a complete Windows-version, interaction, DPI, stress, concurrency, or licensing audit. Resource numbers above describe one short sample only.
## rc.2 reproduction and limits

The native build/test script covers the configuration migration and startup-preference changes. Local isolated GUI automation additionally checked 73 assertions, including silent startup, single-instance behavior, reachable settings, session/startup separation, on-demand panels, layout, and all three settings/tray exit paths. A separate 22-check deployment verification confirmed that the existing Startup shortcut, runtime configuration, and prior Run value were unchanged. These private local GUI/deployment scripts and evidence are not shipped as part of the public native test suite. Actual logout/reboot was not tested. Refer to the rc.2 tag or commit in Actions for its cloud-build result; a successful build is not a full Windows 11 or interaction-matrix acceptance.
