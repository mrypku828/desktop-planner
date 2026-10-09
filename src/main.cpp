#include "ui/AppStyle.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTemporaryDir>
#include <QTimer>
#include <QTranslator>
#include <memory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DesktopPlanner"));
    app.setOrganizationName(QStringLiteral("DesktopPlanner"));
    app.setApplicationVersion("1.1.1");
    applyAppStyle(app);
    QTranslator translator;
    if (translator.load("qtbase_zh_CN", app.applicationDirPath() + "/translations") ||
        translator.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&translator);
    // 发布包启动检查使用临时数据，绝不更改实际用户数据。
    const bool smokeTest = app.arguments().contains("--smoke-test");
    std::unique_ptr<QTemporaryDir> temporary;
    if (smokeTest)
        temporary = std::make_unique<QTemporaryDir>();
    const QString directory =
        smokeTest ? temporary->path()
                  : QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!QDir().mkpath(directory))
    {
        QMessageBox::critical(nullptr, QStringLiteral("启动失败"),
                              QStringLiteral("无法创建数据目录：") + directory);
        return 1;
    }
    QLockFile lock(directory + "/planner.lock");
    lock.setStaleLockTime(0);
    if (!lock.tryLock())
    {
        QMessageBox::information(
            nullptr, QStringLiteral("程序已运行"),
            QStringLiteral("桌面计划可能已在后台运行。\n\n点击右下角时钟旁的“∧”，找到桌面计划图标，"
                           "单击即可打开；右键可退出程序。\n\n若没有找到图标，请检查是否有 "
                           "DesktopPlanner.exe 进程或数据目录权限问题。"));
        return 1;
    }
    Database database;
    QString error;
    if (!database.open(directory + "/planner.sqlite", error))
    {
        QMessageBox::critical(nullptr, QStringLiteral("数据库启动失败"), error);
        return 1;
    }
    PlannerService service(database);
    if (!service.initialize(error))
    {
        QMessageBox::critical(nullptr, QStringLiteral("数据读取失败"), error);
        return 1;
    }
    AppSettings settings(directory + "/settings.ini");
    MainWindow window(service, settings, directory);
    app.setQuitOnLastWindowClosed(false);
    if (!app.arguments().contains("--background") || !QSystemTrayIcon::isSystemTrayAvailable())
        window.show();
    if (smokeTest)
        QTimer::singleShot(800, &window, &MainWindow::quitApplication);

    return app.exec();
}
