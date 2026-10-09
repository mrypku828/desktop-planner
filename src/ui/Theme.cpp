#include "Theme.h"
#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QStyle>

const QVector<Theme> &themePresets()
{
    static const QVector<Theme> themes = {{"mist", QStringLiteral("雾蓝 · Mist"), QColor("#edf3f8"),
                                           QColor("#577f9b"), QColor("#d9e7f1")},
                                          {"sage", QStringLiteral("鼠尾草 · Sage"),
                                           QColor("#eff5f0"), QColor("#547e69"), QColor("#dcebdd")},
                                          {"lavender", QStringLiteral("薰衣草 · Lavender"),
                                           QColor("#f3f0f8"), QColor("#817099"), QColor("#e6def2")},
                                          {"rose", QStringLiteral("玫瑰粉 · Rose"),
                                           QColor("#faf0f3"), QColor("#a16a7e"), QColor("#f1dfe6")},
                                          {"apricot", QStringLiteral("杏仁 · Apricot"),
                                           QColor("#faf4ec"), QColor("#96744e"),
                                           QColor("#f0e4d2")}};
    return themes;
}
Theme themeForId(const QString &id)
{
    for (const auto &theme : themePresets())
        if (theme.id == id)
            return theme;
    return themePresets().first();
}
void applyAppStyle(QApplication &app, const QString &id)
{
    static const bool resourcesReady = []
    {
        Q_INIT_RESOURCE(planner);
        return true;
    }();
    Q_UNUSED(resourcesReady)
    const auto theme = themeForId(id);
    app.setProperty("themeId", theme.id);
    if (app.style()->objectName() != QStringLiteral("fusion"))
        app.setStyle("Fusion");
    auto palette = app.style()->standardPalette();
    palette.setColor(QPalette::Window, theme.background);
    palette.setColor(QPalette::WindowText, QColor("#303c4b"));
    palette.setColor(QPalette::Text, QColor("#303c4b"));
    palette.setColor(QPalette::Base, QColor("#fcfdff"));
    palette.setColor(QPalette::Button, theme.soft);
    palette.setColor(QPalette::ButtonText, QColor("#303c4b"));
    palette.setColor(QPalette::Highlight, theme.accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#89939e"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#89939e"));
    app.setPalette(palette);
    // Qt falls back to another installed font if a family is unavailable.
    QFont readingFont;
    readingFont.setFamilies({QStringLiteral("Microsoft YaHei UI"),
                             QStringLiteral("Microsoft YaHei"), QStringLiteral("Segoe UI")});
    readingFont.setPointSizeF(10.5);
    readingFont.setWeight(QFont::Normal);
    readingFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(readingFont);
    app.setStyleSheet(
        QStringLiteral(
            "QWidget { color: #394858; }"
            "QMainWindow, QDialog, QWidget#desktopWidget { background: %1; }"
            "QWidget#appRoot { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 "
            "%2); }"
            "QLabel { background: transparent; } QLabel[heading=true] { font-size: 20pt; "
            "font-weight: 500; }"
            "QLabel#brand { font-size: 14pt; font-weight: 500; } QLabel#calendarMonthLabel { "
            "font-size: 14pt; font-weight: 500; }"
            "QLabel[muted=true] { color: #6d7c8b; font-size: 9.5pt; }"
            "QToolTip { color: #465565; background: #f8fafc; border: 1px solid %2; "
            "padding: 8px; font-size: 10pt; }"
            "QPlainTextEdit#noteContent { font-size: 11pt; padding: 12px; }"
            "QStatusBar { color: #6d7c8b; font-size: 9pt; }"
            "QPushButton, QToolButton { background: rgba(255,255,255,185); border: 1px solid %2; "
            "border-radius: 9px; padding: 7px 12px; }"
            "QPushButton:hover, QToolButton:hover { background: %2; border-color: %3; }"
            "QPushButton:pressed { background: %2; } QPushButton:disabled { color: #89939e; }"
            "QPushButton[primary=true] { background: %3; color: white; border-color: %3; }"
            "QLineEdit, QPlainTextEdit, QComboBox, QDateTimeEdit, QDateEdit { background: "
            "rgba(255,255,255,225); border: 1px solid #d4dde5; border-radius: 8px; padding: 6px; "
            "selection-background-color: %3; }"
            "QLineEdit:focus, QPlainTextEdit:focus { border-color: %3; }"
            "QListWidget { background: rgba(255,255,255,155); border: 1px solid "
            "rgba(255,255,255,210); border-radius: 12px; outline: 0; }"
            "QListWidget::item { padding: 11px 8px; border-radius: 8px; margin: 3px; }"
            "QListWidget::item:selected { background: %2; color: #303c4b; }"
            "QListWidget#navigation { background: rgba(255,255,255,115); border: none; }"
            "QListWidget#navigation::item { padding: 12px 8px; margin: 5px; }"
            "QListWidget#navigation::item:selected { background: %3; color: white; }"
            "QMenuBar, QStatusBar { background: transparent; } QMenu { background: #fcfdff; "
            "border: 1px solid %2; padding: 5px; }"
            "QMenu::item:selected { background: %2; }"
            "QScrollBar:vertical { width: 9px; background: transparent; }"
            "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 22px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QCheckBox { spacing: 8px; padding: 4px 0; }"
            "QCheckBox::indicator, QListWidget::indicator { width: 16px; height: 16px; border: 1px "
            "solid #a9bac9; border-radius: 4px; background: #ffffff; }"
            "QCheckBox::indicator:checked, QListWidget::indicator:checked { background: %3; "
            "border-color: %3; image: url(:/icons/check.svg); }"
            "QCheckBox::indicator:disabled { border-color: #cbd5df; background: #eef1f5; }"
            "QCheckBox::indicator:disabled:checked { background: #a9bac9; image: "
            "url(:/icons/check.svg); }")
            .arg(theme.background.name(), theme.soft.name(), theme.accent.name()));
}
