#pragma once

#include "platform/AppSettings.h"
#include "services/PlannerService.h"
#include <QMainWindow>
class NotesPage;
class DesktopWidget;
class QStackedWidget;
class QSystemTrayIcon;
class QCheckBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
  public:
    MainWindow(PlannerService &service, AppSettings &settings, const QString &dataDirectory,
               QWidget *parent = nullptr);
    ~MainWindow() override;
    void showManager();
    void quitApplication();

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    bool persistWindowState();
    QWidget *createSettingsPage(const QString &directory);
    PlannerService &service_;
    AppSettings &settings_;
    NotesPage *notes_;
    DesktopWidget *widget_;
    QStackedWidget *pages_;
    QSystemTrayIcon *tray_;
    QCheckBox *widgetEnabled_;
    bool exiting_ = false;
};
