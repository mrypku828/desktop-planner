#include "TodoPage.h"
#include "TodoDialog.h"
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

TodoPage::TodoPage(PlannerService &s, bool todayOnly, QWidget *parent)
    : QWidget(parent), service_(s), todayOnly_(todayOnly), date_(QDate::currentDate())
{
    auto *layout = new QVBoxLayout(this);
    auto *heading =
        new QLabel(todayOnly ? QStringLiteral("今日计划") : QStringLiteral("所有任务"), this);
    heading->setProperty("heading", true);
    heading_ = heading;
    layout->addWidget(heading);
    auto *hint = new QLabel(
        todayOnly ? QStringLiteral(
                        "显示当天安排及截止事项。普通任务可只安排日期；Deadline 必须有截止时间。")
                  : QStringLiteral("按截止时间从早到晚排序；无截止时间的任务排在末尾。"),
        this);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto *bar = new QHBoxLayout;
    auto *add = new QPushButton(QStringLiteral("＋ 新建任务"), this);
    add->setObjectName("addTodo");
    add->setProperty("primary", true);
    bar->addWidget(add);
    auto *deadline = new QPushButton(QStringLiteral("＋ Deadline"), this);
    deadline->setObjectName("addDeadline");
    bar->addWidget(deadline);
    filter_ = new QComboBox(this);
    filter_->addItems({QStringLiteral("未完成"), QStringLiteral("已完成"), QStringLiteral("全部")});
    bar->addStretch();
    bar->addWidget(filter_);
    layout->addLayout(bar);
    list_ = new QListWidget(this);
    list_->setObjectName("todoList");
    list_->setWordWrap(true);
    layout->addWidget(list_);
    summary_ = new QLabel(this);
    layout->addWidget(summary_);
    auto *actions = new QHBoxLayout;
    edit_ = new QPushButton(QStringLiteral("编辑"), this);
    remove_ = new QPushButton(QStringLiteral("删除"), this);
    complete_ = new QPushButton(QStringLiteral("切换完成状态"), this);
    edit_->setObjectName("editTodo");
    remove_->setObjectName("deleteTodo");
    complete_->setObjectName("toggleTodo");
    actions->addWidget(edit_);
    actions->addWidget(complete_);
    actions->addStretch();
    actions->addWidget(remove_);
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this,
            [this]
            {
                Todo initial;
                initial.plannedDate = todayOnly_ ? date_ : QDate::currentDate();
                TodoDialog dialog(initial, this);
                if (dialog.exec() == QDialog::Accepted && !service_.saveTodo(dialog.value()))
                    showError();
            });
    connect(deadline, &QPushButton::clicked, this,
            [this]
            {
                Todo initial;
                initial.isDeadline = true;
                initial.dueAt =
                    QDateTime(todayOnly_ ? date_ : QDate::currentDate(), QTime(23, 59)).toUTC();
                TodoDialog dialog(initial, this);
                if (dialog.exec() == QDialog::Accepted && !service_.saveTodo(dialog.value()))
                    showError();
            });
    connect(edit_, &QPushButton::clicked, this, &TodoPage::editSelected);
    connect(list_, &QListWidget::itemDoubleClicked, this, [this] { editSelected(); });
    connect(complete_, &QPushButton::clicked, this,
            [this]
            {
                auto t = service_.todo(selectedId());
                if (t && !service_.setCompleted(t->id, !t->completed))
                    showError();
            });
    connect(remove_, &QPushButton::clicked, this,
            [this]
            {
                const auto id = selectedId();
                if (id.isEmpty())
                    return;
                if (QMessageBox::question(this, QStringLiteral("删除任务"),
                                          QStringLiteral("删除此任务及其提醒？此操作不可撤销。"),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No) == QMessageBox::Yes &&
                    !service_.deleteTodo(id))
                    showError();
            });
    connect(filter_, &QComboBox::currentIndexChanged, this, [this] { refresh(); });
    connect(list_, &QListWidget::itemSelectionChanged, this,
            [this]
            {
                const bool enabled = !selectedId().isEmpty();
                edit_->setEnabled(enabled);
                remove_->setEnabled(enabled);
                complete_->setEnabled(enabled);
            });
    connect(&service_, &PlannerService::todosChanged, this, &TodoPage::refresh);
    connect(list_, &QListWidget::itemChanged, this,
            [this](QListWidgetItem *item)
            {
                const auto id = item->data(Qt::UserRole).toString();
                const bool completed = item->checkState() == Qt::Checked;
                // 通过 queued 调用，避免在 itemChanged 栈中刷新并销毁当前 item。
                QMetaObject::invokeMethod(
                    this,
                    [this, id, completed]
                    {
                        if (!service_.setCompleted(id, completed))
                        {
                            showError();
                            refresh();
                        }
                    },
                    Qt::QueuedConnection);
            });
    refresh();
}
QString TodoPage::selectedId() const
{
    auto *item = list_->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}
