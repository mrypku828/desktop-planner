#include "PlannerService.h"
#include <QUuid>
#include <algorithm>

PlannerService::PlannerService(Database &db, QObject *parent) : QObject(parent), db_(db) {}
bool PlannerService::initialize(QString &error)
{
    return db_.loadTodos(todos_, error) && db_.loadNotes(notes_, error);
}
QVector<Todo> PlannerService::todos(TodoFilter filter, QDate today) const
{
    QVector<Todo> result;
    for (const auto &t : todos_)
    {
        if (filter == TodoFilter::Unfinished && t.completed)
            continue;
        if (filter == TodoFilter::Completed && !t.completed)
            continue;
        // 安排在今日或今日截止都属于今日事项，避免遗漏截止日期。
        if (filter == TodoFilter::Today && t.plannedDate != today &&
            (!t.dueAt.isValid() || t.dueAt.toLocalTime().date() != today))
            continue;
        result.push_back(t);
    }
    std::sort(result.begin(), result.end(),
              [](const Todo &a, const Todo &b)
              {
                  if (a.dueAt.isValid() != b.dueAt.isValid())
                      return a.dueAt.isValid();
                  if (a.dueAt != b.dueAt)
                      return a.dueAt < b.dueAt;
                  if (a.createdAt != b.createdAt)
                      return a.createdAt < b.createdAt;
                  return a.id < b.id;
              });
    return result;
}
std::optional<Todo> PlannerService::todo(const QString &id) const
{
    for (const auto &t : todos_)
        if (t.id == id)
            return t;
    return std::nullopt;
}
QVector<Todo> PlannerService::todosForDate(QDate date) const
{
    if (!date.isValid())
        return {};
    return todos(TodoFilter::Today, date);
}
bool PlannerService::saveTodo(Todo value)
{
    error_.clear();
    value.title = value.title.trimmed();
    if (value.title.isEmpty())
    {
        error_ = QStringLiteral("任务标题不能为空。");
        return false;
    }
    if (value.isDeadline && !value.dueAt.isValid())
    {
        error_ = QStringLiteral("Deadline 必须设置截止日期和时间。");
        return false;
    }
    if (value.isDeadline)
        value.plannedDate = {};
    if (value.reminderEnabled && !value.reminderAt.isValid())
    {
        error_ = QStringLiteral("启用提醒时必须设置有效的提醒时间。");
        return false;
    }
    const auto now = QDateTime::currentDateTimeUtc();
    auto old = todo(value.id);
    if (!value.id.isEmpty() && !old)
    {
        error_ = QStringLiteral("任务已不存在。");
        return false;
    }
    if (!old)
    {
        value.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        value.createdAt = now;
        value.completedAt = value.completed ? now : QDateTime();
        value.remindedAt = {};
    }
    else
    {
        value.createdAt = old->createdAt;
        value.completedAt =
            value.completed ? (old->completed ? old->completedAt : now) : QDateTime();
        value.remindedAt = old->remindedAt;
        // 改标题/截止时间不重复提醒；重新启用或改变提醒时间才重置。
        if (value.reminderEnabled && (!old->reminderEnabled || value.reminderAt != old->reminderAt))
            value.remindedAt = {};
    }
    if (value.completed)
        value.reminderEnabled = false; // 完成后取消，恢复未完成不自动重新启用。
    value.updatedAt = now;
    if (!db_.saveTodo(value, error_))
        return false;
    bool replaced = false;
    for (auto &t : todos_)
        if (t.id == value.id)
        {
            t = value;
            replaced = true;
            break;
        }
    if (!replaced)
        todos_.push_back(value);
    emit todosChanged();
    return true;
}
bool PlannerService::setCompleted(const QString &id, bool completed)
{
    auto value = todo(id);
    if (!value)
    {
        error_ = QStringLiteral("任务已不存在。");
        return false;
    }
    value->completed = completed;
    return saveTodo(*value);
}
bool PlannerService::deleteTodo(const QString &id)
{
    if (!db_.removeTodo(id, error_))
        return false;
    todos_.erase(
        std::remove_if(todos_.begin(), todos_.end(), [&](const Todo &t) { return t.id == id; }),
        todos_.end());
    emit todosChanged();
    return true;
}
QVector<Note> PlannerService::notes(const QString &search) const
{
    QVector<Note> result;
    for (const auto &n : notes_)
        if (n.title.contains(search, Qt::CaseInsensitive) ||
            n.content.contains(search, Qt::CaseInsensitive))
            result.push_back(n);
    std::sort(result.begin(), result.end(), [](const Note &a, const Note &b)
              { return a.updatedAt == b.updatedAt ? a.id < b.id : a.updatedAt > b.updatedAt; });
    return result;
}
std::optional<Note> PlannerService::note(const QString &id) const
{
    for (const auto &n : notes_)
        if (n.id == id)
            return n;
    return std::nullopt;
}
QString PlannerService::createNote()
{
    Note n;
    n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    n.title = QStringLiteral("未命名笔记");
    n.createdAt = QDateTime::currentDateTimeUtc();
    n.updatedAt = n.createdAt;
    if (!db_.saveNote(n, error_))
        return {};
    notes_.push_back(n);
    emit notesChanged();
    return n.id;
}
bool PlannerService::saveNote(Note value)
{
    auto old = note(value.id);
    if (!old)
    {
        error_ = QStringLiteral("笔记已不存在。");
        return false;
    }
    value.title = value.title.trimmed();
    if (value.title.isEmpty())
        value.title = QStringLiteral("未命名笔记");
    value.createdAt = old->createdAt;
    value.updatedAt = QDateTime::currentDateTimeUtc();
    if (!db_.saveNote(value, error_))
        return false;
    for (auto &n : notes_)
        if (n.id == value.id)
        {
            n = value;
            break;
        }
    emit notesChanged();
    return true;
}
bool PlannerService::deleteNote(const QString &id)
{
    if (!db_.removeNote(id, error_))
        return false;
    notes_.erase(
        std::remove_if(notes_.begin(), notes_.end(), [&](const Note &n) { return n.id == id; }),
        notes_.end());
    emit notesChanged();
    return true;
}
QVector<Todo> PlannerService::claimDueReminders(QDateTime now)
{
    error_.clear();
    QVector<Todo> result;
    for (auto &t : todos_)
    {
        if (t.completed || !t.reminderEnabled || t.remindedAt.isValid() ||
            !t.reminderAt.isValid() || t.reminderAt > now)
            continue;
        Todo updated = t;
        updated.remindedAt = now.toUTC();
        updated.updatedAt = now.toUTC();
        if (!db_.saveTodo(updated, error_))
            break;
        t = updated;
        result.push_back(t);
        break; // 每次只领取一条，其他提醒仍留在数据库。
    }
    if (!result.isEmpty())
        emit todosChanged();
    return result;
}
