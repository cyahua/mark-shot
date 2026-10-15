#include "settings/settings_color_button.h"
#include "settings/settings_dialog.h"
#include "settings/settings_field_metrics.h"
#include "settings/settings_form_layout.h"
#include "settings/settings_path_field.h"
#include "settings/settings_shortcut_edit.h"
#include "settings/settings_ui_helpers.h"
#include "ui/i18n.h"
#include "ui/theme.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QEnterEvent>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QtTest>

using markshot::settings::SettingsPathField;
using markshot::settings::SettingsShortcutEdit;

/// @brief 检查设置字段的胶囊快捷键、颜色胶囊、路径浏览、环境变量框、右侧对齐与底部按钮
class SettingsFieldWidgetsTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 统一使用中文界面，验证新增文案
    /// @return 无返回值
    void initTestCase()
    {
        markshot::i18n::setLanguage(markshot::i18n::Language::Chinese);
    }

    /// @brief 验证快捷键胶囊的提示文案、录入状态与悬停清除图标
    /// @return 无返回值
    void shortcutCapsuleStates()
    {
        QWidget window;
        auto *layout = new QVBoxLayout(&window);
        auto *edit = new SettingsShortcutEdit(&window);
        auto *other = new QLineEdit(&window);
        layout->addWidget(edit);
        layout->addWidget(other);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        other->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY(!edit->isRecording());

        // 1. 【设置测试】【快捷键胶囊】常态有样式背景、固定宽度与中文提示
        QVERIFY(edit->testAttribute(Qt::WA_StyledBackground));
        QCOMPARE(edit->width(), markshot::settings::metrics::kShortcutFieldWidth);
        QCOMPARE(edit->displayEdit()->placeholderText(), QStringLiteral("点击设置快捷键"));
        QVERIFY(!edit->isClearVisible());

        // 2. 【设置测试】【快捷键胶囊】聚焦进入录入态，提示切换且属性驱动青色边框
        edit->setFocus(Qt::MouseFocusReason);
        QTRY_VERIFY(edit->isRecording());
        QCOMPARE(edit->property("recording").toBool(), true);
        QCOMPARE(edit->displayEdit()->placeholderText(), QStringLiteral("请按下快捷键..."));
        QVERIFY(!edit->isClearVisible());

        // 3. 【设置测试】【快捷键胶囊】已有快捷键时聚焦可见清除图标，触发后清空并发出变更
        edit->setKeySequence(QKeySequence(QStringLiteral("Ctrl+Z")));
        QVERIFY(edit->isClearVisible());
        QSignalSpy changed(edit, &QKeySequenceEdit::keySequenceChanged);
        edit->clearAction()->trigger();
        QVERIFY(edit->keySequence().isEmpty());
        QCOMPARE(changed.count(), 1);
        QVERIFY(!edit->isClearVisible());
        QCOMPARE(edit->displayEdit()->placeholderText(), QStringLiteral("请按下快捷键..."));

        // 4. 【设置测试】【快捷键胶囊】失焦后退出录入，仅悬停时显示清除图标
        edit->setKeySequence(QKeySequence(QStringLiteral("F")));
        other->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY(!edit->isRecording());
        QCOMPARE(edit->displayEdit()->placeholderText(), QStringLiteral("点击设置快捷键"));
        QVERIFY(!edit->isClearVisible());
        QEnterEvent enter(QPointF(4, 4), QPointF(4, 4), edit->mapToGlobal(QPointF(4, 4)));
        QApplication::sendEvent(edit, &enter);
        QVERIFY(edit->isClearVisible());
        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(edit, &leave);
        QVERIFY(!edit->isClearVisible());
    }

    /// @brief 验证颜色胶囊固定约 120px，显示色块与 Hex，不再整块填充颜色
    /// @return 无返回值
    void colorSwatchButton()
    {
        QPushButton button;
        markshot::settings::setupColorSwatchButton(&button);
        markshot::settings::updateColorSwatchButton(&button, QColor(255, 77, 77), Qt::black);
        QCOMPARE(button.width(), markshot::settings::metrics::kColorFieldWidth);
        QCOMPARE(button.text(), QStringLiteral("#FF4D4D"));
        QVERIFY(!button.icon().isNull());
        QVERIFY(button.styleSheet().isEmpty());
        markshot::settings::updateColorSwatchButton(&button, QColor(), QColor(94, 234, 212));
        QCOMPARE(button.text(), QStringLiteral("#5EEAD4"));
    }

    /// @brief 验证命令字段浏览只替换程序路径，目录字段整体替换
    /// @return 无返回值
    void pathFieldKeepsCommandArguments()
    {
#ifdef Q_OS_WIN
        const QString programDirectory = QStringLiteral("C:/opt/ocr/bin");
#else
        const QString programDirectory = QStringLiteral("/opt/ocr/bin");
#endif
        const QString program = programDirectory + QStringLiteral("/run");
        const QString expectedCommand = QDir::toNativeSeparators(program) + QStringLiteral(" {image} --lang zh");
        SettingsPathField command(SettingsPathField::Kind::Command, QStringLiteral("OCR"));
        command.lineEdit()->setText(QStringLiteral("mark-shot-ocr {image} --lang zh"));
        command.applySelectedPath(program);
        QCOMPARE(command.lineEdit()->text(), expectedCommand);
        QCOMPARE(command.dialogStartPath(), programDirectory);
        command.applySelectedPath(QString());
        QCOMPARE(command.lineEdit()->text(), expectedCommand);

        SettingsPathField directory(SettingsPathField::Kind::Directory, QStringLiteral("Dir"));
        directory.lineEdit()->setText(QStringLiteral("~/Videos"));
        QCOMPARE(directory.dialogStartPath(), QDir::homePath() + QStringLiteral("/Videos"));
        directory.applySelectedPath(QStringLiteral("/data/videos"));
        QCOMPARE(directory.lineEdit()->text(), QDir::toNativeSeparators(QStringLiteral("/data/videos")));
        QCOMPARE(directory.browseButton()->text(), QStringLiteral("浏览..."));
        QVERIFY(!directory.browseButton()->icon().isNull());
    }

    /// @brief 验证普通字段受宽度上限约束并贴齐右缘，路径字段自适应拉伸
    /// @return 无返回值
    void fieldsAlignRight()
    {
        QWidget window;
        window.resize(820, 400);
        auto *root = new QVBoxLayout(&window);
        QFormLayout *form = markshot::settings::createSettingsFormLayout(root);
        QLineEdit *text = markshot::settings::addTextRow(form, QStringLiteral("Model"));
        QSpinBox *spin = markshot::settings::addSpinRow(form, QStringLiteral("Timeout"), 0, 1000);
        QComboBox *combo = markshot::settings::addComboRow(form, QStringLiteral("Theme"));
        QLineEdit *path = markshot::settings::addPathRow(form, QStringLiteral("Command"),
                                                         SettingsPathField::Kind::Command);
        root->addStretch();
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        // 1. 【设置测试】【右侧对齐】普通字段宽度不超上限，右缘与路径字段一致
        const auto rightEdge = [&window](QWidget *widget) {
            return widget->mapTo(&window, QPoint(widget->width(), 0)).x();
        };
        QWidget *pathField = path->parentWidget();
        QTRY_COMPARE(rightEdge(text), rightEdge(pathField));
        QCOMPARE(rightEdge(spin), rightEdge(pathField));
        QCOMPARE(rightEdge(combo), rightEdge(pathField));
        QVERIFY(text->width() <= markshot::settings::metrics::kTextFieldMaxWidth);
        QVERIFY(combo->width() <= markshot::settings::metrics::kTextFieldMaxWidth);
        // 2. 【设置测试】【路径自适应】路径字段宽于普通字段
        QVERIFY(pathField->width() > text->width());
    }

    /// @brief 验证完整设置窗口的字段类型、底部按钮状态与配置往返
    /// @return 无返回值
    void dialogUsesNewFields()
    {
        markshot::settings::SettingsDialog dialog;
        dialog.resize(860, 620);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));

        // 【设置测试】【完整字段】显式访问延迟页面后再检查全部字段
        auto *stack = dialog.findChild<QStackedWidget *>();
        QVERIFY(stack);
        // 【设置测试】【按需页面】检查全部字段前先访问各分类，不要求初次打开就创建全部控件
        for (int index = 0; index < stack->count(); ++index) stack->setCurrentIndex(index);
        stack->setCurrentIndex(0);

        // 1. 【设置测试】【快捷键】所有快捷键字段均为胶囊录入框
        const auto edits = dialog.findChildren<QKeySequenceEdit *>();
        QVERIFY(!edits.isEmpty());
        for (QKeySequenceEdit *edit : edits) {
            QVERIFY(qobject_cast<SettingsShortcutEdit *>(edit));
        }

        // 2. 【设置测试】【环境变量】等宽字体且约 4 行高度
        const auto envs = dialog.findChildren<QPlainTextEdit *>(QStringLiteral("settingsEnvironmentEdit"));
        QCOMPARE(envs.size(), 3);
        for (QPlainTextEdit *env : envs) {
            QCOMPARE(env->font().family(), markshot::theme::monospaceFontFamily());
            QVERIFY(env->height() <= env->fontMetrics().lineSpacing() * 5 + 16);
        }

        // 3. 【设置测试】【路径字段】命令与目录字段带浏览按钮
        QCOMPARE(dialog.findChildren<SettingsPathField *>().size(), 9);
        QCOMPARE(dialog.findChildren<QPushButton *>(QStringLiteral("settingsColorButton")).size(), 2);

        // 4. 【设置测试】【底部按钮】保存为主按钮且无修改时禁用，撤销为带图标的次级按钮
        auto *save = dialog.findChild<QPushButton *>(QStringLiteral("settingsSave"));
        auto *revert = dialog.findChild<QPushButton *>(QStringLiteral("settingsRevert"));
        QVERIFY(save && revert);
        QCOMPARE(save->property("role").toString(), QStringLiteral("primary"));
        QCOMPARE(revert->property("role").toString(), QStringLiteral("secondary"));
        QVERIFY(!revert->icon().isNull());
        QVERIFY(!save->isEnabled() && !revert->isEnabled());

        // 5. 【设置测试】【变更追踪】修改快捷键与命令后可保存，撤销恢复原值
        auto *shortcut = qobject_cast<SettingsShortcutEdit *>(edits.first());
        const QKeySequence originalShortcut = shortcut->keySequence();
        shortcut->setKeySequence(QKeySequence(QStringLiteral("Ctrl+Alt+F9")));
        QTRY_VERIFY(save->isEnabled());
        auto *command = dialog.findChildren<SettingsPathField *>().first();
        const QString originalCommand = command->lineEdit()->text();
        command->applySelectedPath(QStringLiteral("/opt/x/tool"));
        QVERIFY(revert->isEnabled());
        QTest::mouseClick(revert, Qt::LeftButton);
        QCOMPARE(shortcut->keySequence(), originalShortcut);
        QCOMPARE(command->lineEdit()->text(), originalCommand);
        QVERIFY(!save->isEnabled());
    }
};

/// @brief 使用隔离配置运行设置字段测试
/// @param argc 参数数量
/// @param argv 参数内容
/// @return 测试退出码
int main(int argc, char **argv)
{
    QTemporaryDir isolated;
    if (!isolated.isValid()) {
        return 1;
    }
    for (const char *name : {"XDG_CONFIG_HOME", "XDG_CACHE_HOME", "XDG_DATA_HOME"}) {
        const QString path = isolated.filePath(QString::fromLatin1(name));
        QDir().mkpath(path);
        qputenv(name, path.toUtf8());
    }
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("mark-shot-settings-field-widgets-test"));
    SettingsFieldWidgetsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "settings_field_widgets_test.moc"
