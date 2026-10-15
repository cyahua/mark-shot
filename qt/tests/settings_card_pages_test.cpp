#include "settings/settings_design_tokens.h"
#include "settings/settings_dialog.h"
#include "ui/disclosure_section.h"
#include "ui/settings_card.h"
#include "ui/settings_form_row.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QtTest>

using markshot::ui::SettingsCard;
using markshot::ui::SettingsRowLabel;

namespace {

/// @brief 查找表单所在的分类卡片
/// @param form 设置表单
/// @return 所属卡片，不在卡片内时返回 nullptr
SettingsCard *owningCard(QFormLayout *form)
{
    for (QWidget *widget = form->parentWidget(); widget; widget = widget->parentWidget()) {
        if (auto *card = qobject_cast<SettingsCard *>(widget)) {
            return card;
        }
    }
    return nullptr;
}

/// @brief 查找当前可见且可操作的第一个设置开关，不依赖界面语言
/// @param root 查找范围
/// @return 开关控件，未找到时返回 nullptr
QCheckBox *findVisibleSwitch(QWidget *root)
{
    for (QCheckBox *box : root->findChildren<QCheckBox *>()) {
        if (box->isVisible() && box->isEnabled()) {
            return box;
        }
    }
    return nullptr;
}

}  // namespace

/// @brief 检查完整设置窗口的所有页面均接入分类卡片与统一字段行，且交互与变更追踪保持不变
class SettingsCardPagesTest final : public QObject {
    Q_OBJECT

private slots:
    /** 首次显示只创建常规页，访问其他分类才创建内容，无参数和返回值。 */
    void defersUnvisitedPages()
    {
        markshot::settings::SettingsDialog dialog;
        auto *stack = dialog.findChild<QStackedWidget *>();
        auto *save = dialog.findChild<QPushButton *>(QStringLiteral("settingsSave"));
        QVERIFY(stack && save);
        for (int index = 0; index < stack->count(); ++index) {
            auto *area = qobject_cast<QScrollArea *>(stack->widget(index));
            QVERIFY(area);
            QCOMPARE(area->widget() != nullptr, index == 0);
        }
        for (int index = 1; index < stack->count(); ++index) {
            stack->setCurrentIndex(index);
            QVERIFY(qobject_cast<QScrollArea *>(stack->widget(index))->widget());
            QVERIFY(!save->isEnabled());
        }
    }

    /** 新建分类不覆盖其他页面的未保存输入，原位撤销仍恢复基线，无参数和返回值。 */
    void preservesDraftAcrossDeferredPages()
    {
        markshot::settings::SettingsDialog dialog;
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        auto *stack = dialog.findChild<QStackedWidget *>();
        auto *save = dialog.findChild<QPushButton *>(QStringLiteral("settingsSave"));
        auto *revert = dialog.findChild<QPushButton *>(QStringLiteral("settingsRevert"));
        QCheckBox *field = findVisibleSwitch(stack->currentWidget());
        QVERIFY(field && save && revert);
        const bool original = field->isChecked();
        field->setChecked(!original);
        QVERIFY(save->isEnabled());
        stack->setCurrentIndex(3);
        QWidget *annotationPage = qobject_cast<QScrollArea *>(stack->widget(3))->widget();
        QVERIFY(annotationPage);
        QVERIFY(save->isEnabled());
        stack->setCurrentIndex(0);
        QCOMPARE(field->isChecked(), !original);
        field->setChecked(original);
        QVERIFY(!save->isEnabled());
        field->setChecked(!original);
        revert->click();
        QCOMPARE(field->isChecked(), original);
        QVERIFY(!save->isEnabled());
        stack->setCurrentIndex(3);
        QCOMPARE(qobject_cast<QScrollArea *>(stack->widget(3))->widget(), annotationPage);
    }

