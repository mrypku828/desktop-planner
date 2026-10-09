# 桌面计划 Desktop Planner — Windows V1.1.1
个人任务与随手记管理软件。C++17 / Qt 6 Widgets / Qt SQL SQLite / CMake。
源码可读、离线使用、无额外第三方业务依赖。当前仅 Windows 版本，不含移动端和云同步。

## 直接运行

发布目录为 `dist/DesktopPlanner-v1.1.1`，双击其中的 `DesktopPlanner.exe`。
必须保留同目录的 DLL、`platforms`、`sqldrivers` 和 `translations`，不能只复制 exe。
在没有 Qt 开发工具的其他 Windows 电脑上，请复制整个目录；该场景仍需人工验证。

- 计划日历：显示每一天的计划与截止事项，点击日期可新建、编辑、完成或删除当天任务；支持切换月份与回到今天。
- 普通任务可设置独立的计划日期，不设置截止时间也能出现在日历。未安排且无截止的任务在所有事项中查看。
- 点击“＋ Deadline”创建独立截止事项，必须设置截止时间。日历以粉色“◆”显示截止日；普通任务的截止日也使用该标识，过期进一步加深颜色。
- 计划日与截止日不同的任务，会分别出现在这两天；同一天只显示一次。日期格中长标题省略，溢出显示“+数量”，悬停或点击查看全部。
- 所有任务：按截止时间升序，无截止时间排在末尾；支持未完成、已完成、全部筛选。
- 新建任务：填写标题和描述，截止时间可选，提醒时间独立设置。保存即写入数据库。
- 勾选完成；再次切换可恢复未完成。双击任务或选中后点击“编辑”；删除需确认。
- 过期且未完成的任务以红色和“已过期”标识，最长约 30 秒刷新状态。
- 随手记：新建后直接输入正文；停止输入 600 毫秒后自动保存，切换笔记、离开页面和退出时补保存。
- 笔记按最近修改时间排序，搜索同时匹配标题和正文，不区分英文大小写。
- `Ctrl+N` 快速新建笔记，`Ctrl+S` 保存笔记，`Ctrl+Q` 保存并退出。
- 设置可选择雾蓝、鼠尾草绿、薰衣草、玫瑰粉、杏仁五种柔和主题，即时生效并记住。窗口默认 94% 不透明度，可调到 80–100%，使用真实窗口透明度，没有桌面模糊效果。
- 设置也可显示小组件、选择置顶、启用提醒，以及设置当前用户登录 Windows 时启动。
- 小组件显示今日安排或截止的未完成事项；快速添加默认安排在今天，不强制设置截止时间。拖动标题栏移动，拖动边缘缩放。
- 关闭小组件会隐藏它，并保存关闭状态；可从设置或托盘重新显示。
- 关闭主窗口会驻留系统托盘，提醒继续检查。真正退出用“文件 → 退出程序”或托盘菜单。
- 第二次启动会提示已有实例，请从托盘打开；不支持多实例同时操作同一个数据库。

## 提醒的实际行为

使用 Qt 的 `QSystemTrayIcon::showMessage()` 向 Windows 提交通知，无独立后台服务或计划任务。

1. 程序运行或驻留托盘时每秒检查一次，提醒时间可早于或晚于截止时间，也可用于无截止任务。
2. 重启、休眠恢复后补检未触发的过期提醒。多条到期提醒逐条显示，间隔约 12 秒。
3. 通知前先保存 `reminded_at`，保存失败不发通知。已领取的提醒重启后不会重复发送。
4. 修改标题、描述或截止时间不会重置已触发状态。更改提醒时间、关闭再启用任务提醒，会重新安排一次提醒。
5. 完成任务时取消提醒；恢复未完成不会自动重启提醒，需编辑后重新启用。删除任务同时删除提醒状态。
6. 关闭全局提醒只是暂停检查，未触发记录保留；重新启用后补检。
7. 完全退出、关机、程序尚未运行时不会通知。开机启动用于登录后恢复程序，不代表关闭期间也能提醒。
8. Windows 勿扰模式、通知权限、托盘可用性会影响实际显示。`showMessage()` 不提供“用户已看到”的回执。
9. 当前语义是“最多提交一次”，并非严格保证送达：持久化成功后、提交通知前若进程崩溃，该条可能漏提醒；Windows 阻止显示也不会自动重发。其他尚未领取的提醒保留。

