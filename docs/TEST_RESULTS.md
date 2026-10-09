# Windows V1.1.1 字体与提示优化

日期：2026-10-09。环境沿用 Qt 6.11.1、MinGW GCC 13.1 x64、CMake 4.3.2、Ninja。

## 修改内容

- Theme.cpp：使用微软雅黑 UI / 微软雅黑 / Segoe UI 字体回退，正文 10.5pt，笔记 11pt；标题改为较轻字重，统一辅助文字和悬停提示样式。
- MainWindow.cpp：侧栏固定 48 逻辑像素行高，避免切换主题时文字挤在一起；精简主题说明和状态提示。
- CalendarPage.cpp：月份字重变轻，日历操作提示简化。
- NotesPage.cpp、TodoPage.cpp、TodoDialog.cpp：更自然的占位、空状态与输入提示，保留真实保存失败和删除风险说明。
- main.cpp：重复启动提示说明托盘入口，不再只笼统地说“从托盘打开”。
- CMakeLists.txt、Deploy.ps1、README.md：版本和运行包路径更新为 1.1.1。

## 已完成验证

Release 编译成功。最终版本的原生 Windows 测试 **17 项通过，0 失败，0 跳过**；offscreen CTest **1/1 通过**。
日志：test-results-v1.1.1-windows.txt、test-results-v1.1.1-offscreen.txt。
测试仍使用临时数据，不改实际用户数据库或开机启动。

在本机 200% 缩放下通过 QT_SCALE_FACTOR=0.5 模拟 100% 保存截图，检查最小 880×640 月历、笔记、Deadline 编辑器，修复了侧栏行距问题。最终截图见 screenshots/v1.1.1/。
该模拟不等于覆盖所有真实显示器和混合 DPI 组合。没有引入新字体文件或第三方依赖。

发布包启动通过：从进程 PATH 移除 Qt 开发路径后执行 --smoke-test，退出码 0。该检查在本机完成，未验证另一台干净机器。

## 人工边界

桌面合成透明观感、真实中文输入法、通知中心实际展示、另一台电脑运行仍需人工验收。提醒仍需程序运行或驻留托盘。
先从托盘退出旧版本，再打开新版本；数据结构仍是 v2，无新增迁移。
历史功能与验收步骤见 TEST_RESULTS-v1.1.md。