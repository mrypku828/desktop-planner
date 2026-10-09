#pragma once
#include <QSettings>

class AppSettings
{
  public:
    explicit AppSettings(const QString &path);
    bool widgetEnabled() const;
    bool widgetOnTop() const;
    bool remindersEnabled() const;
    bool set(const QString &key, const QVariant &value);
    QVariant get(const QString &key, const QVariant &fallback = {}) const;
    static bool startupEnabled();
    static bool setStartupEnabled(bool enabled, QString &error);

  private:
    QSettings settings_;
};