如需退出后仍提醒，应后续设计独立后台进程/系统计划任务，处理重复投递、权限与安装卸载；V1.1 没有实现。

## Windows 开发环境与 VS Code

本机已安装并实际构建验证：

| 工具 | 路径/版本 |
| --- | --- |
| Qt | `D:\qt\6.11.1\mingw_64` / Qt 6.11.1 |
| C++ 编译器 | `D:\qt\Tools\mingw1310_64\bin\g++.exe` / GCC 13.1.0 x64 |
| Ninja | `D:\qt\Tools\Ninja\ninja.exe` |
| 构建成功的 CMake | 当前 PATH 中的 CMake 4.3.2 |
| SQLite | Qt 提供的 `QSQLITE` 驱动 |
| 系统 | Windows 11 x64 |

系统 PATH 默认的另一个 g++ 是 i686/32 位，请勿与这套 x64 Qt 混用。CMake 本身的位数不决定应用位数；应用使用明确指定的 x64 编译器。
本机 Qt 自带 CMake 3.30.5 在原中文路径配置时出现无诊断退出，使用 CMake 4.3.2 配置成功。
此外 `moc` 在中文构建输出路径创建文件失败，所以**构建输出请放在纯英文路径**；原中文源码目录无需移动。

新电脑：安装 Qt Online Installer 提供的 Qt 6 Desktop MinGW 64-bit 套件及对应编译器、CMake、Ninja。无需同时安装 Visual Studio 或单独 SQLite 服务器。
VS Code 安装 C/C++ 与 CMake Tools 扩展。本项目 `.vscode` 已给出推荐扩展和本机工具路径；换电脑时修改这些路径。

1. 用 VS Code 打开项目根目录。
2. 在 CMake Tools 选择与 Qt 匹配的 x64 MinGW 工具链；构建输出默认 `%LOCALAPPDATA%/DesktopPlannerBuild`。
3. 执行 `CMake: Configure`，选择 Debug 或 Release，再执行 `CMake: Build`。
4. 运行前需让该终端 PATH 包含配套编译器 bin 与 Qt bin；或直接运行已经打包的发布目录。
5. 若默认 CMake 是本机旧的 3.30.5，在 VS Code 设置 `cmake.cmakePath` 指向构建成功的 CMake。
6. 调试时选择 `DesktopPlanner` 为 CMake 启动目标，使用 `.vscode/launch.json` 的 MinGW x64 配置按 F5；该配置已做 JSON 检查，VS Code 调试交互尚未实际验证。

也可以在 VS Code PowerShell 终端用脚本：

```powershell
# 默认使用本机已发现的 Qt 和工具链，构建到英文用户目录。
.\scripts\Build.ps1 -RunTests
# 要更换目录：使用英文路径，勿复用其他工具链的构建缓存。
.\scripts\Build.ps1 -BuildDirectory 'D:\DesktopPlannerBuild' -Configuration Debug -RunTests
# 其他电脑按实际安装路径传入 -QtRoot、-CompilerBin、-Ninja、-CMake。
```

脚本只设置当前进程 PATH，不修改系统环境变量，不需要管理员权限。
若 PowerShell 执行策略禁止脚本，可逐行执行等价命令；不要为了本项目永久放宽全局执行策略。

本次实际的构建目录位于 Codex 提供的英文可写工作目录，详见 `docs/TEST_RESULTS.md`。执行构建脚本时可显式指定该目录。
Qt Creator 仍可打开 `CMakeLists.txt`，但本项目面向 VS Code 提供配置。

### 命令行构建与测试

```powershell
$env:PATH = 'D:\qt\Tools\mingw1310_64\bin;D:\qt\6.11.1\mingw_64\bin;D:\qt\Tools\Ninja;' + $env:PATH
cmake -S . -B "$env:LOCALAPPDATA/DesktopPlannerBuild" -G Ninja '-DCMAKE_BUILD_TYPE=Release' '-DCMAKE_PREFIX_PATH=D:/qt/6.11.1/mingw_64' '-DCMAKE_CXX_COMPILER=D:/qt/Tools/mingw1310_64/bin/g++.exe' '-DCMAKE_MAKE_PROGRAM=D:/qt/Tools/Ninja/ninja.exe'
cmake --build "$env:LOCALAPPDATA/DesktopPlannerBuild"
ctest --test-dir "$env:LOCALAPPDATA/DesktopPlannerBuild" --output-on-failure
& "$env:LOCALAPPDATA/DesktopPlannerBuild/DesktopPlanner.exe"
```

