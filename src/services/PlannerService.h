#pragma once
#include "storage/Database.h"
#include <QObject>
#include <optional>

enum class TodoFilter
{
    All,
    Unfinished,
    Completed,
    Today
};

class PlannerService : public QObject
{
    Q_OBJECT
  public:
    explicit PlannerService(Database &database, QObject *parent = nullptr);
    bool initialize(QString &error);
    QVector<Todo> todos(TodoFilter filter = TodoFilter::All,
                        QDate today = QDate::currentDate()) const;
    std::optional<Todo> todo(const QString &id) const;
    QVector<Todo> todosForDate(QDate date) const;
    bool saveTodo(Todo value);
    bool setCompleted(const QString &id, bool completed);
    bool deleteTodo(const QString &id);
    QVector<Note> notes(const QString &search = {}) const;
    std::optional<Note> note(const QString &id) const;
    QString createNote();
    bool saveNote(Note value);
    bool deleteNote(const QString &id);
    QVector<Todo> claimDueReminders(QDateTime now); // 保存触发状态成功才交给通知层。
    QString lastError() const
    {
        return error_;
    }
  signals:
    void todosChanged();
    void notesChanged();

  private:
    Database &db_;
    QVector<Todo> todos_;
    QVector<Note> notes_;
    QString error_;
};
