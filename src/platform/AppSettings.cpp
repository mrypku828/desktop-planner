#include "AppSettings.h"
#include <QCoreApplication>
#include <QDir>

namespace
{
const QString runKey =
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
QString startupCommand()
{
    return QStringLiteral("\"") +
           QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) +
           QStringLiteral("\" --background");
}
} // namespace
AppSettings::AppSettings(const QString &path) : settings_(path, QSettings::IniFormat) {}
bool AppSettings::widgetEnabled() const
{
    return get("widget/enabled", false).toBool();
}
bool AppSettings::widgetOnTop() const
{
    return get("widget/onTop", false).toBool();
}
bool AppSettings::remindersEnabled() const
{
    return get("reminders/enabled", true).toBool();
}
QVariant AppSettings::get(const QString &key, const QVariant &fallback) const
{
    return settings_.value(key, fallback);
}
bool AppSettings::set(const QString &key, const QVariant &value)
{
    const bool existed = settings_.contains(key);
    const QVariant previous = settings_.value(key);
    settings_.setValue(key, value);
    settings_.sync();
    if (settings_.status() == QSettings::NoError)
        return true;
    // 写盘失败时也恢复内存视图，使业务开关与界面回滚保持一致。
    if (existed)
        settings_.setValue(key, previous);
    else
        settings_.remove(key);
    return false;
}
bool AppSettings::startupEnabled()
{
#ifdef Q_OS_WIN
    QSettings run(runKey, QSettings::NativeFormat);
    return run.value("DesktopPlanner").toString() == startupCommand();
#else
    return false;
#endif
}
bool AppSettings::setStartupEnabled(bool enabled, QString &error)
{
#ifdef Q_OS_WIN
    QSettings run(runKey, QSettings::NativeFormat);
    if (enabled)
        run.setValue("DesktopPlanner", startupCommand());
    else
        run.remove("DesktopPlanner");
    run.sync();
    if (run.status() == QSettings::NoError)
        return true;
    error = QStringLiteral("无法修改当前用户的开机启动注册表项。");
    return false;
#else
    Q_UNUSED(enabled)
    error = QStringLiteral("仅 Windows 支持此开机启动设置。");
    return false;
#endif
}
