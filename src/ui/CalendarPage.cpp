#include "CalendarPage.h"
#include "Theme.h"
#include "TodoDialog.h"
#include "TodoPage.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

CalendarCell::CalendarCell(QWidget *parent) : QAbstractButton(parent)
{
    setMinimumSize(48, 66);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
}
void CalendarCell::setDay(QDate date, int month, bool selected, QVector<Todo> entries)
{
    date_ = date;
    month_ = month;
    selected_ = selected;
    entries_ = std::move(entries);
    QStringList tooltip{date.toString(QStringLiteral("yyyy年M月d日"))};
    for (const auto &t : entries_)
    {
        const bool deadline = t.dueAt.isValid() && t.dueAt.toLocalTime().date() == date;
        tooltip.push_back(
            (deadline ? QStringLiteral("◆ Deadline ") : QStringLiteral("• ")) + t.title +
            (deadline ? QStringLiteral(" · ") + t.dueAt.toLocalTime().toString("HH:mm")
                      : QString()) +
            (t.completed ? QStringLiteral("（已完成）") : QString()));
    }
    setToolTip(tooltip.join('\n'));
    setAccessibleName(tooltip.join(QStringLiteral("，")));
    update();
}
void CalendarCell::paintEvent(QPaintEvent *)
{
    const auto theme = themeForId(qApp->property("themeId").toString());
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF card = QRectF(rect()).adjusted(2, 2, -2, -2);
    const bool thisMonth = date_.month() == month_;
    painter.setPen(
        QPen(selected_ ? theme.accent : QColor(255, 255, 255, 180), selected_ ? 1.6 : 1));
    painter.setBrush(QColor(255, 255, 255, thisMonth ? (underMouse() ? 235 : 190) : 95));
    painter.drawRoundedRect(card, 10, 10);
    const bool today = date_ == QDate::currentDate();
    QRect dayRect(10, 8, 26, 23);
    if (today)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme.accent);
        painter.drawRoundedRect(dayRect, 7, 7);
    }
    QFont numberFont = font();
    numberFont.setBold(today || selected_);
    painter.setFont(numberFont);
    painter.setPen(today ? Qt::white : (thisMonth ? QColor("#344456") : QColor("#8d9aa7")));
    painter.drawText(dayRect, Qt::AlignCenter, QString::number(date_.day()));
    if (hasFocus())
    {
        painter.setPen(QPen(theme.accent, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(card.adjusted(3, 3, -3, -3), 8, 8);
    }
    const int lineHeight = fontMetrics().height() + 8;
    const int capacity = qMax(0, (height() - 38) / lineHeight);
    const int visible = qMin(capacity, static_cast<int>(entries_.size()));
    int y = 34;
    for (int i = 0; i < visible; ++i)
    {
        const auto &t = entries_[i];
        const bool deadline = t.dueAt.isValid() && t.dueAt.toLocalTime().date() == date_;
        const bool overdue = deadline && !t.completed && t.dueAt < QDateTime::currentDateTimeUtc();
        const QRect chip(8, y, width() - 16, lineHeight - 3);
        painter.setPen(Qt::NoPen);
        painter.setBrush(t.completed ? QColor("#edf0f3")
                                     : (deadline ? QColor("#f7e0e5") : theme.soft));
        painter.drawRoundedRect(chip, 5, 5);
        QFont taskFont = font();
        taskFont.setPointSizeF(qMax(8.0, taskFont.pointSizeF() - 1));
        taskFont.setStrikeOut(t.completed);
        painter.setFont(taskFont);
        painter.setPen(t.completed ? QColor("#82909b")
                                   : (deadline ? QColor(overdue ? "#a83f58" : "#965369")
                                               : theme.accent.darker(130)));
        const QString text = (deadline ? QStringLiteral("◆ ") : QStringLiteral("• ")) + t.title;
        painter.drawText(chip.adjusted(5, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                         painter.fontMetrics().elidedText(text, Qt::ElideRight, chip.width() - 9));
        y += lineHeight;
    }
    if (entries_.size() > visible)
    {
        painter.setFont(font());
        painter.setPen(theme.accent);
        painter.drawText(QRect(38, 8, width() - 48, 23), Qt::AlignVCenter | Qt::AlignRight,
                         QStringLiteral("+%1").arg(entries_.size() - visible));
    }
}

CalendarPage::CalendarPage(PlannerService &service, QWidget *parent)
    : QWidget(parent), service_(service),
      month_(QDate::currentDate().year(), QDate::currentDate().month(), 1),
      selected_(QDate::currentDate())
{
    setObjectName("calendarPage");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(10);
    auto *top = new QHBoxLayout;
    auto *heading = new QLabel(QStringLiteral("计划日历"), this);
    heading->setProperty("heading", true);
    top->addWidget(heading);
    top->addStretch();
    auto *task = new QPushButton(QStringLiteral("＋ 任务"), this);
    task->setObjectName("calendarAddTodo");
    task->setProperty("primary", true);
    auto *deadline = new QPushButton(QStringLiteral("＋ Deadline"), this);
    deadline->setObjectName("calendarAddDeadline");
    top->addWidget(task);
    top->addWidget(deadline);
    layout->addLayout(top);
    auto *monthBar = new QHBoxLayout;
    auto *previous = new QPushButton(QStringLiteral("‹"), this);
    previous->setAccessibleName(QStringLiteral("上个月"));
    previous->setObjectName("previousMonth");
    auto *next = new QPushButton(QStringLiteral("›"), this);
    next->setAccessibleName(QStringLiteral("下个月"));
    next->setObjectName("nextMonth");
    auto *today = new QPushButton(QStringLiteral("今天"), this);
    today->setObjectName("calendarToday");
    monthLabel_ = new QLabel(this);
    monthLabel_->setObjectName("calendarMonthLabel");
    QFont f = monthLabel_->font();
    f.setPointSize(15);
    f.setWeight(QFont::Medium);
    monthLabel_->setFont(f);
    monthBar->addWidget(previous);
    monthBar->addWidget(monthLabel_);
    monthBar->addWidget(next);
    monthBar->addWidget(today);
    monthBar->addStretch();
    summary_ = new QLabel(this);
    summary_->setProperty("muted", true);
    monthBar->addWidget(summary_);
    layout->addLayout(monthBar);
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(5);
    grid->setVerticalSpacing(5);
    const QStringList weekdays = {QStringLiteral("一"), QStringLiteral("二"), QStringLiteral("三"),
                                  QStringLiteral("四"), QStringLiteral("五"), QStringLiteral("六"),
                                  QStringLiteral("日")};
    for (int column = 0; column < 7; ++column)
    {
        auto *label = new QLabel(weekdays[column], this);
        label->setAlignment(Qt::AlignCenter);
        label->setProperty("muted", true);
        grid->addWidget(label, 0, column);
        grid->setColumnStretch(column, 1);
    }
    for (int i = 0; i < 42; ++i)
    {
        auto *cell = new CalendarCell(this);
        cell->setObjectName(QStringLiteral("calendarCell%1").arg(i));
        cells_.push_back(cell);
        grid->addWidget(cell, 1 + i / 7, i % 7);
        grid->setRowStretch(1 + i / 7, 1);
        connect(cell, &QAbstractButton::clicked, this, [this, cell] { openDate(cell->date()); });
    }
    layout->addLayout(grid, 1);
    selectionLabel_ = new QLabel(this);
    selectionLabel_->setProperty("muted", true);
    selectionLabel_->setWordWrap(true);
    layout->addWidget(selectionLabel_);
    connect(previous, &QPushButton::clicked, this, [this] { setMonth(month_.addMonths(-1)); });
    connect(next, &QPushButton::clicked, this, [this] { setMonth(month_.addMonths(1)); });
    connect(today, &QPushButton::clicked, this,
            [this]
            {
                selected_ = QDate::currentDate();
                setMonth(selected_);
            });
    connect(task, &QPushButton::clicked, this, [this] { addItem(false); });
    connect(deadline, &QPushButton::clicked, this, [this] { addItem(true); });
    connect(&service_, &PlannerService::todosChanged, this, &CalendarPage::refresh);
    refresh();
}
void CalendarPage::setMonth(QDate month)
{
    if (!month.isValid())
        return;
    month_ = QDate(month.year(), month.month(), 1);
    if (selected_.year() != month_.year() || selected_.month() != month_.month())
        selected_ =
            QDate(month_.year(), month_.month(), qMin(selected_.day(), month_.daysInMonth()));
    refresh();
}
void CalendarPage::refresh()
{
    monthLabel_->setText(month_.toString(QStringLiteral("yyyy年 M月")));
    const QDate start = month_.addDays(1 - month_.dayOfWeek());
    for (int i = 0; i < cells_.size(); ++i)
    {
        const auto date = start.addDays(i);
        cells_[i]->setDay(date, month_.month(), date == selected_, service_.todosForDate(date));
    }
    int deadlines = 0, unfinished = 0;
    for (const auto &t : service_.todos())
    {
        const auto due = t.dueAt.toLocalTime().date();
        if (!t.completed && due.year() == month_.year() && due.month() == month_.month())
            ++deadlines;
        if (!t.completed && t.plannedDate.year() == month_.year() &&
            t.plannedDate.month() == month_.month())
            ++unfinished;
    }
    summary_->setText(QStringLiteral("%1 项计划  /  %2 项截止").arg(unfinished).arg(deadlines));
    selectionLabel_->setText(QStringLiteral("%1  ·  点选一天，安排当天事项  ·  ◆ 粉色标记截止日")
                                 .arg(selected_.toString("MM月dd日")));
}
void CalendarPage::addItem(bool deadline)
{
    Todo value;
    value.isDeadline = deadline;
    if (deadline)
        value.dueAt = QDateTime(selected_, QTime(23, 59)).toUTC();
    else
        value.plannedDate = selected_;
    TodoDialog dialog(value, this);
    if (dialog.exec() == QDialog::Accepted && !service_.saveTodo(dialog.value()))
        QMessageBox::warning(this, QStringLiteral("保存失败"), service_.lastError());
}
void CalendarPage::openDate(QDate date)
{
    if (!date.isValid())
        return;
    selected_ = date;
    setMonth(date);
    QDialog dialog(this);
    dialog.setObjectName("dayTasksDialog");
    dialog.setWindowTitle(date.toString(QStringLiteral("yyyy年M月d日 · 当天事项")));
    dialog.resize(680, 560);
    auto *layout = new QVBoxLayout(&dialog);
    auto *page = new TodoPage(service_, true, &dialog);
    page->setDate(date);
    layout->addWidget(page);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("返回日历"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}
