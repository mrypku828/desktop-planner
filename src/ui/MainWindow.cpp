#include "MainWindow.h"
#include "CalendarPage.h"
#include "DesktopWidget.h"
#include "NotesPage.h"
#include "Theme.h"
#include "TodoPage.h"
#include "services/ReminderService.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSlider>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(PlannerService &s, AppSettings &settings, const QString &directory,
                       QWidget *parent)
    : QMainWindow(parent), service_(s), settings_(settings)
{
    setWindowTitle(QStringLiteral("桌面计划 · Desktop Planner"));
    applyAppStyle(*qApp, settings_.get("appearance/theme", "mist").toString());
    setWindowOpacity(qBound(80, settings_.get("appearance/opacity", 94).toInt(), 100) / 100.0);
    resize(1080, 760);
    setMinimumSize(880, 640);
    setWindowIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    restoreGeometry(settings_.get("main/geometry").toByteArray());
    bool onScreen = false;
    for (auto *screen : QGuiApplication::screens())
        if (screen->availableGeometry().contains(frameGeometry().topLeft() + QPoint(40, 20)))
            onScreen = true;
    if (!onScreen)
        move(QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(50, 50));
    auto *central = new QWidget(this);
    central->setObjectName("appRoot");
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(20);
    auto *sidebar = new QWidget(central);
    sidebar->setFixedWidth(140);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 4, 0, 0);
    auto *brand = new QLabel(QStringLiteral("桌面计划"), sidebar);
    brand->setObjectName("brand");
    QFont brandFont = brand->font();
    brandFont.setPointSize(15);
    brandFont.setWeight(QFont::Medium);
    brand->setFont(brandFont);
    sidebarLayout->addWidget(brand);
    auto *tagline = new QLabel(QStringLiteral("把每一天，轻轻安排好"), sidebar);
    tagline->setProperty("muted", true);
    tagline->setWordWrap(true);
    sidebarLayout->addWidget(tagline);
    sidebarLayout->addSpacing(18);
    auto *navigation = new QListWidget(central);
    navigation->setObjectName("navigation");
    navigation->addItems({QStringLiteral("计划日历"), QStringLiteral("所有事项"),
                          QStringLiteral("随手记"), QStringLiteral("设置")});
    // Keep the row height stable when a theme changes the application font.
    for (int row = 0; row < navigation->count(); ++row)
        navigation->item(row)->setSizeHint(QSize(120, 48));
    sidebarLayout->addWidget(navigation, 1);
    pages_ = new QStackedWidget(central);
    auto *today = new CalendarPage(service_, pages_);
    auto *all = new TodoPage(service_, false, pages_);
    notes_ = new NotesPage(service_, pages_);
    pages_->addWidget(today);
    pages_->addWidget(all);
    pages_->addWidget(notes_);
    widget_ = new DesktopWidget(service_, settings_);
    pages_->addWidget(createSettingsPage(directory));
    layout->addWidget(sidebar);
    layout->addWidget(pages_, 1);
    setCentralWidget(central);
    connect(navigation, &QListWidget::currentRowChanged, this,
            [this, navigation](int row)
            {
                if (pages_->currentIndex() == 2 && !notes_->flush())
                {
                    QSignalBlocker blocker(navigation);
                    navigation->setCurrentRow(2);
                    return;
                }
                pages_->setCurrentIndex(row);
            });
    navigation->setCurrentRow(0);
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("文件"));
    fileMenu->addAction(QStringLiteral("新建随手记"), QKeySequence("Ctrl+N"), this,
                        [this, navigation]
                        {
                            navigation->setCurrentRow(2);
                            notes_->quickNote();
                        });
    fileMenu->addAction(QStringLiteral("保存笔记"), QKeySequence::Save, this,
                        [this] { notes_->flush(); });
    fileMenu->addAction(QStringLiteral("退出程序"), QKeySequence("Ctrl+Q"), this,
                        &MainWindow::quitApplication);
    tray_ = new QSystemTrayIcon(windowIcon(), this);
    tray_->setToolTip(QStringLiteral("桌面计划：运行期间提供提醒"));
    auto *trayMenu = new QMenu(this);
    trayMenu->addAction(QStringLiteral("打开主窗口"), this, &MainWindow::showManager);
    trayMenu->addAction(QStringLiteral("显示/隐藏小组件"), this,
                        [this] { widgetEnabled_->setChecked(!settings_.widgetEnabled()); });
    trayMenu->addAction(QStringLiteral("快速随手记"), this,
                        [this, navigation]
                        {
                            showManager();
                            navigation->setCurrentRow(2);
                            notes_->quickNote();
                        });
    trayMenu->addSeparator();
    trayMenu->addAction(QStringLiteral("退出程序"), this, &MainWindow::quitApplication);
    tray_->setContextMenu(trayMenu);
    if (QSystemTrayIcon::isSystemTrayAvailable())
        tray_->show();
    connect(tray_, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason)
            {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                    showManager();
            });
    connect(tray_, &QSystemTrayIcon::messageClicked, this, &MainWindow::showManager);
    connect(widget_, &DesktopWidget::openManager, this, &MainWindow::showManager);
    connect(widget_, &DesktopWidget::quickNoteRequested, this,
            [this, navigation]
            {
                showManager();
                navigation->setCurrentRow(2);
                notes_->quickNote();
            });
    connect(widget_, &DesktopWidget::hiddenByUser, this,
            [this] { widgetEnabled_->setChecked(false); });
    auto *reminders = new ReminderService(service_, settings_, *tray_, this);
    connect(reminders, &ReminderService::delivered, this, [this](const QString &title)
            { statusBar()->showMessage(QStringLiteral("已提交通知：") + title, 15000); });
    connect(reminders, &ReminderService::errorOccurred, this, [this](const QString &error)
            { statusBar()->showMessage(QStringLiteral("提醒保存失败：") + error); });
    auto *refreshTimer = new QTimer(this);
    refreshTimer->setInterval(30000);
    connect(refreshTimer, &QTimer::timeout, this,
            [this, today, all]
            {
                today->refresh();
                all->refresh();
                widget_->refresh();
            });
    refreshTimer->start();
    auto *geometryTimer = new QTimer(this);
    geometryTimer->setInterval(5000);
    connect(geometryTimer, &QTimer::timeout, widget_, &DesktopWidget::savePosition);
    geometryTimer->start();
    widget_->applySettings();
    statusBar()->showMessage(
        QSystemTrayIcon::isSystemTrayAvailable()
            ? QStringLiteral("安排好今天，也给自己留一点空闲 · 关闭窗口后，可从右下角托盘重新打开")
            : QStringLiteral("系统托盘不可用：关闭窗口将退出，系统提醒不可用。"));
}
MainWindow::~MainWindow()
{
    delete widget_;
}
QWidget *MainWindow::createSettingsPage(const QString &directory)
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    auto *heading = new QLabel(QStringLiteral("设置"), page);
    heading->setProperty("heading", true);
    layout->addWidget(heading);
    auto *appearance = new QLabel(QStringLiteral("柔和主题"), page);
    layout->addWidget(appearance);
    auto *themes = new QComboBox(page);
    themes->setObjectName("themeSelector");
    const QString selectedTheme =
        themeForId(settings_.get("appearance/theme", "mist").toString()).id;
    for (const auto &theme : themePresets())
    {
        QPixmap swatch(20, 20);
        swatch.fill(theme.accent);
        themes->addItem(QIcon(swatch), theme.name, theme.id);
        if (theme.id == selectedTheme)
            themes->setCurrentIndex(themes->count() - 1);
    }
    layout->addWidget(themes);
    auto *opacityBar = new QHBoxLayout;
    auto *opacityLabel = new QLabel(QStringLiteral("窗口不透明度"), page);
    auto *opacity = new QSlider(Qt::Horizontal, page);
    opacity->setObjectName("windowOpacity");
    opacity->setRange(80, 100);
    opacity->setValue(qBound(80, settings_.get("appearance/opacity", 94).toInt(), 100));
    auto *opacityValue = new QLabel(QString::number(opacity->value()) + "%", page);
    opacityValue->setMinimumWidth(45);
    opacityBar->addWidget(opacityLabel);
    opacityBar->addWidget(opacity, 1);
    opacityBar->addWidget(opacityValue);
    layout->addLayout(opacityBar);
    auto *appearanceHint = new QLabel(
        QStringLiteral("选一款喜欢的颜色，设置会自动记住。调高不透明度，文字会更清晰。"), page);
    appearanceHint->setProperty("muted", true);
    appearanceHint->setWordWrap(true);
    layout->addWidget(appearanceHint);
    connect(themes, &QComboBox::currentIndexChanged, page,
            [this, themes](int index)
            {
                const auto previous =
                    themeForId(settings_.get("appearance/theme", "mist").toString()).id;
                const auto id = themes->itemData(index).toString();
                if (!settings_.set("appearance/theme", id))
                {
                    QSignalBlocker blocker(themes);
                    themes->setCurrentIndex(themes->findData(previous));
                    QMessageBox::warning(this, QStringLiteral("设置保存失败"),
                                         QStringLiteral("主题选择未保存。"));
                    return;
                }
                applyAppStyle(*qApp, id);
            });
    connect(opacity, &QSlider::valueChanged, page,
            [this, opacity, opacityValue](int value)
            {
                const int previous = settings_.get("appearance/opacity", 94).toInt();
                if (!settings_.set("appearance/opacity", value))
                {
                    QSignalBlocker blocker(opacity);
                    opacity->setValue(previous);
                    return;
                }
                opacityValue->setText(QString::number(value) + "%");
                setWindowOpacity(value / 100.0);
                widget_->setWindowOpacity(value / 100.0);
            });
    layout->addSpacing(14);
    widgetEnabled_ = new QCheckBox(QStringLiteral("显示桌面悬浮窗口"), page);
    widgetEnabled_->setChecked(settings_.widgetEnabled());
    layout->addWidget(widgetEnabled_);
    auto *onTop = new QCheckBox(QStringLiteral("悬浮窗口置顶"), page);
    onTop->setChecked(settings_.widgetOnTop());
    layout->addWidget(onTop);
    auto *remind = new QCheckBox(QStringLiteral("启用任务提醒"), page);
    remind->setChecked(settings_.remindersEnabled());
    layout->addWidget(remind);
    auto *startup = new QCheckBox(QStringLiteral("登录 Windows 时启动（仅当前用户）"), page);
    startup->setChecked(AppSettings::startupEnabled());
    layout->addWidget(startup);
