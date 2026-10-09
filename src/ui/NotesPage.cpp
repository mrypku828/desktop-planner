#include "NotesPage.h"
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

NotesPage::NotesPage(PlannerService &s, QWidget *parent) : QWidget(parent), service_(s)
{
    auto *layout = new QVBoxLayout(this);
    auto *heading = new QLabel(QStringLiteral("随手记"), this);
    heading->setProperty("heading", true);
    layout->addWidget(heading);
    auto *bar = new QHBoxLayout;
    auto *add = new QPushButton(QStringLiteral("＋ 新建笔记"), this);
    add->setObjectName("addNote");
    remove_ = new QPushButton(QStringLiteral("删除笔记"), this);
    remove_->setObjectName("deleteNote");
    search_ = new QLineEdit(this);
    search_->setPlaceholderText(QStringLiteral("搜索标题和内容"));
    search_->setObjectName("noteSearch");
    bar->addWidget(add);
    bar->addWidget(remove_);
    bar->addWidget(search_, 1);
    layout->addLayout(bar);
    auto *splitter = new QSplitter(this);
    list_ = new QListWidget(splitter);
    list_->setObjectName("noteList");
    list_->setMinimumWidth(160);
    auto *editor = new QWidget(splitter);
    auto *editorLayout = new QVBoxLayout(editor);
    title_ = new QLineEdit(editor);
    title_->setObjectName("noteTitle");
    title_->setPlaceholderText(QStringLiteral("给这个想法起个名字…"));
    title_->setMaxLength(500);
    content_ = new QPlainTextEdit(editor);
    content_->setObjectName("noteContent");
    content_->setPlaceholderText(
        QStringLiteral("有什么想记下来的？\n\n从这里开始写，停下输入后会自动保存。"));
    status_ = new QLabel(editor);
    status_->setProperty("muted", true);
    status_->setWordWrap(true);
    editorLayout->addWidget(title_);
    editorLayout->addWidget(content_, 1);
    editorLayout->addWidget(status_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    layout->addWidget(splitter, 1);
    saveTimer_.setSingleShot(true);
    saveTimer_.setInterval(600);
    connect(&saveTimer_, &QTimer::timeout, this, [this] { flush(); });
    connect(title_, &QLineEdit::textChanged, this, &NotesPage::markDirty);
    connect(content_, &QPlainTextEdit::textChanged, this, &NotesPage::markDirty);
    connect(add, &QPushButton::clicked, this, &NotesPage::quickNote);
    connect(search_, &QLineEdit::textChanged, this,
            [this]
            {
                if (flush())
                    refreshList();
            });
    connect(&service_, &PlannerService::notesChanged, this, &NotesPage::refreshList);
    connect(list_, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *item)
            {
                const QString next = item ? item->data(Qt::UserRole).toString() : QString();
                if (next == activeId_)
                    return;
                if (!flush())
                {
                    refreshList();
                    return;
                }
                load(next);
            });
    connect(remove_, &QPushButton::clicked, this,
            [this]
            {
                if (activeId_.isEmpty())
                    return;
                if (QMessageBox::question(this, QStringLiteral("删除笔记"),
                                          QStringLiteral("删除当前笔记？此操作不可撤销。"),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No) != QMessageBox::Yes)
                    return;
                if (!flush())
                    return;
                if (!service_.deleteNote(activeId_))
                {
                    status_->setText(QStringLiteral("删除失败：") + service_.lastError());
                    return;
                }
                load({});
                refreshList();
            });
    const auto existing = service_.notes();
    load(existing.isEmpty() ? QString() : existing.first().id);
    refreshList();
}
void NotesPage::markDirty()
{
    if (loading_ || activeId_.isEmpty())
        return;
    dirty_ = true;
    status_->setText(QStringLiteral("正在记录，稍后自动保存…"));
    saveTimer_.start();
}
bool NotesPage::flush()
{
    saveTimer_.stop();
    if (!dirty_)
        return true;
    auto n = service_.note(activeId_);
    if (!n)
    {
        status_->setText(QStringLiteral("保存失败：笔记已不存在。"));
        return false;
    }
    n->title = title_->text();
    n->content = content_->toPlainText();
    if (!service_.saveNote(*n))
    {
        status_->setText(QStringLiteral("保存失败，内容仍留在编辑器：") + service_.lastError());
        return false;
    }
    dirty_ = false;
    status_->setText(QStringLiteral("已保存 · ") +
                     QDateTime::currentDateTime().toString("HH:mm:ss"));
    return true;
}
void NotesPage::load(const QString &id)
{
    loading_ = true;
    activeId_ = id;
    auto n = service_.note(id);
    title_->setText(n ? n->title : QString());
    content_->setPlainText(n ? n->content : QString());
    title_->setEnabled(n.has_value());
    content_->setEnabled(n.has_value());
    remove_->setEnabled(n.has_value());
    dirty_ = false;
    status_->setText(n ? QStringLiteral("创建 %1 · 修改 %2")
                             .arg(n->createdAt.toLocalTime().toString("yyyy-MM-dd HH:mm"),
                                  n->updatedAt.toLocalTime().toString("yyyy-MM-dd HH:mm"))
                       : QStringLiteral("还没有选中的笔记。点“新建笔记”，留下第一个想法吧。"));
    loading_ = false;
}
void NotesPage::refreshList()
{
    QSignalBlocker blocker(list_);
    list_->clear();
    for (const auto &n : service_.notes(search_->text()))
    {
        auto *item = new QListWidgetItem(n.title + QStringLiteral("\n") +
                                             n.updatedAt.toLocalTime().toString("MM-dd HH:mm"),
                                         list_);
        item->setData(Qt::UserRole, n.id);
        item->setToolTip(n.title);
        if (n.id == activeId_)
            list_->setCurrentItem(item);
    }
}
void NotesPage::quickNote()
{
    if (!flush())
        return;
    search_->clear();
    const auto id = service_.createNote();
    if (id.isEmpty())
    {
        status_->setText(QStringLiteral("新建失败：") + service_.lastError());
        return;
    }
    load(id);
    refreshList();
    content_->setFocus();
}
