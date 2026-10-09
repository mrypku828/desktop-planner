#include "TodoDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

TodoDialog::TodoDialog(const Todo &t, QWidget *parent) : QDialog(parent), original_(t)
{
    setWindowTitle(t.id.isEmpty() ? QStringLiteral("新建任务") : QStringLiteral("编辑任务"));
    resize(520, 610);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    kind_ = new QComboBox(this);
    kind_->setObjectName("todoKind");
    kind_->addItems({QStringLiteral("普通任务"), QStringLiteral("Deadline · 截止事项")});
    kind_->setCurrentIndex(t.isDeadline ? 1 : 0);
    hasPlan_ = new QCheckBox(QStringLiteral("安排到日历日期"), this);
    hasPlan_->setChecked(t.plannedDate.isValid());
    hasPlan_->setObjectName("hasPlannedDate");
    plan_ = new QDateEdit(t.plannedDate.isValid() ? t.plannedDate : QDate::currentDate(), this);
    plan_->setObjectName("plannedDate");
    plan_->setCalendarPopup(true);
    plan_->setDisplayFormat("yyyy-MM-dd");
    title_ = new QLineEdit(t.title, this);
    title_->setObjectName("todoTitle");
    title_->setMaxLength(500);
    title_->setPlaceholderText(QStringLiteral("准备做些什么？"));
    description_ = new QPlainTextEdit(t.description, this);
    description_->setObjectName("todoDescription");
    description_->setPlaceholderText(QStringLiteral("补充一些细节，或留给之后的自己…（选填）"));
    hasDue_ = new QCheckBox(QStringLiteral("设置 Deadline / 截止时间"), this);
    hasDue_->setObjectName("hasDueDate");
    hasDue_->setChecked(t.dueAt.isValid());
    due_ = new QDateTimeEdit(t.dueAt.isValid() ? t.dueAt.toLocalTime()
                                               : QDateTime::currentDateTime().addSecs(3600),
                             this);
    due_->setCalendarPopup(true);
    due_->setObjectName("dueDateTime");
    due_->setDisplayFormat("yyyy-MM-dd HH:mm");
    due_->setEnabled(hasDue_->isChecked());
    remind_ = new QCheckBox(QStringLiteral("启用提醒（独立于截止时间）"), this);
    remind_->setChecked(t.reminderEnabled);
    remind_->setEnabled(!t.completed);
    reminder_ =
        new QDateTimeEdit(t.reminderAt.isValid() ? t.reminderAt.toLocalTime()
                                                 : QDateTime::currentDateTime().addSecs(300),
                          this);
    reminder_->setCalendarPopup(true);
    reminder_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    reminder_->setEnabled(remind_->isChecked());
    form->addRow(QStringLiteral("类型"), kind_);
    form->addRow(QStringLiteral("标题"), title_);
    form->addRow(QStringLiteral("描述"), description_);
    form->addRow(hasPlan_);
    form->addRow(QStringLiteral("计划日期"), plan_);
    form->addRow(hasDue_);
    form->addRow(QStringLiteral("截止"), due_);
    form->addRow(remind_);
    form->addRow(QStringLiteral("提醒"), reminder_);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    layout->addWidget(buttons);
    auto updateKind = [this]
    {
        const bool deadline = kind_->currentIndex() == 1;
        hasPlan_->setEnabled(!deadline);
        plan_->setEnabled(!deadline && hasPlan_->isChecked());
        if (deadline)
            hasDue_->setChecked(true);
        hasDue_->setEnabled(!deadline);
        due_->setEnabled(hasDue_->isChecked());
        setWindowTitle(deadline ? QStringLiteral("编辑 Deadline") : QStringLiteral("编辑任务"));
    };
    connect(kind_, &QComboBox::currentIndexChanged, this, [updateKind] { updateKind(); });
    connect(hasPlan_, &QCheckBox::toggled, this, [updateKind] { updateKind(); });
    updateKind();
    connect(hasDue_, &QCheckBox::toggled, due_, &QWidget::setEnabled);
    connect(remind_, &QCheckBox::toggled, reminder_, &QWidget::setEnabled);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this,
            [this]
            {
                if (title_->text().trimmed().isEmpty())
                {
                    QMessageBox::information(this, QStringLiteral("还差一个标题"),
                                             QStringLiteral("给这件事起个名字，就可以保存了。"));
                    title_->setFocus();
                    return;
                }
                accept();
            });
    title_->setFocus();
}
Todo TodoDialog::value() const
{
    Todo t = original_;
    t.title = title_->text();
    t.description = description_->toPlainText();
    t.isDeadline = kind_->currentIndex() == 1;
    t.plannedDate = !t.isDeadline && hasPlan_->isChecked() ? plan_->date() : QDate();
    t.dueAt = hasDue_->isChecked() ? due_->dateTime().toUTC() : QDateTime();
    t.reminderEnabled = remind_->isChecked() && !t.completed;
    t.reminderAt = t.reminderEnabled ? reminder_->dateTime().toUTC() : QDateTime();
    return t;
}
