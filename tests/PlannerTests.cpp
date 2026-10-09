#include "platform/AppSettings.h"
#include "services/PlannerService.h"
#include "ui/AppStyle.h"
#include "ui/CalendarPage.h"
#include "ui/DesktopWidget.h"
#include "ui/MainWindow.h"
#include "ui/NotesPage.h"
#include "ui/Theme.h"
#include "ui/TodoDialog.h"
#include "ui/TodoPage.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class PlannerTests : public QObject
{
    Q_OBJECT
  private slots:
    void initTestCase()
    {
        applyAppStyle(*qApp);
    }
    void todoPersistenceAndValidation();
    void sortFilterAndLocalToday();
    void notesPersistenceSearchDelete();
    void reminderLifecycleAndRestart();
    void storageErrorsAndFutureSchema();
    void settingsPersistence();
    void notesAutosaveAndSwitch();
    void sharedWidgetAndTaskPage();
    void dialogAndMainWindowSmoke();
    void todoUiCreateEditDeleteConfirmation();
    void notesDeleteConfirmationAndFailedSave();
    void migrateVersionOneWithoutDataLoss();
    void calendarDatesAndDeadlinePersistence();
    void calendarClickCreatesSelectedDayTask();
    void deadlineEditorAndThemePersistence();
};

