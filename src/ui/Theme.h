#pragma once
#include <QColor>
#include <QString>
#include <QVector>
class QApplication;

struct Theme
{
    QString id;
    QString name;
    QColor background;
    QColor accent;
    QColor soft;
};
const QVector<Theme> &themePresets();
Theme themeForId(const QString &id);
void applyAppStyle(QApplication &app, const QString &themeId = QStringLiteral("mist"));
