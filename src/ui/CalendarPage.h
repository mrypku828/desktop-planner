#pragma once
#include "services/PlannerService.h"
#include <QAbstractButton>
#include <QWidget>
class QLabel;

class CalendarCell : public QAbstractButton
{
  public:
    explicit CalendarCell(QWidget *parent = nullptr);
    void setDay(QDate date, int month, bool selected, QVector<Todo> entries);
    QDate date() const
    {
        return date_;
    }
    QVector<Todo> entries() const
    {
        return entries_;
    }
    QSize sizeHint() const override
    {
        return {110, 90};
    }

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    QDate date_;
    int month_ = 0;
    bool selected_ = false;
    QVector<Todo> entries_;
};

class CalendarPage : public QWidget
{
    Q_OBJECT
  public:
    explicit CalendarPage(PlannerService &service, QWidget *parent = nullptr);
    void refresh();
    void setMonth(QDate month);
    void openDate(QDate date);
    QDate displayedMonth() const
    {
        return month_;
    }
    QDate selectedDate() const
    {
        return selected_;
    }

  private:
    void addItem(bool deadline);
    PlannerService &service_;
    QDate month_;
    QDate selected_;
    QLabel *monthLabel_;
    QLabel *summary_;
    QLabel *selectionLabel_;
    QVector<CalendarCell *> cells_;
};
