# 桌面计划管理：从第一个窗口开始

采用 C++17、Qt 6 Widgets、CMake，后续通过 Qt SQL 的 QSQLITE 驱动使用 SQLite。
当前只实现主窗口，尚未实现任务、笔记、提醒、悬浮组件或数据库。

## 1. 简单架构

调用方向：界面 → 业务服务 → 数据存储。数据结构由这些模块共用。
开始时只有一个可执行程序，不拆多套库，不引入插件系统、依赖注入框架或通用 Repository 模板。
类在真正需要时才添加，避免为了分层写大量转发函数。

| 文件或目录 | 作用 | 状态 |
| --- | --- | --- |
| `CMakeLists.txt` | 指定 C++ 标准、源文件和 Qt 依赖 | 已实现 |
| `src/main.cpp` | 创建应用和主窗口，启动事件循环 | 已实现 |
| `src/ui/MainWindow.h/.cpp` | 主窗口声明和布局 | 已实现 |
| `.gitignore` | 排除生成的编译文件与个人配置 | 已实现 |
| `src/domain/Todo.h` | 任务数据：ID、标题、完成状态、可选截止时间 | 按需添加 |
| `src/domain/Note.h` | 笔记数据：ID、内容、更新时间 | 按需添加 |
| `src/domain/Reminder.h` | 提醒数据：ID、触发时间、触发状态、可选任务 ID | 按需添加 |
| `src/storage/Database.h/.cpp` | 打开 SQLite、建表和数据库版本升级 | 按需添加 |
| `src/storage/TodoRepository.h/.cpp` | 使用参数绑定执行任务增删改查 | 按需添加 |
| `src/storage/NoteRepository.h/.cpp` | 笔记持久化 | 按需添加 |
| `src/storage/ReminderRepository.h/.cpp` | 提醒持久化 | 按需添加 |
| `src/services/TodoService.h/.cpp` | 标题校验、任务操作；变化后通知界面刷新 | 有持久化时添加 |
| `src/services/ReminderService.h/.cpp` | 检查到期时间、防止重复触发、恢复后的补检 | 按需添加 |
| `src/ui/TodoPage.h/.cpp` | 任务输入、列表、编辑 | 按需添加 |
| `src/ui/NotesPage.h/.cpp` | 笔记列表和编辑器 | 按需添加 |
| `src/ui/DesktopWidget.h/.cpp` | 独立的小型置顶窗口，与主窗口共享业务服务 | 按需添加 |
| `src/platform/` | 需要时封装 Windows 通知等系统能力 | 按需添加 |
| `src/sync/` | 将来放同步逻辑；现在不创建 | 远期 |
| `tests/` | 业务规则、SQLite 读写、提醒边界的测试 | 相应功能落地时添加 |

UI 不写 SQL；数据库代码不弹消息框，而是把错误交给调用方处理。
初版内存 Todo 可以先放在 TodoPage 中，接入存储时再提取业务服务。
使用 Qt 信号与槽传递“数据已变化”通知；主窗口和悬浮窗口共享同一份数据来源。
不需要给所有类添加单例，也不用提前增加线程。

### 数据与扩展约定

- SQLite 是本地文件，不需要安装数据库服务器。数据文件放在 `QStandardPaths::AppLocalDataLocation`，不要写入源码或安装目录。
- 第一次建表时采用 UUID 文本主键，时间统一保存为 UTC，显示时转换为本地时间。截止时间和提醒时间是两个概念，均可为空。
- 数据库升级使用 `PRAGMA user_version` 管理版本，在事务中迁移，避免升级时丢失旧数据。
- Todo、Note、Reminder 分表，具体字段随功能引入。接入同步前再增加删除标记、服务端版本等字段，并明确冲突规则。UUID 和更新时间本身不能解决同步冲突。
- Windows 悬浮组件先做普通无边框、可拖动、可置顶窗口，不做 Explorer 桌面嵌入。
- Android/iPad 争取复用领域数据、存储和业务规则。触屏界面可能改用 Qt Quick；后台提醒和桌面小组件需要各平台实现，不能直接搬运 Windows 窗口。iPad 的构建与签名需要 macOS/Xcode。

## 2. Windows 环境

本机已发现：

- Qt：`D:\qt\6.11.1\mingw_64`
- 配套编译器：`D:\qt\Tools\mingw1310_64\bin`
- CMake：`D:\qt\Tools\CMake_64\bin\cmake.exe`
- Ninja：`D:\qt\Tools\Ninja\ninja.exe`
- Qt Creator：`D:\qt\Tools\QtCreator`
- SQLite 插件：`D:\qt\6.11.1\mingw_64\plugins\sqldrivers\qsqlite.dll`

系统 PATH 默认的 g++ 是另一套 i686（32 位）工具链，不能与这套 64 位 Qt 混用。

### 推荐：Qt Creator

1. 启动 Qt Creator，选择“打开文件或项目”，打开本目录的 `CMakeLists.txt`。
2. 选择 Desktop Qt 6.11.1 MinGW 64-bit Kit；若未自动发现，在首选项的 Kits 中设置上述 Qt、配套编译器、CMake 和 Ninja。
3. 使用独立构建目录，例如项目下 `build/creator`，选择 Debug。
4. 点击构建，再点击运行。修改代码后再次构建运行。

换到新电脑时，用 Qt Online Installer 安装一个稳定的 Qt 6 桌面 MinGW 64-bit 套件，以及安装器提供的对应 MinGW、Qt Creator、CMake、Ninja。各版本支持的编译器应以该版本官方说明和安装器为准。无需同时安装 Visual Studio；若选择 MSVC 则 Qt 库和编译器也必须一起换成匹配的 MSVC 套件。

