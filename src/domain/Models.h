#pragma once
#include <QDateTime>
#include <QString>

struct Todo
{
    QString id;
    QString title;
    QString description = QStringLiteral("");
    QDateTime createdAt;
    QDateTime dueAt; // 无效 QDateTime 表示没有截止时间。
    bool completed = false;
    QDateTime completedAt;
    QDateTime updatedAt;
    bool reminderEnabled = false;
    QDateTime reminderAt;
    QDateTime remindedAt;
    QDate plannedDate;       // 日历安排日期；与截止时间独立。
    bool isDeadline = false; // 独立的截止事项，必须设置 dueAt。
};

struct Note
{
    QString id;
    QString title;
    QString content = QStringLiteral("");
    QDateTime createdAt;
    QDateTime updatedAt;
};
