#include "Database.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace
{
QVariant timestamp(const QDateTime &value)
{
    return value.isValid() ? QVariant(value.toUTC().toString(Qt::ISODateWithMs))
                           : QVariant(QMetaType::fromType<QString>());
}
QDateTime date(const QVariant &value)
{
    return QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
}
} // namespace

Database::Database() : connectionName_(QUuid::createUuid().toString()) {}
Database::~Database()
{
    db_.close();
    db_ = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName_);
}

bool Database::open(const QString &path, QString &error)
{
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db_.setDatabaseName(path);
    db_.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=3000"));
    if (!db_.open())
    {
        error = db_.lastError().text();
        return false;
    }
    QSqlQuery q(db_);
    if (!q.exec(QStringLiteral("PRAGMA user_version")) || !q.next())
    {
        error = q.lastError().text();
        return false;
    }
    const int version = q.value(0).toInt();
    q.finish();
    if (version > 2)
    {
        error = QStringLiteral("数据库来自较新版本，请使用新版程序打开。");
        return false;
    }
    if (version == 2)
        return true;
    if (!db_.transaction())
    {
        error = db_.lastError().text();
        return false;
    }
    QStringList statements;
    if (version == 0)
        statements = {
            QStringLiteral(
                "CREATE TABLE todos (id TEXT PRIMARY KEY, title TEXT NOT NULL, description TEXT "
                "NOT "
                "NULL, "
                "created_at TEXT NOT NULL, due_at TEXT, completed INTEGER NOT NULL, completed_at "
                "TEXT, "
                "updated_at TEXT NOT NULL, reminder_enabled INTEGER NOT NULL, reminder_at TEXT, "
                "reminded_at TEXT)"),
            QStringLiteral("CREATE TABLE notes (id TEXT PRIMARY KEY, title TEXT NOT NULL, content "
                           "TEXT NOT NULL, "
                           "created_at TEXT NOT NULL, updated_at TEXT NOT NULL)"),
            QStringLiteral("CREATE INDEX todos_due ON todos(due_at)"),
            QStringLiteral("CREATE INDEX notes_updated ON notes(updated_at)"),
            QStringLiteral("PRAGMA user_version=1")};
    // v1 -> v2 原地迁移，仅加列，不重建或删除用户数据。
    statements.append(
        {QStringLiteral("ALTER TABLE todos ADD COLUMN planned_date TEXT"),
         QStringLiteral("ALTER TABLE todos ADD COLUMN is_deadline INTEGER NOT NULL DEFAULT 0"),
         QStringLiteral("PRAGMA user_version=2")});
    for (const auto &sql : statements)
    {
        if (!q.exec(sql))
        {
            error = q.lastError().text();
            db_.rollback();
            return false;
        }
    }
    if (!db_.commit())
    {
        error = db_.lastError().text();
        db_.rollback();
        return false;
    }
    return true;
}

bool Database::loadTodos(QVector<Todo> &result, QString &error) const
{
    QSqlQuery q(db_);
    if (!q.exec(
            QStringLiteral("SELECT id,title,description,created_at,due_at,completed,completed_at,"
                           "updated_at,reminder_enabled,reminder_at,reminded_at,planned_date,is_"
                           "deadline FROM todos")))
    {
        error = q.lastError().text();
        return false;
    }
    result.clear();
    while (q.next())
    {
        Todo t;
        t.id = q.value(0).toString();
        t.title = q.value(1).toString();
        t.description = q.value(2).toString();
        t.createdAt = date(q.value(3));
        t.dueAt = date(q.value(4));
        t.completed = q.value(5).toBool();
        t.completedAt = date(q.value(6));
        t.updatedAt = date(q.value(7));
        t.reminderEnabled = q.value(8).toBool();
        t.reminderAt = date(q.value(9));
        t.remindedAt = date(q.value(10));
        t.plannedDate = QDate::fromString(q.value(11).toString(), Qt::ISODate);
        t.isDeadline = q.value(12).toBool();
        result.push_back(t);
    }
    return true;
}

bool Database::saveTodo(const Todo &t, QString &error)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "INSERT INTO todos "
        "(id,title,description,created_at,due_at,completed,completed_at,updated_at,"
        "reminder_enabled,reminder_at,reminded_at,planned_date,is_deadline) VALUES "
        "(?,?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET "
        "title=excluded.title,description=excluded.description,due_at=excluded.due_at,completed="
        "excluded.completed,"
        "completed_at=excluded.completed_at,updated_at=excluded.updated_at,reminder_enabled="
        "excluded.reminder_enabled,"
        "reminder_at=excluded.reminder_at,reminded_at=excluded.reminded_at,planned_date=excluded."
        "planned_date,is_deadline=excluded.is_deadline"));
    q.addBindValue(t.id);
    q.addBindValue(t.title);
    q.addBindValue(t.description);
    q.addBindValue(timestamp(t.createdAt));
    q.addBindValue(timestamp(t.dueAt));
    q.addBindValue(t.completed);
    q.addBindValue(timestamp(t.completedAt));
    q.addBindValue(timestamp(t.updatedAt));
    q.addBindValue(t.reminderEnabled);
    q.addBindValue(timestamp(t.reminderAt));
    q.addBindValue(timestamp(t.remindedAt));
    q.addBindValue(t.plannedDate.isValid() ? QVariant(t.plannedDate.toString(Qt::ISODate))
                                           : QVariant(QMetaType::fromType<QString>()));
    q.addBindValue(t.isDeadline);
    if (!q.exec())
    {
        error = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::loadNotes(QVector<Note> &result, QString &error) const
{
    QSqlQuery q(db_);
    if (!q.exec(QStringLiteral("SELECT id,title,content,created_at,updated_at FROM notes")))
    {
        error = q.lastError().text();
        return false;
    }
    result.clear();
    while (q.next())
        result.push_back({q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                          date(q.value(3)), date(q.value(4))});
    return true;
}

bool Database::saveNote(const Note &n, QString &error)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "INSERT INTO notes VALUES (?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET "
        "title=excluded.title,content=excluded.content,updated_at=excluded.updated_at"));
    q.addBindValue(n.id);
    q.addBindValue(n.title);
    q.addBindValue(n.content);
    q.addBindValue(timestamp(n.createdAt));
    q.addBindValue(timestamp(n.updatedAt));
    if (!q.exec())
    {
        error = q.lastError().text();
        return false;
    }
    return true;
}
bool Database::remove(const QString &table, const QString &id, QString &error)
{
    QSqlQuery q(db_);
    // table 只来自下面两个内部调用，用户输入全部作为参数绑定。
    q.prepare(QStringLiteral("DELETE FROM ") + table + QStringLiteral(" WHERE id=?"));
    q.addBindValue(id);
    if (!q.exec())
    {
        error = q.lastError().text();
        return false;
    }
    return true;
}
bool Database::removeTodo(const QString &id, QString &error)
{
    return remove(QStringLiteral("todos"), id, error);
}
bool Database::removeNote(const QString &id, QString &error)
{
    return remove(QStringLiteral("notes"), id, error);
}