    /** 【设置测试】【延迟保存】保存常规页不创建其他页面，也不覆盖其配置，无参数和返回值。 */
    void savePreservesUnvisitedConfig()
    {
        using namespace markshot::settings;
        const SettingsConfig original = readSettingsConfig();
        SettingsConfig fixture = original;
        fixture.capture.includeCursor = true;
        fixture.shortcuts.startupRuler = QKeySequence(QStringLiteral("Ctrl+Alt+R"));
        fixture.annotation.toolbarIconSize = 30;
        fixture.pinned.borderWidth = 4.5;
        fixture.storage.recordingVideoDirectory = QStringLiteral("/tmp/deferred-video");
        fixture.scroll.previewGap = 12;
        fixture.integrations.uploadEnv.insert(QStringLiteral("UPLOAD_MODE"), QStringLiteral("test"));
        fixture.advanced.appEnv.insert(QStringLiteral("CUSTOM_SETTING"), QStringLiteral("preserved"));
        QString error;
        QVERIFY2(writeSettingsConfig(fixture, &error), qPrintable(error));
        const SettingsConfig before = readSettingsConfig();

        SettingsDialog dialog;
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        auto *stack = dialog.findChild<QStackedWidget *>();
        auto *save = dialog.findChild<QPushButton *>(QStringLiteral("settingsSave"));
        QVERIFY(stack && save);
        QCheckBox *field = findVisibleSwitch(stack->currentWidget());
        QVERIFY(field);
        field->setChecked(!field->isChecked());
        QVERIFY(save->isEnabled());
        save->click();
        QVERIFY(!save->isEnabled());

        SettingsConfig after = readSettingsConfig();
        QVERIFY(settingsConfigToJson(after) != settingsConfigToJson(before));
        after.general = before.general;
        QCOMPARE(settingsConfigToJson(after), settingsConfigToJson(before));
        for (int index = 1; index < stack->count(); ++index) {
            QVERIFY(!qobject_cast<QScrollArea *>(stack->widget(index))->widget());
        }
        QVERIFY2(writeSettingsConfig(original, &error), qPrintable(error));
    }

    /// @brief 验证初次显示不创建集成和插件页面，访问后复用且不产生未保存状态
    /// @return 无返回值
    void defersProviderPages()
    {
        markshot::settings::SettingsDialog dialog;
        auto *stack = dialog.findChild<QStackedWidget *>();
        auto *save = dialog.findChild<QPushButton *>(QStringLiteral("settingsSave"));
        QVERIFY(stack && save);
        auto *integrations = qobject_cast<QScrollArea *>(stack->widget(5));
        auto *plugins = qobject_cast<QScrollArea *>(stack->widget(6));
        QVERIFY(integrations && plugins);
        QVERIFY(!integrations->widget() && !plugins->widget());
        stack->setCurrentIndex(5);
        QVERIFY(integrations->widget());
        QVERIFY(!plugins->widget());
        QWidget *page = integrations->widget();
        stack->setCurrentIndex(0);
        stack->setCurrentIndex(5);
        QCOMPARE(integrations->widget(), page);
        QVERIFY(!save->isEnabled());
        stack->setCurrentIndex(6);
        QVERIFY(plugins->widget());
        QVERIFY(!save->isEnabled());
    }

    /// @brief 验证每个设置页至少包含一张卡片，所有表单都位于卡片内
    /// @return 无返回值
    void everyPageUsesCards()
    {
        markshot::settings::SettingsDialog dialog;
        auto *stack = dialog.findChild<QStackedWidget *>();
        QVERIFY(stack);
        QVERIFY(stack->count() >= 11);
        for (int index = 0; index < stack->count(); ++index) {
            stack->setCurrentIndex(index);
            auto *area = qobject_cast<QScrollArea *>(stack->widget(index));
            QVERIFY(area && area->widget());
            const QString page = QString::fromLatin1(area->widget()->metaObject()->className());
            QVERIFY2(!area->widget()->findChildren<SettingsCard *>().isEmpty(), qPrintable(page));
            for (QFormLayout *form : area->widget()->findChildren<QFormLayout *>(QStringLiteral("settingsCardForm"))) {
                QVERIFY2(owningCard(form), qPrintable(page));
            }
        }
    }