`BUILD_TESTING` 默认开启，需要 Qt Test。用 `-DBUILD_TESTING=OFF` 可仅构建应用。
CTest 使用 offscreen 平台，详细报告写到构建目录的 `test-results.txt`；原生 Windows 测试使用：

```powershell
& "$env:LOCALAPPDATA/DesktopPlannerBuild/planner_tests.exe" -platform windows -o windows-test-results.txt,txt
```

测试只创建临时数据库与设置，**不写开机启动注册表**。受限沙箱可能禁止 Qt SQL 写入重定向临时目录；本次完整测试是在正常 Windows 临时目录运行。
测试窗口会自动关闭，真实系统通知仍需人工验收。

### 发布包

```powershell
.\scripts\Deploy.ps1
# 指定先前 Build.ps1 使用的目录：
.\scripts\Deploy.ps1 -BuildDirectory 'D:\DesktopPlannerBuild' -Destination '.\dist\DesktopPlanner-v1.1.1-new'
```

使用同一套件的 `windeployqt` 复制 Qt、平台插件、SQL 驱动、MinGW 运行库及中文翻译。脚本拒绝覆盖已有发布目录，请选择新目录。
发布包中的 `DesktopPlanner.exe --smoke-test` 会使用临时数据启动主窗口，约 800 毫秒后保存并退出，便于验证依赖是否齐全。正常使用不要添加此参数。
`--background` 用于开机启动时驻留托盘；托盘不可用时仍显示主窗口。
V1.1 没有安装器、代码签名或自动更新。

## 数据与设置

默认目录由 `QStandardPaths::AppLocalDataLocation` 决定，本机通常为：

```text
%LOCALAPPDATA%\DesktopPlanner\DesktopPlanner\
    planner.sqlite   任务与笔记
    settings.ini     提醒开关、小组件开关/置顶、窗口位置与大小
    planner.lock     单实例运行锁
```

可在“设置”中查看实际路径并打开目录；不存于源码或编译目录。
数据库初始化在事务中完成，以 `PRAGMA user_version=2` 标记模式版本；v1 自动在事务中添加计划日期与 Deadline 类型列，保留旧任务、笔记与提醒。更高版本会拒绝打开，不覆盖旧数据。
首次运行新版前，退出旧程序并备份数据目录。升级后的数据库不能再由 V1.0 打开，请持续使用新版。
所有用户内容都通过绑定参数传给 SQL。ID 使用 UUID，时间以 UTC ISO 文本保存，显示时转换为本地时间。
截止和完成时间可为空，提醒状态附属于任务。业务层在 SQL 成功后才更新内存和刷新界面。
启动建库或读取失败会提示并停止；任务写入失败会显示错误；笔记保存失败保留编辑器内容，阻止新建、切换和正常退出，直到成功保存。

**备份**：从“退出程序”完全退出后，复制整个数据目录到安全位置。暂未实现软件内备份导出。
强制结束进程/断电可能丢失最近 600 毫秒尚未自动保存的笔记。

开机启动仅当你在设置中主动勾选时，写入 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 的 `DesktopPlanner` 项；取消勾选删除该项，不需管理员权限。
本次没有执行该设置。将发布目录放到固定位置后再启用；移动 exe 后需关闭再开启。Windows 自己的启动项禁用策略可能覆盖此设置。

## 目录与架构

