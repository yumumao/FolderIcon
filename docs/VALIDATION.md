# 验证范围 / Validation scope

更新：2026-10-17。本页区分rc.3最终候选的本机验证、实现阶段证据与rc.2／rc.1历史结果。用户已验收本机新版并授权更新GitHub；rc.1／rc.2标签和下载附件保留。

## rc.3：桌面打开方式与多显示器跟随

- Windows10 x64、Zig0.13.0完整构建通过，新增8项窗口几何检查，保留147项模式、211项角标及原生套件；CMake目标已去重，但本机未执行CMake配置或MSVC构建。
- 两台实际显示器：2560×1440主屏与1920×1080副屏，副屏纵向偏移232像素。64项真实桌面检查覆盖两屏图标点击、程序新窗口／旧窗口唤起、真实文件夹新开／复用、原图标和目录属性恢复；同时确认测试后原后台恢复，现用EXE和INI未改。
- 同目标重叠唤起另有11项检查，确认较旧任务不会把新任务已定位的窗口拉回另一屏。最终模块另有37项集成检查，覆盖正常大小、最大化及还原、最小化及还原、双向跟随、同程序另一窗口保持位置、启动失败。旧启动路径先复现跨屏失败；最大化仅更新还原坐标无法换屏的问题复现后已修正。最终候选的63项管理GUI回归通过。
- 本机已安装同一验证候选，27项部署核验通过：原Startup快捷方式静默启动且保持单实例，设置可达，真实INI、原快捷方式字节及Run值不变；仅更新EXE并保留旧版备份。未执行注销或重启；本轮附件采用同一已验收EXE。
- 点击位置随启动任务传入独立线程，Shell派发后短期识别新建或本次唤起窗口，再摆位一次；绿色单击仍不等待双击时间，也不等待摆位完成。保留快捷方式原参数和显示方式。用户主动开始移动的窗口不再调整；同目标的新请求取消旧摆位观察，避免互相拉扯。
- 第三方启动器／代理进程、管理员窗口、特殊窗口、出现过晚或归属不明确的窗口可能正常启动而无法跟随。系统已设单击打开、后台未运行或原生双击先于身份解析的启动走Windows原有路径。没有测试全部Windows版本、DPI组合或真实软件，也不保证零闪动／启动延迟上限。
- 最终同一候选另通过41项真实快捷方式回归，覆盖1毫秒单击、重复点击抑制、第二次长按、旧计时消息、漂移、拖动、Ctrl／Shift多选、右键、后台停止后的原生行为及链接字节恢复。第一次补验因实际指针未到目标终止，独立对照确认输入失败，该次不计通过；输入恢复后的完整矩阵才计入本轮证据。

The rc.3 candidate passes 8 geometry checks, 64 actual dual-monitor desktop checks, 37 window-state integration checks, 11 overlapping-activation checks, 41 real-click regression checks, and 63 manager GUI checks on Windows 10. New and reactivated app/Explorer windows follow the clicked icon's monitor. This covers one real monitor arrangement, not all DPI settings or third-party applications. The public rc.2 download is unchanged.

## rc.3实现阶段：统一入口管理与快速单击

- Windows10 x64、Zig0.13.0完整原生测试及应用构建通过，包含147项模式检查、211项角标检查及快速点击状态测试。显式重加角标复用原件备份、持久操作记录和外部修改保护。
- 绿色白箭头向角标中心平移，保留原有圆角、白描边、颜色和32％比例。三选项卡集中管理蓝色面板快捷方式、橙色桌面文件夹和绿色单击入口。
- 最终候选的63项管理GUI检查覆盖三类添加／删除／重加角标、橙绿切换、同ID副本及主设置未保存的外观草稿。蓝色生成入口仍按entryId整组管理，同组有单独打开方式时需先恢复默认；删除真实文件夹或普通快捷方式的管理项只撤销本工具设置。
- 最终候选的41项真实快捷方式检查覆盖1毫秒按键、单击、快速双击、第二次长按、轻微漂移、Ctrl／Shift、右键、直接按住拖动、提前到达的旧计时消息、停止后台后的原生行为以及原链接字节恢复。另有26项真实空文件夹检查和36项原有打开方式对话框／设置回归。
- 绿色在松开且确认未拖动、未按Ctrl／Shift后发起打开，不等待系统双击时间。已发起打开后，快速第二次按下与松开成对忽略，因此快速连点只打开一次；拖动应直接按住图标移动，快速连点的第二次按住不启动拖动。异步身份尚未解析而原生双击已经到达时，由原生打开一次，程序取消补发。
- 启动交给独立STA线程，并等Shell完成派发后退出线程；不等待目标程序结束。原先约520毫秒的固定双击等待已移除，图标身份识别、Shell及目标程序本身仍可能耗时。界面的面板展开等待只控制面板，不控制绿色直接打开。
- 私有角标刷新工具的19项合成检查覆盖快捷方式／目录刷新、当前链接／desktop.ini／图标／模式库备份、外部编辑跳过、失败后编号不复用及恢复原件；这些私有工具不进入发行包。
- 本机已更新到上述最终候选：25项部署核验及18项安装版管理／角标刷新检查通过。原配置、Startup快捷方式及Run值不变；现有1个绿色快捷方式刷新成功、0跳过，原启动属性及原始链接备份保持，后台已恢复。
- 先前两次无预热首击尝试因自动鼠标输入未到达目标而无效，不计入通过项。验收器增加实际坐标前置校验后，用户解锁桌面，已安装的同一最终EXE通过16项无预热首击补验：1毫秒按键只启动一次，原链接字节恢复，现用EXE及INI不变，后台恢复正常。
- 所有结果均为本机受控测试，不代表完整Windows版本、DPI、多显示器、网络快捷方式或长期压力矩阵。模式仍按路径绑定，移动或重命名前先恢复默认。公开rc.2文件和下载不变。

