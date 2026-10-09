#pragma once
#include "domain/Models.h"
#include <QDialog>
class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QDateTimeEdit;
class QDateEdit;
class QComboBox;

class TodoDialog : public QDialog
{
  public:
    explicit TodoDialog(const Todo &value, QWidget *parent = nullptr);
    Todo value() const;

  private:
    Todo original_;
    QLineEdit *title_;
    QPlainTextEdit *description_;
    QCheckBox *hasDue_;
    QDateTimeEdit *due_;
    QCheckBox *remind_;
    QDateTimeEdit *reminder_;
    QComboBox *kind_;
    QCheckBox *hasPlan_;
    QDateEdit *plan_;
};
