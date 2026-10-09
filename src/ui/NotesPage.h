#pragma once
#include "services/PlannerService.h"
#include <QTimer>
#include <QWidget>
class QLineEdit;
class QPlainTextEdit;
class QListWidget;
class QLabel;
class QPushButton;

class NotesPage : public QWidget
{
    Q_OBJECT
  public:
    explicit NotesPage(PlannerService &service, QWidget *parent = nullptr);
    bool flush();
    void quickNote();

  private:
    void refreshList();
    void load(const QString &id);
    void markDirty();
    PlannerService &service_;
    QString activeId_;
    bool loading_ = false;
    bool dirty_ = false;
    QTimer saveTimer_;
    QLineEdit *search_;
    QListWidget *list_;
    QLineEdit *title_;
    QPlainTextEdit *content_;
    QLabel *status_;
    QPushButton *remove_;
};