void PlannerTests::todoPersistenceAndValidation()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString error, id;
    {
        Database db;
        QVERIFY2(db.open(dir.filePath("test.sqlite"), error), qPrintable(error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        Todo invalid;
        invalid.title = "  ";
        QVERIFY(!s.saveTodo(invalid));
        QVERIFY(s.todos().isEmpty());
        Todo t;
        t.title = QStringLiteral("学习 C++ ' ; DROP TABLE todos; --");
        t.description = QStringLiteral("中文描述\n第二行 😀");
        QVERIFY2(s.saveTodo(t), qPrintable(s.lastError()));
        QCOMPARE(s.todos().size(), 1);
        auto saved = s.todos().first();
        id = saved.id;
        QVERIFY(!id.isEmpty());
        QVERIFY(!saved.dueAt.isValid());
        QVERIFY(saved.createdAt.isValid());
        QVERIFY(!saved.completedAt.isValid());
        const auto created = saved.createdAt;
        saved.title = QStringLiteral("修改标题");
        QVERIFY(s.saveTodo(saved));
        QCOMPARE(s.todo(id)->createdAt, created);
        QVERIFY(s.setCompleted(id, true));
        QVERIFY(s.todo(id)->completedAt.isValid());
        QVERIFY(s.setCompleted(id, false));
        QVERIFY(!s.todo(id)->completedAt.isValid());
    }
    {
        Database db;
        QVERIFY(db.open(dir.filePath("test.sqlite"), error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        auto t = s.todo(id);
        QVERIFY(t);
        QCOMPARE(t->title, QStringLiteral("修改标题"));
        QCOMPARE(t->description, QStringLiteral("中文描述\n第二行 😀"));
        QVERIFY(!t->dueAt.isValid());
        QVERIFY(s.deleteTodo(id));
        QVERIFY(s.todos().isEmpty());
    }
    Database db;
    QVERIFY(db.open(dir.filePath("test.sqlite"), error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    QVERIFY(s.todos().isEmpty());
}
void PlannerTests::sortFilterAndLocalToday()
{
    Database db;
    QString error;
    QVERIFY(db.open(":memory:", error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    const QDate today = QDate::currentDate();
    Todo t;
    t.title = "no due";
    QVERIFY(s.saveTodo(t));
    t.title = "late";
    t.dueAt = QDateTime(today, QTime(22, 0)).toUTC();
    QVERIFY(s.saveTodo(t));
    t.title = "early";
    t.dueAt = QDateTime(today, QTime(1, 0)).toUTC();
    QVERIFY(s.saveTodo(t));
    t.title = "tomorrow";
    t.dueAt = QDateTime(today.addDays(1), QTime(1, 0)).toUTC();
    t.completed = true;
    QVERIFY(s.saveTodo(t));
    const auto all = s.todos();
    QCOMPARE(all.size(), 4);
    QCOMPARE(all.first().title, QString("early"));
    QCOMPARE(all.last().title, QString("no due"));
    QCOMPARE(s.todos(TodoFilter::Today, today).size(), 2);
    QCOMPARE(s.todos(TodoFilter::Unfinished).size(), 3);
    QCOMPARE(s.todos(TodoFilter::Completed).size(), 1);
}
void PlannerTests::notesPersistenceSearchDelete()
{
    QTemporaryDir dir;
    QString error, id;
    {
        Database db;
        QVERIFY(db.open(dir.filePath("notes.sqlite"), error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        id = s.createNote();
        QVERIFY(!id.isEmpty());
        auto n = *s.note(id);
        n.title = QStringLiteral("想法 ' ?");
        n.content = QStringLiteral("正文中文\nSQLite %_ 😀");
        QVERIFY(s.saveNote(n));
        QCOMPARE(s.notes(QStringLiteral("正文中文")).size(), 1);
        QCOMPARE(s.notes(QStringLiteral("想法")).size(), 1);
        QCOMPARE(s.notes("%_").size(), 1);
        QVERIFY(s.notes("missing").isEmpty());
        QTest::qWait(2);
        const auto newer = s.createNote();
        QCOMPARE(s.notes().first().id, newer);
    }
    Database db;
    QVERIFY(db.open(dir.filePath("notes.sqlite"), error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    QVERIFY(s.note(id));
    QCOMPARE(s.note(id)->content, QStringLiteral("正文中文\nSQLite %_ 😀"));
    QVERIFY(s.deleteNote(id));
    QVERIFY(!s.note(id));
}
void PlannerTests::reminderLifecycleAndRestart()
{
    QTemporaryDir dir;
    QString error, id;
    const auto now = QDateTime::currentDateTimeUtc();
    {
        Database db;
        QVERIFY(db.open(dir.filePath("r.sqlite"), error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        Todo t;
        t.title = "reminder";
        t.reminderEnabled = true;
        t.reminderAt = now.addSecs(30);
        QVERIFY(s.saveTodo(t));
        id = s.todos().first().id;
        QVERIFY(s.claimDueReminders(now).isEmpty());
    }
    {
        Database db;
        QVERIFY(db.open(dir.filePath("r.sqlite"), error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        QCOMPARE(s.claimDueReminders(now.addSecs(30)).size(), 1);
        QVERIFY(s.claimDueReminders(now.addSecs(31)).isEmpty());
        auto t = *s.todo(id);
        t.title = "changed";
        QVERIFY(s.saveTodo(t));
        QVERIFY(s.claimDueReminders(now.addSecs(40)).isEmpty());
        t = *s.todo(id);
        t.reminderAt = now.addSecs(50);
        QVERIFY(s.saveTodo(t));
        QCOMPARE(s.claimDueReminders(now.addSecs(50)).size(), 1);
        QVERIFY(s.setCompleted(id, true));
        QVERIFY(!s.todo(id)->reminderEnabled);
        QVERIFY(s.setCompleted(id, false));
        QVERIFY(!s.todo(id)->reminderEnabled);
        t = *s.todo(id);
        t.reminderEnabled = true;
        t.reminderAt = now;
        QVERIFY(s.saveTodo(t));
        QVERIFY(!s.todo(id)->remindedAt.isValid());
        QCOMPARE(s.claimDueReminders(now).size(), 1);
    }
    {
        Database db;
        QVERIFY(db.open(dir.filePath("r.sqlite"), error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        QVERIFY(s.claimDueReminders(now.addSecs(200)).isEmpty());
        auto t = *s.todo(id);
        t.reminderAt = now.addSecs(300);
        QVERIFY(s.saveTodo(t));
        QVERIFY(s.deleteTodo(id));
        QVERIFY(s.claimDueReminders(now.addSecs(400)).isEmpty());
        Todo disabled;
        disabled.title = "disabled";
        disabled.reminderAt = now;
        QVERIFY(s.saveTodo(disabled));
        QVERIFY(s.claimDueReminders(now).isEmpty());
        Todo a;
        a.title = "a";
        a.reminderAt = now;
        a.reminderEnabled = true;
        QVERIFY(s.saveTodo(a));
        a.title = "b";
        QVERIFY(s.saveTodo(a));
        QCOMPARE(s.claimDueReminders(now).size(), 1);
        QCOMPARE(s.claimDueReminders(now).size(), 1);
        QVERIFY(s.claimDueReminders(now).isEmpty());
    }
}
void PlannerTests::storageErrorsAndFutureSchema()
{
    QTemporaryDir dir;
    QString error;
    Database bad;
    QVERIFY(!bad.open(dir.filePath("missing/test.sqlite"), error));
    QVERIFY(!error.isEmpty());
    const QString connection = "schema_test";
    {
        auto raw = QSqlDatabase::addDatabase("QSQLITE", connection);
        raw.setDatabaseName(dir.filePath("future.sqlite"));
        QVERIFY(raw.open());
        QSqlQuery q(raw);
        QVERIFY(q.exec("PRAGMA user_version=99"));
    }
    QSqlDatabase::removeDatabase(connection);
    Database future;
    QVERIFY(!future.open(dir.filePath("future.sqlite"), error));
    QVERIFY(error.contains(QStringLiteral("较新")));
    Database db;
    const auto path = dir.filePath("broken.sqlite");
    QVERIFY(db.open(path, error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    Todo empty;
    empty.reminderEnabled = true;
    empty.title = "invalid reminder";
    QVERIFY(!s.saveTodo(empty));
    QVERIFY(s.todos().isEmpty());
    // 仅破坏临时测试库，验证 SQL 错误不会先改变内存中的任务列表。
    const QString brokenConnection = "broken_test";
    {
        auto raw = QSqlDatabase::addDatabase("QSQLITE", brokenConnection);
        raw.setDatabaseName(path);
        QVERIFY(raw.open());
        QSqlQuery q(raw);
        QVERIFY(q.exec("DROP TABLE todos"));
    }
    QSqlDatabase::removeDatabase(brokenConnection);
    Todo t;
    t.title = "write failure";
    QVERIFY(!s.saveTodo(t));
    QVERIFY(!s.lastError().isEmpty());
    QVERIFY(s.todos().isEmpty());
}
void PlannerTests::settingsPersistence()
{
    QTemporaryDir dir;
    const auto path = dir.filePath("settings.ini");
    {
        AppSettings settings(path);
        QVERIFY(!settings.widgetEnabled());
        QVERIFY(settings.remindersEnabled());
        QVERIFY(settings.set("widget/enabled", true));
        QVERIFY(settings.set("widget/onTop", true));
        QVERIFY(settings.set("reminders/enabled", false));
        QVERIFY(settings.set("widget/geometry", QByteArray("geometry")));
    }
    AppSettings settings(path);
    QVERIFY(settings.widgetEnabled());
    QVERIFY(settings.widgetOnTop());
    QVERIFY(!settings.remindersEnabled());
    QCOMPARE(settings.get("widget/geometry").toByteArray(), QByteArray("geometry"));
    AppSettings failed(dir.path()); // 把目录当文件，稳定触发写入失败。
    QVERIFY(!failed.set("reminders/enabled", false));
    QVERIFY(failed.remindersEnabled());
}
void PlannerTests::notesAutosaveAndSwitch()
{
    Database db;
    QString error;
    QVERIFY(db.open(":memory:", error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    NotesPage page(s);
    page.show();
    page.quickNote();
    auto *title = page.findChild<QLineEdit *>("noteTitle");
    auto *content = page.findChild<QPlainTextEdit *>("noteContent");
    auto *search = page.findChild<QLineEdit *>("noteSearch");
    auto *list = page.findChild<QListWidget *>("noteList");
    QVERIFY(title && content && search && list);
    title->setText(QStringLiteral("自动保存中文"));
    content->setPlainText(QStringLiteral("快速记录正文"));
    QTRY_COMPARE_WITH_TIMEOUT(s.notes().first().content, QStringLiteral("快速记录正文"), 2000);
    const auto id = s.notes().first().id;
    content->setPlainText(QStringLiteral("切换前必须保存"));
    page.quickNote();
    QCOMPARE(s.note(id)->content, QStringLiteral("切换前必须保存"));
    QCOMPARE(s.notes().size(), 2);
    content->setPlainText("second");
    QVERIFY(page.flush());
    search->setText(QStringLiteral("自动保存"));
    QCOMPARE(list->count(), 1);
    list->setCurrentRow(0);
    QCOMPARE(content->toPlainText(), QStringLiteral("切换前必须保存"));
}
void PlannerTests::sharedWidgetAndTaskPage()
{
    QTemporaryDir dir;
    AppSettings settings(dir.filePath("settings.ini"));
    Database db;
    QString error;
    QVERIFY(db.open(":memory:", error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    DesktopWidget widget(s, settings);
    TodoPage page(s, true);
    widget.show();
    page.show();
    auto *input = widget.findChild<QLineEdit *>("widgetQuickTodo");
    auto *widgetList = widget.findChild<QListWidget *>("widgetTodoList");
    auto *mainList = page.findChild<QListWidget *>("todoList");
    QVERIFY(input && widgetList && mainList);
    input->setText(QStringLiteral("小组件快速添加"));
    QTest::keyClick(input, Qt::Key_Return);
    QCOMPARE(widgetList->count(), 1);
    QCOMPARE(mainList->count(), 1);
    QCOMPARE(s.todos().size(), 1);
    widgetList->item(0)->setCheckState(Qt::Checked);
    QTRY_VERIFY(s.todos().first().completed);
    QCOMPARE(widgetList->count(), 0);
    QCOMPARE(mainList->count(), 0);
    QVERIFY(s.setCompleted(s.todos().first().id, false));
    QCOMPARE(widgetList->count(), 1);
    QCOMPARE(mainList->count(), 1);
    widget.resize(360, 400);
    widget.move(40, 50);
    widget.savePosition();
    QVERIFY(!settings.get("widget/geometry").toByteArray().isEmpty());
}
void PlannerTests::dialogAndMainWindowSmoke()
{
    TodoDialog dialog({});
    auto *title = dialog.findChild<QLineEdit *>("todoTitle");
    QVERIFY(title);
    title->setText(QStringLiteral("对话框输入"));
    auto *buttons = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(buttons);
    QTest::mouseClick(buttons->button(QDialogButtonBox::Save), Qt::LeftButton);
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
    QCOMPARE(dialog.value().title, QStringLiteral("对话框输入"));
    QVERIFY(!dialog.value().dueAt.isValid());
    QTemporaryDir dir;
    AppSettings settings(dir.filePath("settings.ini"));
    Database db;
    QString error;
    QVERIFY(db.open(":memory:", error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    Todo sample;
    sample.title = QStringLiteral("阅读 C++：对象的生命周期与 Qt 信号");
    sample.description = QStringLiteral("只用于布局测试，不进入真实用户数据库。");
    sample.dueAt = QDateTime(QDate::currentDate(), QTime(0, 1)).toUTC();
    QVERIFY(s.saveTodo(sample));
    sample.title = QStringLiteral("整理今天的学习笔记");
    sample.dueAt = QDateTime(QDate::currentDate(), QTime(23, 59)).toUTC();
    QVERIFY(s.saveTodo(sample));
    for (int offset : {1, 3, 5, 8, 12})
    {
        Todo t;
        t.title = offset % 2 ? QStringLiteral("学习与复盘") : QStringLiteral("项目阶段验收");
        t.plannedDate = QDate::currentDate().addDays(offset);
        QVERIFY(s.saveTodo(t));
    }
    Todo deadline;
    deadline.title = QStringLiteral("项目报告提交");
    deadline.isDeadline = true;
    deadline.dueAt = QDateTime(QDate::currentDate().addDays(3), QTime(18, 0)).toUTC();
    QVERIFY(s.saveTodo(deadline));
    const auto noteId = s.createNote();
    auto sampleNote = *s.note(noteId);
    sampleNote.title = QStringLiteral("项目想法与中文记录");
    sampleNote.content = QStringLiteral("先让核心功能稳定，再逐步优化体验。\n\n今天学习：\n1. "
                                        "参数化 SQL\n2. 信号与槽\n3. 自动保存与错误处理");
    QVERIFY(s.saveNote(sampleNote));
    MainWindow window(s, settings, dir.path());
    window.show();
    QTest::qWait(50);
    auto *navigation = window.findChild<QListWidget *>("navigation");
    QVERIFY(navigation);
    QCOMPARE(navigation->count(), 4);
    for (int i = 0; i < 4; ++i)
    {
        navigation->setCurrentRow(i);
        QTest::qWait(5);
    }
    const auto artifact = qEnvironmentVariable("PLANNER_TEST_ARTIFACT_DIR");
    if (!artifact.isEmpty())
    {
        navigation->setCurrentRow(0);
        QVERIFY(window.grab().save(artifact + "/main-window.png"));
        navigation->setCurrentRow(3);
        QVERIFY(window.grab().save(artifact + "/settings.png"));
        navigation->setCurrentRow(2);
        QVERIFY(window.grab().save(artifact + "/notes.png"));
        window.resize(880, 640);
        navigation->setCurrentRow(0);
        QTest::qWait(10);
        QVERIFY(window.grab().save(artifact + "/minimum-window.png"));
    }
}
void PlannerTests::todoUiCreateEditDeleteConfirmation()
{
    Database db;
    QString error;
    QVERIFY(db.open(":memory:", error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    TodoPage page(s, false);
    page.show();
    auto *list = page.findChild<QListWidget *>("todoList");
    auto *add = page.findChild<QPushButton *>("addTodo");
    auto *edit = page.findChild<QPushButton *>("editTodo");
    auto *remove = page.findChild<QPushButton *>("deleteTodo");
    QVERIFY(list && add && edit && remove);
    auto fillDialog = [](const QString &title)
    {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        auto *field = dialog->findChild<QLineEdit *>("todoTitle");
        auto *buttons = dialog->findChild<QDialogButtonBox *>();
        QVERIFY(field && buttons);
        field->setText(title);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Save), Qt::LeftButton);
    };
    QTimer::singleShot(50, &page, [&] { fillDialog(QStringLiteral("界面创建任务")); });
    QTest::mouseClick(add, Qt::LeftButton);
    QCOMPARE(s.todos().size(), 1);
    QCOMPARE(list->count(), 1);
    list->setCurrentRow(0);
    QTimer::singleShot(50, &page, [&] { fillDialog(QStringLiteral("界面修改任务")); });
    QTest::mouseClick(edit, Qt::LeftButton);
    QCOMPARE(s.todos().first().title, QStringLiteral("界面修改任务"));
    QTimer::singleShot(50, &page,
                       []
                       {
                           auto *box =
                               qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                           QVERIFY(box);
                           QTest::mouseClick(box->button(QMessageBox::No), Qt::LeftButton);
                       });
    QTest::mouseClick(remove, Qt::LeftButton);
    QCOMPARE(s.todos().size(), 1);
    QTimer::singleShot(50, &page,
                       []
                       {
                           auto *box =
                               qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                           QVERIFY(box);
                           QTest::mouseClick(box->button(QMessageBox::Yes), Qt::LeftButton);
                       });
    QTest::mouseClick(remove, Qt::LeftButton);
    QVERIFY(s.todos().isEmpty());
    QCOMPARE(list->count(), 0);
}
void PlannerTests::notesDeleteConfirmationAndFailedSave()
{
    QTemporaryDir dir;
    QString error;
    Database db;
    const auto path = dir.filePath("notes.sqlite");
    QVERIFY(db.open(path, error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    NotesPage page(s);
    page.show();
    page.quickNote();
    auto *remove = page.findChild<QPushButton *>("deleteNote");
    auto *content = page.findChild<QPlainTextEdit *>("noteContent");
    QVERIFY(remove && content);
    QTimer::singleShot(50, &page,
                       []
                       {
                           auto *box =
                               qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                           QVERIFY(box);
                           QTest::mouseClick(box->button(QMessageBox::No), Qt::LeftButton);
                       });
    QTest::mouseClick(remove, Qt::LeftButton);
    QCOMPARE(s.notes().size(), 1);
    QTimer::singleShot(50, &page,
                       []
                       {
                           auto *box =
                               qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                           QVERIFY(box);
                           QTest::mouseClick(box->button(QMessageBox::Yes), Qt::LeftButton);
                       });
    QTest::mouseClick(remove, Qt::LeftButton);
    QVERIFY(s.notes().isEmpty());
    page.quickNote();
    content->setPlainText(QStringLiteral("保存失败时不要丢失这段内容"));
    const QString connection = "notes_failure";
    {
        auto raw = QSqlDatabase::addDatabase("QSQLITE", connection);
        raw.setDatabaseName(path);
        QVERIFY(raw.open());
        QSqlQuery q(raw);
        QVERIFY(q.exec("DROP TABLE notes"));
    }
    QSqlDatabase::removeDatabase(connection);
    QVERIFY(!page.flush());
    page.quickNote();
    QCOMPARE(content->toPlainText(), QStringLiteral("保存失败时不要丢失这段内容"));
    QCOMPARE(s.notes().size(), 1);
}
void PlannerTests::migrateVersionOneWithoutDataLoss()
{
    QTemporaryDir dir;
    QString error;
    const auto path = dir.filePath("v1.sqlite");
    const QString connection = "migration_seed";
    const auto created = QDateTime::currentDateTimeUtc().addDays(-2).toString(Qt::ISODateWithMs);
    {
        auto raw = QSqlDatabase::addDatabase("QSQLITE", connection);
        raw.setDatabaseName(path);
        QVERIFY(raw.open());
        QSqlQuery q(raw);
        QVERIFY(
            q.exec("CREATE TABLE todos (id TEXT PRIMARY KEY,title TEXT,description TEXT,created_at "
                   "TEXT,due_at TEXT,completed INTEGER,completed_at TEXT,updated_at "
                   "TEXT,reminder_enabled INTEGER,reminder_at TEXT,reminded_at TEXT)"));
        QVERIFY(q.exec("CREATE TABLE notes (id TEXT PRIMARY KEY,title TEXT,content TEXT,created_at "
                       "TEXT,updated_at TEXT)"));
        q.prepare("INSERT INTO todos VALUES (?,?,?, ?,NULL,0,NULL,?,0,NULL,NULL)");
        q.addBindValue("old-task");
        q.addBindValue(QStringLiteral("旧版中文任务"));
        q.addBindValue(QStringLiteral("保留描述"));
        q.addBindValue(created);
        q.addBindValue(created);
        QVERIFY(q.exec());
        q.prepare("INSERT INTO notes VALUES (?,?,?,?,?)");
        q.addBindValue("old-note");
        q.addBindValue(QStringLiteral("旧笔记"));
        q.addBindValue(QStringLiteral("升级不得丢失"));
        q.addBindValue(created);
        q.addBindValue(created);
        QVERIFY(q.exec());
        QVERIFY(q.exec("PRAGMA user_version=1"));
    }
    QSqlDatabase::removeDatabase(connection);
    {
        Database db;
        QVERIFY2(db.open(path, error), qPrintable(error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        auto t = s.todo("old-task");
        QVERIFY(t);
        QCOMPARE(t->title, QStringLiteral("旧版中文任务"));
        QVERIFY(!t->plannedDate.isValid());
        QVERIFY(!t->isDeadline);
        QCOMPARE(t->createdAt.toString(Qt::ISODateWithMs), created);
        QCOMPARE(s.note("old-note")->content, QStringLiteral("升级不得丢失"));
        t->plannedDate = QDate::currentDate();
        QVERIFY(s.saveTodo(*t));
    }
    Database db;
    QVERIFY(db.open(path, error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    QCOMPARE(s.todo("old-task")->plannedDate, QDate::currentDate());
    QCOMPARE(s.notes().size(), 1);
}
void PlannerTests::calendarDatesAndDeadlinePersistence()
{
    QTemporaryDir dir;
    QString error, id;
    const QDate day(2026, 10, 9);
    {
        Database db;
        QVERIFY(db.open(dir.filePath("calendar.sqlite"), error));
        PlannerService s(db);
        QVERIFY(s.initialize(error));
        Todo t;
        t.title = "planned only";
        t.plannedDate = day;
        QVERIFY(s.saveTodo(t));
        QCOMPARE(s.todosForDate(day).size(), 1);
        QVERIFY(!s.todosForDate(day).first().dueAt.isValid());
        t = s.todos().first();
        id = t.id;
        t.dueAt = QDateTime(day.addDays(2), QTime(18, 0)).toUTC();
        QVERIFY(s.saveTodo(t));
        QCOMPARE(s.todosForDate(day).size(), 1);
        QCOMPARE(s.todosForDate(day.addDays(2)).size(), 1);
        t.dueAt = QDateTime(day, QTime(18, 0)).toUTC();
        QVERIFY(s.saveTodo(t));
        QCOMPARE(s.todosForDate(day).size(), 1); // 同日去重。
        Todo deadline;
        deadline.title = "deadline";
        deadline.isDeadline = true;
        QVERIFY(!s.saveTodo(deadline));
        deadline.dueAt = QDateTime(day, QTime(20, 0)).toUTC();
        deadline.plannedDate = day.addDays(4);
        QVERIFY(s.saveTodo(deadline));
        QCOMPARE(s.todosForDate(day).size(), 2);
        QVERIFY(s.todosForDate(day.addDays(4)).isEmpty());
        auto saved = s.todosForDate(day).last();
        QVERIFY(saved.isDeadline);
        QVERIFY(!saved.plannedDate.isValid());
        saved.dueAt = QDateTime(day.addDays(1), QTime(8, 0)).toUTC();
        QVERIFY(s.saveTodo(saved));
        QCOMPARE(s.todosForDate(day).size(), 1);
        QCOMPARE(s.todosForDate(day.addDays(1)).size(), 1);
    }
    Database db;
    QVERIFY(db.open(dir.filePath("calendar.sqlite"), error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    QCOMPARE(s.todo(id)->plannedDate, day);
    QVERIFY(s.todosForDate(day.addDays(1)).first().isDeadline);
    QVERIFY(s.todosForDate(QDate()).isEmpty());
}
void PlannerTests::calendarClickCreatesSelectedDayTask()
{
    Database db;
    QString error;
    QVERIFY(db.open(":memory:", error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    CalendarPage page(s);
    page.resize(900, 700);
    page.show();
    const auto date = QDate::currentDate().addDays(2);
    page.setMonth(date);
    CalendarCell *chosen = nullptr;
    for (auto *button : page.findChildren<QAbstractButton *>())
        if (auto *cell = dynamic_cast<CalendarCell *>(button); cell && cell->date() == date)
            chosen = cell;
    QVERIFY(chosen);
    QTimer::singleShot(
        50, &page,
        [&]
        {
            auto *day = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(day);
            QCOMPARE(day->objectName(), QString("dayTasksDialog"));
            auto *add = day->findChild<QPushButton *>("addTodo");
            QVERIFY(add);
            QTimer::singleShot(
                50, &page,
                [&]
                {
                    auto *editor = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                    QVERIFY(editor);
                    auto *title = editor->findChild<QLineEdit *>("todoTitle");
                    QVERIFY(title);
                    title->setText(QStringLiteral("点击日期创建任务"));
                    auto *buttons = editor->findChild<QDialogButtonBox *>();
                    QVERIFY(buttons);
                    QTest::mouseClick(buttons->button(QDialogButtonBox::Save), Qt::LeftButton);
                });
            QTest::mouseClick(add, Qt::LeftButton);
            QCOMPARE(s.todosForDate(date).size(), 1);
            const auto artifact = qEnvironmentVariable("PLANNER_TEST_ARTIFACT_DIR");
            if (!artifact.isEmpty())
            {
                auto *list = day->findChild<QListWidget *>("todoList");
                QVERIFY(list);
                list->setCurrentRow(0);
                QVERIFY(day->grab().save(artifact + "/day-editor.png"));
            }
            day->accept();
        });
    QTest::mouseClick(chosen, Qt::LeftButton);
    QCOMPARE(s.todos().size(), 1);
    QCOMPARE(s.todos().first().plannedDate, date);
    QVERIFY(!s.todos().first().dueAt.isValid());
    QCOMPARE(chosen->entries().size(), 1);
    auto *next = page.findChild<QPushButton *>("nextMonth");
    QVERIFY(next);
    QTest::mouseClick(next, Qt::LeftButton);
    QCOMPARE(page.selectedDate().month(), page.displayedMonth().month());
}
void PlannerTests::deadlineEditorAndThemePersistence()
{
    Todo deadline;
    deadline.isDeadline = true;
    TodoDialog editor(deadline);
    auto *due = editor.findChild<QCheckBox *>("hasDueDate");
    auto *plan = editor.findChild<QDateEdit *>("plannedDate");
    QVERIFY(due && plan);
    QVERIFY(due->isChecked());
    QVERIFY(!due->isEnabled());
    QVERIFY(!plan->isEnabled());
    QVERIFY(editor.value().dueAt.isValid());
    QVERIFY(editor.value().isDeadline);
    auto *dueEditor = editor.findChild<QDateTimeEdit *>("dueDateTime");
    QVERIFY(dueEditor);
    const auto editedTime = QDateTime(QDate::currentDate().addDays(7), QTime(17, 30));
    dueEditor->setDateTime(editedTime);
    QCOMPARE(editor.value().dueAt, editedTime.toUTC());
    const auto editorArtifact = qEnvironmentVariable("PLANNER_TEST_ARTIFACT_DIR");
    if (!editorArtifact.isEmpty())
    {
        editor.findChild<QLineEdit *>("todoTitle")->setText(QStringLiteral("项目报告提交"));
        editor.show();
        QTest::qWait(20);
        QVERIFY(editor.grab().save(editorArtifact + "/deadline-editor.png"));
        editor.hide();
    }
    QTemporaryDir dir;
    const auto path = dir.filePath("settings.ini");
    Database db;
    QString error;
    QVERIFY(db.open(":memory:", error));
    PlannerService s(db);
    QVERIFY(s.initialize(error));
    Todo t;
    t.title = QStringLiteral("设计日历与主题");
    t.plannedDate = QDate::currentDate();
    QVERIFY(s.saveTodo(t));
    deadline.title = QStringLiteral("报告提交 Deadline");
    deadline.dueAt = QDateTime(QDate::currentDate().addDays(2), QTime(18, 0)).toUTC();
    QVERIFY(s.saveTodo(deadline));
    {
        AppSettings settings(path);
        MainWindow window(s, settings, dir.path());
        window.show();
        auto *themes = window.findChild<QComboBox *>("themeSelector");
        auto *opacity = window.findChild<QSlider *>("windowOpacity");
        QVERIFY(themes && opacity);
        QCOMPARE(themes->count(), 5);
        const auto artifact = qEnvironmentVariable("PLANNER_TEST_ARTIFACT_DIR");
        for (int i = 0; i < themes->count(); ++i)
        {
            themes->setCurrentIndex(i);
            QCOMPARE(qApp->property("themeId").toString(), themes->itemData(i).toString());
            QTest::qWait(10);
            if (!artifact.isEmpty())
                QVERIFY(window.grab().save(artifact + "/theme-" + themes->itemData(i).toString() +
                                           ".png"));
        }
        themes->setCurrentIndex(themes->findData("lavender"));
        opacity->setValue(90);
        QVERIFY(qAbs(window.windowOpacity() - 0.9) < 0.01);
    }
    AppSettings restored(path);
    QCOMPARE(restored.get("appearance/theme").toString(), QString("lavender"));
    QCOMPARE(restored.get("appearance/opacity").toInt(), 90);
    {
        MainWindow window(s, restored, dir.path());
        QCOMPARE(qApp->property("themeId").toString(), QString("lavender"));
        QVERIFY(qAbs(window.windowOpacity() - 0.9) < 0.01);
    }
    QCOMPARE(themeForId("missing").id, QString("mist"));
    applyAppStyle(*qApp);
}
QTEST_MAIN(PlannerTests)
#include "PlannerTests.moc"
