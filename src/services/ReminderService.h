#pragma once
#include "PlannerService.h"
#include "platform/AppSettings.h"
#include <QTimer>
class QSystemTrayIcon;

class ReminderService : public QObject
{
    Q_OBJECT
  public:
    ReminderService(PlannerService &service, AppSettings &settings, QSystemTrayIcon &tray,
                    QObject *parent = nullptr);
    void poll();
  signals:
    void delivered(const QString &title);
    void errorOccurred(const QString &message);

  private:
    PlannerService &service_;
    AppSettings &settings_;
    QSystemTrayIcon &tray_;
    QTimer timer_;
    QDateTime nextNotification_;
    QString lastError_;
};
