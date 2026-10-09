#include "ReminderService.h"
#include <QSystemTrayIcon>

ReminderService::ReminderService(PlannerService &s, AppSettings &settings, QSystemTrayIcon &tray,
                                 QObject *parent)
    : QObject(parent), service_(s), settings_(settings), tray_(tray)
{
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &ReminderService::poll);
    timer_.start();
}
void ReminderService::poll()
{
    if (!settings_.remindersEnabled())
        return;
    if (!QSystemTrayIcon::isSystemTrayAvailable() || !QSystemTrayIcon::supportsMessages() ||
        !tray_.isVisible())
        return;
    const auto now = QDateTime::currentDateTimeUtc();
    if (nextNotification_.isValid() && now < nextNotification_)
        return;
    // 一次领取一条；多条通知间隔显示，避免相互覆盖。
    const auto due = service_.claimDueReminders(now);
    const auto error = service_.lastError();
    if (!error.isEmpty() && error != lastError_)
        emit errorOccurred(error);
    lastError_ = error;
    for (const auto &t : due)
    {
        tray_.showMessage(QStringLiteral("计划提醒"), t.title, QSystemTrayIcon::Information, 10000);
        emit delivered(t.title);
        nextNotification_ = now.addSecs(12);
    }
}
