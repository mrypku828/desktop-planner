#include "DesktopWidget.h"
#include <QCloseEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

DesktopWidget::DesktopWidget(PlannerService &s, AppSettings &settings)
    : service_(s), settings_(settings)
{
    setWindowTitle(QStringLiteral("今日计划 · 小组件"));
    setObjectName("desktopWidget");
    setWindowOpacity(qBound(80, settings_.get("appearance/opacity", 94).toInt(), 100) / 100.0);
    resize(340, 380);
    setMinimumSize(280, 220);
    restoreGeometry(settings_.get("widget/geometry").toByteArray());
    bool onScreen = false;
    for (auto *screen : QGuiApplication::screens())
        if (screen->availableGeometry().contains(frameGeometry().topLeft() + QPoint(30, 15)))
            onScreen = true;
    if (!onScreen)
        move(QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(40, 40));
    auto *layout = new QVBoxLayout(this);
    summary_ = new QLabel(this);
    layout->addWidget(summary_);
    list_ = new QListWidget(this);
    list_->setObjectName("widgetTodoList");
    list_->setWordWrap(true);
    layout->addWidget(list_, 1);
    auto *bar = new QHBoxLayout;
    input_ = new QLineEdit(this);
    input_->setObjectName("widgetQuickTodo");
    input_->setPlaceholderText(QStringLiteral("快速添加今日任务"));
    input_->setMaxLength(500);
    auto *add = new QPushButton(QStringLiteral("添加"), this);
    bar->addWidget(input_, 1);
    bar->addWidget(add);
    layout->addLayout(bar);
    auto *links = new QHBoxLayout;
    auto *note = new QPushButton(QStringLiteral("随手记"), this);
    auto *manager = new QPushButton(QStringLiteral("打开主窗口"), this);
    links->addWidget(note);
    links->addWidget(manager);
    layout->addLayout(links);
    auto quickAdd = [this]
    {
        if (input_->text().trimmed().isEmpty())
            return;
        Todo t;
        t.title = input_->text();
        t.plannedDate = QDate::currentDate();
        if (service_.saveTodo(t))
            input_->clear();
        else
            QMessageBox::warning(this, QStringLiteral("添加失败"), service_.lastError());
    };
    connect(add, &QPushButton::clicked, this, quickAdd);
    connect(input_, &QLineEdit::returnPressed, this, quickAdd);
    connect(manager, &QPushButton::clicked, this, &DesktopWidget::openManager);
    connect(note, &QPushButton::clicked, this, &DesktopWidget::quickNoteRequested);
    connect(&service_, &PlannerService::todosChanged, this, &DesktopWidget::refresh);
    connect(list_, &QListWidget::itemDoubleClicked, this, [this] { emit openManager(); });
    connect(list_, &QListWidget::itemChanged, this,
            [this](QListWidgetItem *item)
            {
                if (item->checkState() != Qt::Checked)
                    return;
                const auto id = item->data(Qt::UserRole).toString();
                QMetaObject::invokeMethod(
                    this,
                    [this, id]
                    {
                        if (!service_.setCompleted(id, true))
                        {
                            QMessageBox::warning(this, QStringLiteral("保存失败"),
                                                 service_.lastError());
                            refresh();
                        }
                    },
                    Qt::QueuedConnection);
            });
    refresh();
}
void DesktopWidget::refresh()
{
    QSignalBlocker blocker(list_);
    list_->clear();
    int count = 0;
    for (const auto &t : service_.todos(TodoFilter::Today))
    {
        if (t.completed)
            continue;
        const bool overdue = t.dueAt.isValid() && t.dueAt < QDateTime::currentDateTimeUtc();
        auto *item = new QListWidgetItem(
            t.title + QStringLiteral("\n") + (overdue ? QStringLiteral("已过期 · ") : QString()) +
                (t.dueAt.isValid() ? QStringLiteral("◆ ") + t.dueAt.toLocalTime().toString("HH:mm")
                                   : QStringLiteral("今日安排 · 无截止")),
            list_);
        item->setData(Qt::UserRole, t.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        if (overdue)
            item->setForeground(QColor("#b42318"));
        ++count;
    }
    summary_->setText(QStringLiteral("%1 · 今日待办 %2 条")
                          .arg(QDate::currentDate().toString("MM月dd日"))
                          .arg(count));
}
void DesktopWidget::applySettings()
{
    const auto geometryBefore = geometry();
    setWindowFlag(Qt::WindowStaysOnTopHint, settings_.widgetOnTop());
    setGeometry(geometryBefore);
    setVisible(settings_.widgetEnabled());
}
void DesktopWidget::savePosition()
{
    settings_.set("widget/geometry", saveGeometry());
}
void DesktopWidget::closeEvent(QCloseEvent *event)
{
    savePosition();
    event->ignore();
    hide();
    emit hiddenByUser();
}
