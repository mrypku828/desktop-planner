#pragma once
#include "platform/AppSettings.h"
#include "services/PlannerService.h"
#include <QWidget>
class QListWidget;
class QLineEdit;
class QLabel;

// 保留系统标题栏：系统负责拖动、缩放及键盘可访问性。
class DesktopWidget : public QWidget
{
    Q_OBJECT
  public:
    DesktopWidget(PlannerService &service, AppSettings &settings);
    void applySettings();
    void refresh();
    void savePosition();
  signals:
    void openManager();
    void quickNoteRequested();
    void hiddenByUser();

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    PlannerService &service_;
    AppSettings &settings_;
    QListWidget *list_;
    QLineEdit *input_;
    QLabel *summary_;
};
