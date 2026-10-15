#include "settings/settings_form_layout.h"
#include "ui/disclosure_section.h"
#include "ui/form_row_visibility.h"
#include "ui/settings_card.h"
#include "ui/settings_form_row.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtTest/QtTest>

using markshot::ui::SettingsCard;
using markshot::ui::SettingsRowLabel;

namespace {

/// @brief 构造与设置页一致的滚动页面，并返回内容页
/// @param area 外层滚动区
/// @return 已安装垂直布局的内容页
QWidget *createScrollPage(QScrollArea &area)
{
    area.setWidgetResizable(true);
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);
    area.setWidget(page);
    return page;
}

/// @brief 创建带统一表单的卡片并加入页面
/// @param page 内容页
/// @param title 卡片标题
/// @param mode 卡片呈现方式
/// @return 卡片内的表单布局
QFormLayout *addCard(QWidget *page, const QString &title, SettingsCard::Mode mode)
{
    auto *card = new SettingsCard(title, QStringLiteral("Card description"), mode, page);
    QFormLayout *form = markshot::settings::createSettingsFormLayout(card->bodyLayout());
    static_cast<QVBoxLayout *>(page->layout())->addWidget(card);
    return form;
}

}  // namespace

class SettingsCardTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证常驻卡片与折叠卡片的结构、对象名与表单查找方式
    /// @return 无返回值
    void buildsStaticAndCollapsibleCards()
    {
        // 1. 常驻卡片显示标题，说明作为提示保留
        SettingsCard plain(QStringLiteral("Capture"), QStringLiteral("Capture options"), SettingsCard::Mode::Static);
        markshot::settings::createSettingsFormLayout(plain.bodyLayout());
        QCOMPARE(plain.objectName(), QStringLiteral("settingsCard"));
        QVERIFY(plain.titleLabel());
        QCOMPARE(plain.titleLabel()->text(), QStringLiteral("Capture"));
        QCOMPARE(plain.titleLabel()->toolTip(), QStringLiteral("Capture options"));
        QVERIFY(!plain.disclosure());
        QVERIFY(plain.findChild<QFormLayout *>(QStringLiteral("settingsCardForm")));

        // 2. 折叠卡片复用折叠分组，默认收起且说明位于内容区
        SettingsCard folded(QStringLiteral("Advanced"), QStringLiteral("Rarely used"), SettingsCard::Mode::Collapsible);
        markshot::settings::createSettingsFormLayout(folded.bodyLayout());
        QVERIFY(!folded.titleLabel());
        QVERIFY(folded.disclosure());
        QVERIFY(!folded.disclosure()->isExpanded());
        QVERIFY(folded.findChild<QFormLayout *>(QStringLiteral("settingsCardForm")));
        auto *help = folded.findChild<QLabel *>(QStringLiteral("settingsCardDescription"));
        QVERIFY(help);
        QCOMPARE(help->text(), QStringLiteral("Rarely used"));
    }

    /// @brief 验证常驻与折叠卡片中的字段使用同一起点，标题说明位于控件左侧
    /// @return 无返回值
    void alignsFieldsAcrossCardModes()
    {
        // 1. 两种卡片各放一个长短不同的字段，展开折叠卡片
        QScrollArea area;
        QWidget *page = createScrollPage(area);
        QFormLayout *first = addCard(page, QStringLiteral("General"), SettingsCard::Mode::Static);
        QFormLayout *second = addCard(page, QStringLiteral("Advanced"), SettingsCard::Mode::Collapsible);
        static_cast<QVBoxLayout *>(page->layout())->addStretch();
        auto *shortField = new QComboBox;
        auto *longField = new QLineEdit;
        markshot::ui::addSettingsFormRow(first, QStringLiteral("Theme"), QStringLiteral("Interface colors"), shortField);
        SettingsRowLabel *longLabel = markshot::ui::addSettingsFormRow(
            second, QStringLiteral("Window detection working directory"), {}, longField);
        page->findChild<markshot::ui::DisclosureSection *>()->setExpanded(true);
        area.resize(760, 480);
        area.show();
        QVERIFY(QTest::qWaitForWindowExposed(&area));

        // 2. 字段左边缘一致，标签区在字段左侧且不重叠
        QTRY_COMPARE(shortField->mapTo(page, QPoint()).x(), longField->mapTo(page, QPoint()).x());
        QVERIFY(longLabel->mapTo(page, QPoint(longLabel->width(), 0)).x() <= longField->mapTo(page, QPoint()).x());
        auto *shortLabel = static_cast<SettingsRowLabel *>(first->labelForField(shortField));
        QVERIFY(shortLabel->descriptionLabel());
        QVERIFY(shortLabel->descriptionLabel()->isVisible());
        QVERIFY(shortLabel->descriptionLabel()->geometry().top() >= shortLabel->titleLabel()->geometry().bottom());
        QVERIFY(!longLabel->descriptionLabel());
    }

    /// @brief 验证点击开关所在行的标签区会切换开关，禁用控件时标签同步禁用
    /// @return 无返回值
    void labelClickTogglesSwitchAndFollowsEnabledState()
    {
        // 1. 建立只显示指示器的开关行
        QScrollArea area;
        QWidget *page = createScrollPage(area);
        QFormLayout *form = addCard(page, QStringLiteral("Startup"), SettingsCard::Mode::Static);
        auto *box = new QCheckBox;
        SettingsRowLabel *label = markshot::ui::addSettingsFormRow(
            form, QStringLiteral("Start in Tray"), QStringLiteral("Launch into the system tray."), box);
        area.resize(760, 360);
        area.show();
        QVERIFY(QTest::qWaitForWindowExposed(&area));
        QCOMPARE(box->accessibleName(), QStringLiteral("Start in Tray"));
        QCOMPARE(box->accessibleDescription(), QStringLiteral("Launch into the system tray."));

        // 2. 点击标题与说明都会切换状态
        QTest::mouseClick(label->titleLabel(), Qt::LeftButton);
        QVERIFY(box->isChecked());
        QTest::mouseClick(label->descriptionLabel(), Qt::LeftButton);
        QVERIFY(!box->isChecked());

        // 3. 控件禁用后标签置灰，点击不再改变状态
        box->setEnabled(false);
        QVERIFY(!label->isEnabled());
        QTest::mouseClick(label->titleLabel(), Qt::LeftButton);
        QVERIFY(!box->isChecked());
        box->setEnabled(true);
        QVERIFY(label->isEnabled());
    }

    /// @brief 验证窄视区时卡片切换紧凑内边距、标签位于控件上方且没有横向滚动
    /// @return 无返回值
    void stacksRowsAndCompactsCardInNarrowViewport()
    {
        // 1. 宽视区下保持左右排列
        QScrollArea area;
        QWidget *page = createScrollPage(area);
        QFormLayout *form = addCard(page, QStringLiteral("Saving"), SettingsCard::Mode::Static);
        auto *field = new QLineEdit(QStringLiteral("retained value"));
        SettingsRowLabel *label = markshot::ui::addSettingsFormRow(
            form, QStringLiteral("Path Template"), QStringLiteral("Default file name for saved screenshots."), field);
        auto *card = page->findChild<SettingsCard *>();
        area.resize(760, 360);
        area.show();
        QVERIFY(QTest::qWaitForWindowExposed(&area));
        QTRY_VERIFY(field->geometry().left() > label->geometry().right());
        QVERIFY(!card->isCompact());

        // 2. 缩窄后上下排列，卡片使用紧凑内边距，输入内容不变
        area.resize(340, 360);
        QTRY_VERIFY(field->geometry().top() > label->geometry().bottom());
        QTRY_VERIFY(card->isCompact());
        QTRY_COMPARE(area.horizontalScrollBar()->maximum(), 0);
        QCOMPARE(field->text(), QStringLiteral("retained value"));
    }

    /// @brief 验证按字段切换可见性时复合标签同步隐藏和恢复
    /// @return 无返回值
    void hidesCompositeLabelWithField()
    {
        SettingsCard card(QStringLiteral("Clipboard"), {}, SettingsCard::Mode::Static);
        QFormLayout *form = markshot::settings::createSettingsFormLayout(card.bodyLayout());
        auto *field = new QLineEdit;
        SettingsRowLabel *label = markshot::ui::addSettingsFormRow(form, QStringLiteral("Threshold"), {}, field);
        card.show();
        QVERIFY(QTest::qWaitForWindowExposed(&card));
        QCOMPARE(form->labelForField(field), label);
        markshot::ui::setFormRowVisible(form, field, false);
        QVERIFY(label->isHidden());
        markshot::ui::setFormRowVisible(form, field, true);
        QVERIFY(!label->isHidden());
    }

    /// @brief 验证标签列宽在上下限之间按视区宽度变化
    /// @return 无返回值
    void labelColumnWidthStaysWithinBounds()
    {
        const int line = 16;
        QCOMPARE(markshot::settings::settingsLabelColumnWidth(line, 200), line * 7);
        QCOMPARE(markshot::settings::settingsLabelColumnWidth(line, 3000), line * 13);
        const int middle = markshot::settings::settingsLabelColumnWidth(line, 600);
        QVERIFY(middle >= line * 7 && middle <= 300);
    }
};

QTEST_MAIN(SettingsCardTest)
#include "settings_card_test.moc"