### 本机 PowerShell 构建

在项目目录运行；PATH 仅修改当前终端会话：

```powershell
$env:PATH = 'D:\qt\Tools\mingw1310_64\bin;D:\qt\6.11.1\mingw_64\bin;D:\qt\Tools\Ninja;' + $env:PATH
& 'D:\qt\Tools\CMake_64\bin\cmake.exe' -S . -B build/mingw-debug -G Ninja '-DCMAKE_BUILD_TYPE=Debug' '-DCMAKE_PREFIX_PATH=D:/qt/6.11.1/mingw_64' '-DCMAKE_CXX_COMPILER=D:/qt/Tools/mingw1310_64/bin/g++.exe' '-DCMAKE_MAKE_PROGRAM=D:/qt/Tools/Ninja/ninja.exe'
& 'D:\qt\Tools\CMake_64\bin\cmake.exe' --build build/mingw-debug
& '.\build\mingw-debug\DesktopPlanner.exe'
```

切换 Qt 或编译器时换一个构建目录，避免使用旧 CMake 缓存。当前命令只适用于这台电脑。
开发运行由 Qt Creator 或上面的 PATH 提供 DLL。以后分享给其他电脑时，用相同套件的 `windeployqt` 收集运行库和插件；加入 SQLite 后检查 `sqldrivers/qsqlite.dll` 也被包含。

## 3. 小任务与验收

每次只完成一行或其中的一个小步骤；先说清预期行为，再编码、构建、验收和解释。

| 顺序 | 任务 | 验收方法 |
| --- | --- | --- |
| 1 | 最小窗口（当前） | 编译成功；启动显示标题、说明和状态栏；缩放正常，关闭后退出 |
| 2 | 内存 Todo：输入与添加 | 添加后立即显示；空白标题被拒绝；明确此时重启会丢失数据 |
| 3 | 完成、删除任务 | 可以切换完成状态、删除选中项；无选中项时不崩溃 |
| 4 | SQLite 建库与任务保存 | 增删改后重启仍一致；用临时数据库测试读写；写入失败明确提示 |
| 5 | 编辑及截止时间 | 支持无截止时间；修改可保存；过期且未完成的任务有提示 |
| 6 | Notes 最小编辑器 | 新建、修改、删除、重启恢复；防止未保存内容静默丢失 |
| 7 | 应用运行期间的 Reminder | 设置一分钟后提醒；只触发一次；持久化触发状态；测试跨过到期时间 |
| 8 | 托盘与恢复补检 | 明确“关闭窗口”和“退出”区别；休眠恢复/重启补检过期提醒；程序退出期间不承诺通知 |
| 9 | 悬浮小组件 | 显示未完成任务；主窗口修改后同步刷新；可拖动且位置能恢复 |
| 10 | 发布包 | 在无开发环境的 Windows 上验证启动、SQLite、提醒 |
| 11 | 云同步设计与原型 | 先确定账号、服务端和冲突策略；再验证离线修改、重复请求、多设备冲突 |

第 4 步引入 `Qt6::Sql`，不是现在就增加未使用的依赖。先打通任务完整闭环，再铺开 Notes。
提醒测试用可控制的当前时间，避免测试中真实等待一分钟。

## 4. 第一课：关键代码

- `main.cpp`：`QApplication` 管理 GUI 应用。先创建它，再创建任何窗口。`window.show()` 显示窗口，`app.exec()` 进入事件循环，处理点击、重绘和关闭。窗口是局部变量，在事件循环退出后销毁。
- `MainWindow.h`：头文件声明类；`public QMainWindow` 表示继承 Qt 主窗口能力；`explicit` 避免构造函数参与意外的隐式转换，默认 `parent = nullptr` 表示可作为顶层窗口。
- `MainWindow.cpp`：构造函数创建界面。`QMainWindow` 有自己的结构，所以通过 `setCentralWidget()` 放入内容容器，再给容器设置 `QVBoxLayout`。
- 标签的父对象是 `central`，`central` 的父对象是主窗口。Qt 父子对象机制负责销毁子控件，所以这里的 `new` 不需要手动逐个 `delete`。这不意味着所有 C++ 的 `new` 都能省略释放。
- `QStringLiteral` 创建 Qt 字符串；源码保存为 UTF-8。布局负责调整控件位置，避免写死坐标；`addStretch()` 把内容推向上方。
- 当前没有自定义信号、属性或元对象需求，因此没有添加 `Q_OBJECT`。将来需要这些能力时再添加。
- `CMakeLists.txt`：`find_package` 找到 Qt；`qt_add_executable` 定义程序；`target_link_libraries` 连接 Widgets；`WIN32_EXECUTABLE` 避免额外控制台窗口。

学习方式：每一步请先读懂数据放在哪里、谁拥有对象、点击后调用哪个函数、失败如何反馈。使用 AI 时说明“只做本步骤、给出验收条件、解释新增概念”，每个可运行阶段再保存一个 Git 提交。

## 官方参考

- [Qt 安装](https://doc.qt.io/qt-6/get-and-install-qt.html)
- [Qt 与 CMake 入门](https://doc.qt.io/qt-6/cmake-get-started.html)
- [Qt SQL 驱动与 SQLite](https://doc.qt.io/qt-6/sql-driver.html)
- [Windows 发布](https://doc.qt.io/qt-6/windows-deployment.html)
- [Qt for iOS](https://doc.qt.io/qt-6/ios.html)
