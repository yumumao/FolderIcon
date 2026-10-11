# 验证范围 / Validation scope

更新：2026-10-12。用户已确认本机候选可用并同意公开发布；首次中文版发行仍标记为公开预览，不把用户反馈泛化为所有环境完整验收。

## 已验证

- Windows10 x64、Zig0.13.0，公开`build.ps1`完整测试与中文版构建。
- 核心点击/尺寸逻辑、上下文菜单命令、图标缓存、按目录排序、展开规则、导航、配置存储与面板交互回归。
- 桌面范围48项、入口删除与角标恢复43项、入口操作42项、目录角标110项、角标风格198项。
- 失效入口维护34项、蓝色角标修复33条PASS（含当前图案像素、重复不叠加、资源图标与248字符目标路径）、目录通知6条PASS。
- 隔离GUI真实控件操作：更换图案后重加角标、重复执行、手动清理、启动核对、保留源ICO/目录及快捷方式目标参数。
- 安装版设置18项检查：两列响应增宽、最小宽度、按钮不重叠、保存/取消可达、关闭不改配置。
- 部署前后正式INI与真实快捷方式哈希不变；后台单实例、静默启动。
- 2026-10-12用户反馈候选已经可以使用，授权README与发布。

公开截图只使用合成演示目录；原始测试日志、真实桌面路径、个人INI和部署记录不进入Git或发行包。

## 资源样本

已验收EXE：369664字节，约361KiB。单个后台空闲样本：工作集14.24MiB、私有内存3.89MiB，15秒CPU时间增量0ms。仅为一次短时测量，不是峰值、内存上限或持续零CPU保证。

## 未覆盖到的范围

- Windows11实机、全部DPI/多显示器组合、长期运行压力、Explorer重启后的完整交互矩阵。
- 所有真实桌面单击/双击/拖动组合的可重复自动化验收；已有用户确认不等同于此矩阵全部通过。
- 极端跨进程配置并发写入压力；当前并未新增完整跨进程事务锁。
- 全部网络/云盘离线、权限、第三方Shell扩展和图标损坏组合。
- MSVC/CMake构建路径的本机完整验收。
- 未提供代码签名；上游许可文件缺失问题仍有披露，见UPSTREAM.md。

构建过程中存在Zig/libunwind的工具链警告；成功退出不能表述为零警告构建。

## Reproduction

Run `scripts/build.ps1` with Windows x64 and Zig 0.13.0. The script executes the native test suite before compiling the application. GUI screenshots and settings checks used isolated synthetic data. Local deployment evidence is deliberately not published.

The user accepted the local candidate, but this preview does not claim a complete Windows-version, interaction, DPI, stress, concurrency, or licensing audit. Resource numbers above describe one short sample only.