void TodoPage::showError()
{
    QMessageBox::warning(this, QStringLiteral("操作失败"), service_.lastError());
}
void TodoPage::editSelected()
{
    auto t = service_.todo(selectedId());
    if (!t)
        return;
    TodoDialog dialog(*t, this);
    if (dialog.exec() == QDialog::Accepted && !service_.saveTodo(dialog.value()))
        showError();
}
void TodoPage::refresh()
{
    const QString selected = selectedId();
    QSignalBlocker blocker(list_);
    list_->clear();
    const auto filter = todayOnly_ ? TodoFilter::Today : TodoFilter::All;
    const auto values = service_.todos(filter, date_);
    int count = 0;
    for (const auto &t : values)
    {
        if (filter_->currentIndex() == 0 && t.completed)
            continue;
        if (filter_->currentIndex() == 1 && !t.completed)
            continue;
        const bool overdue =
            !t.completed && t.dueAt.isValid() && t.dueAt < QDateTime::currentDateTimeUtc();
        QString detail = t.dueAt.isValid() ? QStringLiteral("◆ Deadline ") +
                                                 t.dueAt.toLocalTime().toString("yyyy-MM-dd HH:mm")
                                           : QStringLiteral("无截止时间");
        if (overdue)
            detail.prepend(QStringLiteral("⚠ 已过期 · "));
        if (t.plannedDate.isValid())
            detail += QStringLiteral(" · 计划 ") + t.plannedDate.toString("MM-dd");
        if (t.reminderEnabled)
            detail += t.remindedAt.isValid()
                          ? QStringLiteral(" · 已提醒")
                          : QStringLiteral(" · 提醒 ") +
                                t.reminderAt.toLocalTime().toString("MM-dd HH:mm");
        auto *item = new QListWidgetItem((t.isDeadline ? QStringLiteral("◆ ") : QString()) +
                                             t.title + QStringLiteral("\n") + detail,
                                         list_);
        item->setData(Qt::UserRole, t.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(t.completed ? Qt::Checked : Qt::Unchecked);
        item->setToolTip(t.description);
        if (overdue)
            item->setForeground(QColor("#b42318"));
        if (t.id == selected)
            list_->setCurrentItem(item);
        ++count;
    }
    summary_->setText(count
                          ? QStringLiteral("%1 条任务 · 双击编辑，勾选完成").arg(count)
                          : QStringLiteral("这里暂时没有事项。想安排些什么？点“新建任务”开始吧。"));
    const bool enabled = !selectedId().isEmpty();
    edit_->setEnabled(enabled);
    remove_->setEnabled(enabled);
    complete_->setEnabled(enabled);
}
void TodoPage::setDate(QDate date)
{
    if (!date.isValid())
        return;
    date_ = date;
    heading_->setText(date.toString(QStringLiteral("M月d日 · 当天事项")));
    refresh();
}
