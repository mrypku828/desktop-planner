#pragma once
#include "services/PlannerService.h"
#include <QWidget>
class QComboBox;
class QListWidget;
class QPushButton;
class QLabel;

class TodoPage : public QWidget
{
    Q_OBJECT
  public:
    TodoPage(PlannerService &service, bool todayOnly, QWidget *parent = nullptr);
    void refresh();
    void setDate(QDate date);

  private:
    void editSelected();
    void showError();
    QString selectedId() const;
    PlannerService &service_;
    bool todayOnly_;
    QDate date_;
    QLabel *heading_;
    QComboBox *filter_;
    QListWidget *list_;
    QLabel *summary_;
    QPushButton *edit_;
    QPushButton *remove_;
    QPushButton *complete_;
};