    /// @brief 验证带标签的字段统一使用标题加说明的标签区，开关不再自带文字
    /// @return 无返回值
    void labeledRowsUseUnifiedRowLabel()
    {
        markshot::settings::SettingsDialog dialog;
        auto *stack = dialog.findChild<QStackedWidget *>();
        int rows = 0;
        for (int index = 0; index < stack->count(); ++index) stack->setCurrentIndex(index);
        for (QFormLayout *form : stack->findChildren<QFormLayout *>(QStringLiteral("settingsCardForm"))) {
            for (int row = 0; row < form->rowCount(); ++row) {
                QLayoutItem *label = form->itemAt(row, QFormLayout::LabelRole);
                if (!label || !label->widget()) {
                    continue;
                }
                auto *rowLabel = qobject_cast<SettingsRowLabel *>(label->widget());
                QVERIFY2(rowLabel, qPrintable(label->widget()->metaObject()->className()));
                QLayoutItem *field = form->itemAt(row, QFormLayout::FieldRole);
                QVERIFY(field && field->widget());
                QCOMPARE(rowLabel->buddy(), field->widget());
                ++rows;
            }
        }
        QVERIFY(rows > 40);
        for (QCheckBox *box : stack->findChildren<QCheckBox *>()) {
            QVERIFY2(box->text().isEmpty(), qPrintable(box->accessibleName()));
            QVERIFY2(!box->accessibleName().isEmpty(), "switch without accessible name");
        }
    }

    /// @brief 验证点击开关标签会改变配置并触发未保存状态，撤销后恢复原值
    /// @return 无返回值
    void labelClickMarksChangesAndRevertRestores()
    {
        markshot::settings::SettingsDialog dialog;
        dialog.resize(860, 620);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QCheckBox *tray = findVisibleSwitch(dialog.findChild<QStackedWidget *>()->currentWidget());
        QVERIFY(tray);
        SettingsRowLabel *label = nullptr;
        for (QFormLayout *candidate : dialog.findChildren<QFormLayout *>(QStringLiteral("settingsCardForm"))) {
            if (QWidget *found = candidate->labelForField(tray)) {
                label = qobject_cast<SettingsRowLabel *>(found);
            }
        }
        QVERIFY(label);
        auto *save = dialog.findChild<QPushButton *>(QStringLiteral("settingsSave"));
        auto *revert = dialog.findChild<QPushButton *>(QStringLiteral("settingsRevert"));
        QVERIFY(save && revert);
        QVERIFY(!save->isEnabled());

        // 1. 【设置测试】【标签点击】点击标题切换开关并进入未保存状态
        const bool original = tray->isChecked();
        QTest::mouseClick(label->titleLabel(), Qt::LeftButton);
        QCOMPARE(tray->isChecked(), !original);
        QTRY_VERIFY(save->isEnabled());

        // 2. 【设置测试】【撤销】恢复保存基线，开关回到原状态
        QTest::mouseClick(revert, Qt::LeftButton);
        QCOMPARE(tray->isChecked(), original);
        QVERIFY(!save->isEnabled());
    }

    /// @brief 验证紧凑窗口下所有页面无横向滚动，字段与标签上下排列
    /// @return 无返回值
    void compactWindowHasNoHorizontalScroll()
    {
        markshot::settings::SettingsDialog dialog;
        dialog.setStyleSheet(markshot::settings::tokens::settingsStyleSheet(markshot::ui::UiThemeMode::Dark));
        dialog.resize(440, 620);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        auto *stack = dialog.findChild<QStackedWidget *>();
        for (int index = 0; index < stack->count(); ++index) {
            stack->setCurrentIndex(index);
            stack->setCurrentIndex(index);
            auto *area = qobject_cast<QScrollArea *>(stack->widget(index));
            // 1. 【设置测试】【紧凑布局】展开折叠卡片，确保隐藏内容同样遵守窄宽度
            for (auto *section : area->widget()->findChildren<markshot::ui::DisclosureSection *>()) {
                section->setExpanded(true);
            }
            QTRY_COMPARE(area->horizontalScrollBar()->maximum(), 0);
            for (SettingsCard *card : area->widget()->findChildren<SettingsCard *>()) {
                QTRY_VERIFY2(card->isCompact(), qPrintable(card->title()));
            }
        }
    }
};

/// @brief 使用隔离配置运行设置卡片回归测试
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
    QApplication::setApplicationName(QStringLiteral("mark-shot-settings-card-test"));
    SettingsCardPagesTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "settings_card_pages_test.moc"