The preceding implementation-stage candidate passed the complete native suite, including 147 mode checks, 211 badge checks and immediate-click state tests. The exact final binary passes 63 manager GUI assertions, 41 real shortcut checks, 26 real-folder checks and 36 opening-dialog/settings checks. Green entries dispatch on release without a double-click timer; a rapid second press/release pair is ignored after dispatch. Direct dragging and Ctrl/Shift selection are retained. Shell and target startup time still apply. This is local validation, not a Windows/DPI/network/stress or latency guarantee.

## 前一轮桌面打开方式验证（历史，已被本轮替代）

- Windows10 x64、Zig0.13.0，全量原生测试与应用构建通过；144项模式检查和211项角标检查覆盖模式往返、原启动参数、原图标、外部修改保护、失败取消、跨进程锁，以及替换瞬间中断后的恢复。
- 沿用圆角、白描边和32％比例绘制绿色白箭头。10种ICO尺寸的角标外像素保持不变；实际桌面标识和36项对话框／设置GUI检查已核对。
- 使用专用计数快捷方式验证单击、快速双击、第二次长按、轻微漂移、Ctrl／Shift、右键、拖动、停止后台后的系统行为以及快捷方式原件恢复。额外注入提前到达的旧计时消息，先复现取消新计时器的问题，修复后单击启动一次、第二次长按不重复。
- 真实空测试文件夹的26项检查覆盖直接单击／双击、Ctrl选择、切换面板、恢复原图标与属性及默认双击；测试后移回隔离目录，没有修改用户文件夹内容。
- 首次启动诊断中，固定1.6秒等待曾误报零启动；保持原进程、不补发输入后观察到延迟启动一次。点击验收改为最多8秒等待实际计数，再观察双击判定窗口及余量，避免因过早关闭后台而误判。该证据不等于确定了某个具体Shell调用的延迟根因，也不构成启动延迟上限保证。
- 本机安装后的28项检查通过，原配置、Startup快捷方式及Run值保持不变；另有12项原生Shell动词检查确认文件夹和快捷方式都打开正确选择窗口，取消不改变模式或图标。没有修改已公开下载包，也未执行注销／重启。
- 所有场景仅代表本机受控测试；完整Windows版本、DPI、多显示器、网络快捷方式和长期压力矩阵仍未覆盖。该历史版本需要后台监听且等待双击判定窗口；本轮已取消绿色入口的这段等待。模式按路径绑定，移动或重命名前应先恢复默认。

The preceding unreleased build passed the full native build/test suite, including 144 mode checks and 211 badge checks. Local tests additionally cover 36 GUI assertions, controlled real desktop shortcut gestures, stale timer messages, and 26 real-folder checks. The first-launch harness now waits for observed completion before checking for duplicates; a fixed early timeout had mistaken a late launch for a dropped click. This is local validation, not a Windows/DPI/network/stress or latency guarantee. Published rc.2 assets are unchanged.

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

## 历史版本未覆盖范围（当前源码增补见上节）

- Windows11实机、全部DPI/多显示器组合、长期运行压力、Explorer重启后的完整交互矩阵。
- 所有真实桌面单击/双击/拖动组合的可重复自动化验收；已有用户确认不等同于此矩阵全部通过。
- 极端跨进程配置并发写入压力；rc.2历史版本未新增完整跨进程事务锁，当前源码仅针对模式及关联入口写入增补互斥与恢复测试。
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