```text
src/
  main.cpp                     应用入口、单实例锁、用户目录、启动检查
  domain/Models.h              Todo 与 Note 数据结构
  storage/Database.h/.cpp      SQLite 连接、建表、参数化增删改查
  services/
    PlannerService.h/.cpp      校验、排序、筛选、数据缓存、变化信号、提醒状态
    ReminderService.h/.cpp     定时补检、全局开关、Windows 托盘通知
  platform/AppSettings.h/.cpp  INI 设置及 Windows 当前用户开机启动
  ui/
    AppStyle.h                 统一字体和轻量样式
    Theme.h/.cpp               集中主题配色、应用全局柔和样式
    CalendarPage.h/.cpp        月历色卡、日期点击、当天事项入口
    MainWindow.h/.cpp          导航、托盘、设置、关闭/退出行为
    TodoPage.h/.cpp            今日与全部任务视图，编辑/删除入口
    TodoDialog.h/.cpp          标题、描述、截止与提醒编辑
    NotesPage.h/.cpp           搜索、纯文本编辑、自动保存与失败保护
    DesktopWidget.h/.cpp       共享任务视图、快速添加、随手记入口
scripts/                       构建与发布 PowerShell 脚本
resources/                     内置勾选图标与 Qt 资源文件
.vscode/                       本机 VS Code CMake 配置与扩展推荐
.clang-format                  可读的 C++ 格式约定
CMakeLists.txt                 核心库、界面库、应用、测试目标
tests/PlannerTests.cpp          Qt Test 数据/业务/界面集成测试
docs/TEST_RESULTS.md            真实测试结果与人工验收表
docs/LEARNING.md                关键代码讲解与推荐阅读路线
docs/first-window.md            保留的第一阶段学习说明（历史文档）
dist/                          本地生成的发布包，不纳入 Git
```

依赖方向：`UI → PlannerService → Database`。UI 与小组件使用同一个 service，通过 `todosChanged/notesChanged` 刷新。
两个静态库只是构建边界，不是插件或多进程系统；无通用 Repository 模板、单例容器或预先实现的同步系统。
所有数据库操作都在 GUI 线程，适合个人规模数据。大量任务/笔记时应再引入模型视图、分页或独立存储线程。
已初始化本地 Git 仓库，不修改全局 Git 身份、不设置远程仓库。首次保存可自行运行 `git add .` 与 `git commit`。

## 测试与已知限制

以 `docs/TEST_RESULTS.md` 为准，不能将源码实现视为人工验收通过。
自动测试覆盖 CRUD、Unicode、可空截止时间、重启恢复、筛选排序、提醒去重与取消、自动保存、删除确认、失败保护和共享窗口刷新。
Windows 原生 Qt 窗口测试已执行；实际通知显示、标题栏拖动、登录启动以及无开发环境电脑运行需人工验证。

已知边界：

- 完全退出后不提醒；不存在后台守护进程。
- 提醒没有系统送达回执，采用最多一次提交，存在极小的崩溃窗口。
- 日历按计划日期与本地截止日期展示，未安排且没有截止时间的任务在“所有事项”。
- 笔记搜索暂用内存子串匹配，非全文检索；纯文本，无附件或 Markdown 渲染。
- 小组件是普通可缩放窗口，不嵌入 Explorer，也不是原生 Windows Widgets。
- 未实现安装器、代码签名、自动更新、回收站、重复任务、导入导出与云同步。
- 单实例重复启动只提示去托盘打开，不主动激活已有窗口。
- 未承诺所有分辨率、多显示器组合和输入法均已验证；高 DPI 的实际检查范围见测试报告。

## 后续手机、iPad 与同步

1. 先确认华为手机运行环境：Android/兼容 Android 与纯鸿蒙需要不同技术评估，不能假定都能直接运行 Qt Android 包。
2. 数据和业务复用 C++/Qt Core/Qt SQL；手机触屏界面可单独用 Qt Quick，不强行复用桌面 Widgets。
3. iPad 构建与签名需要 macOS、Xcode 和相应发布配置；后台通知与桌面组件遵循 iOS 平台规则。
4. 云同步先设计账号、服务端协议、删除标记、客户端操作队列、版本号和冲突策略。UUID 和 UTC 时间只是基础，不代表已有同步能力。
5. 做数据库迁移时保留旧用户数据，增加测试覆盖离线修改、幂等请求、同步失败重试和跨设备冲突。

官方参考：[Qt CMake](https://doc.qt.io/qt-6/cmake-get-started.html)、[Qt SQL](https://doc.qt.io/qt-6/sql-driver.html)、[托盘通知及限制](https://doc.qt.io/qt-6/qsystemtrayicon.html)、[Windows 发布](https://doc.qt.io/qt-6/windows-deployment.html)、[Qt for iOS](https://doc.qt.io/qt-6/ios.html)。


## V1.1.1 界面微调

正文和笔记字号略微增大，标题字重调轻；提示和空状态更自然，侧栏保持舒展间距。运行包为 DesktopPlanner-v1.1.1；先退出托盘里的旧版再运行。数据格式仍为 v2。