#ifndef Q_OS_WIN
    startup->setEnabled(false);
#endif
    auto change = [this](QCheckBox *box, const QString &key, bool value)
    {
        if (!settings_.set(key, value))
        {
            QSignalBlocker blocker(box);
            box->setChecked(!value);
            QMessageBox::warning(this, QStringLiteral("设置保存失败"),
                                 QStringLiteral("无法写入设置文件，请检查用户数据目录权限。"));
            return;
        }
        widget_->applySettings();
    };
    connect(widgetEnabled_, &QCheckBox::toggled, page,
            [=](bool value) { change(widgetEnabled_, "widget/enabled", value); });
    connect(onTop, &QCheckBox::toggled, page,
            [=](bool value) { change(onTop, "widget/onTop", value); });
    connect(remind, &QCheckBox::toggled, page,
            [=](bool value) { change(remind, "reminders/enabled", value); });
    connect(startup, &QCheckBox::toggled, page,
            [this, startup](bool enabled)
            {
                QString error;
                if (!AppSettings::setStartupEnabled(enabled, error))
                {
                    QSignalBlocker blocker(startup);
                    startup->setChecked(!enabled);
                    QMessageBox::warning(this, QStringLiteral("开机启动设置失败"), error);
                }
            });
    auto *hint = new QLabel(
        QStringLiteral(
            "提醒只在程序运行（包括驻留托盘）时工作。完全退出后不会提醒；下次启动或休眠恢复后补检未"
            "触发提醒。\n关闭全局提醒不会删除计划，重新启用后会补提醒。Windows "
            "勿扰模式和通知设置可能阻止展示。\n开机启动需由你勾选才写入当前用户注册表；移动程序位置"
            "后需关闭再重新开启。"),
        page);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto *path = new QLabel(QStringLiteral("数据目录：\n") + directory, page);
    path->setWordWrap(true);
    path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(path);
    auto *open = new QPushButton(QStringLiteral("打开数据目录"), page);
    layout->addWidget(open, 0, Qt::AlignLeft);
    connect(open, &QPushButton::clicked, page,
            [directory] { QDesktopServices::openUrl(QUrl::fromLocalFile(directory)); });
    auto *exit = new QPushButton(QStringLiteral("保存并退出程序"), page);
    layout->addWidget(exit, 0, Qt::AlignLeft);
    connect(exit, &QPushButton::clicked, this, &MainWindow::quitApplication);
    layout->addStretch();
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(page);
    return scroll;
}
void MainWindow::showManager()
{
    showNormal();
    raise();
    activateWindow();
}
bool MainWindow::persistWindowState()
{
    if (!notes_->flush())
    {
        showManager();
        QMessageBox::warning(
            this, QStringLiteral("暂不能退出"),
            QStringLiteral("笔记保存失败，请检查随手记页面的错误提示。编辑内容仍保留。"));
        return false;
    }
    widget_->savePosition();
    if (!settings_.set("main/geometry", saveGeometry()))
    {
        QMessageBox::warning(this, QStringLiteral("设置保存失败"),
                             QStringLiteral("窗口位置保存失败。任务和笔记已单独保存。"));
    }
    return true;
}
void MainWindow::quitApplication()
{
    if (!persistWindowState())
        return;
    exiting_ = true;
    widget_->hide();
    tray_->hide();
    qApp->quit();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (exiting_)
    {
        event->accept();
        return;
    }
    if (!persistWindowState())
    {
        event->ignore();
        return;
    }
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        event->ignore();
        hide();
    }
    else
    {
        event->accept();
        widget_->hide();
        qApp->quit();
    }
}
