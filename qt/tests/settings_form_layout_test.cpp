#include "settings/settings_form_layout.h"

#include <QCheckBox>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtTest/QtTest>

class SettingsFormLayoutTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证不同分组的长短标签使用同一字段起点
    /// @return 无返回值
    void alignsFieldsAcrossGroups()
    {
        // 1. 创建标签长度不同的两个设置分组
        QWidget window;
        window.resize(680, 400);
        auto *root = new QVBoxLayout(&window);
        auto *first = new QFrame(&window);
        auto *second = new QFrame(&window);
        auto *firstForm = markshot::settings::createSettingsFormLayout(new QVBoxLayout(first));
        auto *secondForm = markshot::settings::createSettingsFormLayout(new QVBoxLayout(second));
        auto *shortField = new QLineEdit(first);
        auto *longField = new QLineEdit(second);
        firstForm->addRow(QStringLiteral("Theme"), shortField);
        secondForm->addRow(QStringLiteral("Screenshot output directory"), longField);
        root->addWidget(first);
        root->addWidget(second);
        root->addStretch();
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        // 2. 核对字段对齐以及长标签与输入控件互不重叠
        QTRY_COMPARE(shortField->mapTo(&window, QPoint()).x(), longField->mapTo(&window, QPoint()).x());
        auto *label = qobject_cast<QLabel *>(secondForm->labelForField(longField));
        QVERIFY(label->wordWrap());
        QVERIFY(label->geometry().right() < longField->geometry().left());
    }

    /// @brief 验证由宽变窄后表单上下排列且输入内容保留
    /// @return 无返回值
    void stacksRowsInNarrowViewport()
    {
        // 1. 在滚动页面中建立带长标签的输入项
        QScrollArea area;
        area.setWidgetResizable(true);
        auto *page = new QWidget;
        auto *form = markshot::settings::createSettingsFormLayout(new QVBoxLayout(page));
        auto *field = new QLineEdit(QStringLiteral("retained value"), page);
        form->addRow(QStringLiteral("Default screenshot storage location"), field);
        area.setWidget(page);
        area.resize(680, 320);
        area.show();
        QVERIFY(QTest::qWaitForWindowExposed(&area));
        // 2. 缩窄视区后标签位于输入项上方，无横向滚动
        area.resize(340, 320);
        auto *label = qobject_cast<QLabel *>(form->labelForField(field));
        QTRY_VERIFY(field->geometry().top() > label->geometry().bottom());
        QTRY_COMPARE(area.horizontalScrollBar()->maximum(), 0);
        QCOMPARE(field->text(), QStringLiteral("retained value"));
    }

    /// @brief 验证长开关文字随视区换行，且切换尺寸不改变选择状态
    /// @return 无返回值
    void wrapsSwitchLabelsWithoutLosingState()
    {
        // 1. 创建完整说明与可访问名称一致的开关
        const QString caption = QStringLiteral("Keep the original screenshot while translating selected image text");
        QScrollArea area;
        area.setWidgetResizable(true);
        auto *page = new QWidget;
        auto *form = markshot::settings::createSettingsFormLayout(new QVBoxLayout(page));
        auto *box = new QCheckBox(caption, page);
        box->setAccessibleName(caption);
        box->setProperty("settingsSwitchLabel", caption);
        form->addRow(box);
        area.setWidget(page);
        area.resize(340, 320);
        area.show();
        QVERIFY(QTest::qWaitForWindowExposed(&area));
        // 2. 核对长文字换行且没有撑出横向滚动
        QTRY_VERIFY(box->text().contains(QLatin1Char('\n')));
        QTRY_COMPARE(area.horizontalScrollBar()->maximum(), 0);
        QCOMPARE(box->accessibleName(), caption);
        box->setChecked(true);
        // 3. 【设置测试】【换行恢复】按实际字体留足行宽，兼容无字体环境的替代字形
        area.resize(qMax(760, box->fontMetrics().horizontalAdvance(caption) + 120), 320);
        QTRY_COMPARE(box->text(), caption);
        QVERIFY(box->isChecked());
    }
};

QTEST_MAIN(SettingsFormLayoutTest)
#include "settings_form_layout_test.moc"
