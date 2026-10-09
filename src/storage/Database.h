#pragma once
#include "domain/Models.h"
#include <QSqlDatabase>
#include <QVector>

// 仅负责 SQL 与连接生命周期。错误由 UI/服务决定如何显示。
class Database
{
  public:
    Database();
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    bool open(const QString &path, QString &error);
    bool loadTodos(QVector<Todo> &result, QString &error) const;
    bool saveTodo(const Todo &todo, QString &error);
    bool removeTodo(const QString &id, QString &error);
    bool loadNotes(QVector<Note> &result, QString &error) const;
    bool saveNote(const Note &note, QString &error);
    bool removeNote(const QString &id, QString &error);

  private:
    bool remove(const QString &table, const QString &id, QString &error);
    QString connectionName_;
    QSqlDatabase db_;
};
