#include "ui/disclosure_section.h"

#include <QLineEdit>
#include <QSignalSpy>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtTest/QtTest>

class DisclosureSectionTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证用户在低频配置中输入的值不会因折叠丢失，也不会重复触发变更
    /// @return 无返回值
    void retainsEditedValueAcrossCollapse()
    {
        markshot::ui::DisclosureSection section(QStringLiteral("Advanced"));
        auto *layout = new QVBoxLayout(section.content());
        auto *edit = new QLineEdit(section.content());
        layout->addWidget(edit);
        section.show();
        auto *toggle = section.findChild<QToolButton *>();
        QVERIFY(toggle);
        QVERIFY(!section.isExpanded());
        QTest::mouseClick(toggle, Qt::LeftButton);
        QVERIFY(section.isExpanded());
        QTest::keyClicks(edit, "saved setting");
        QSignalSpy changed(edit, &QLineEdit::textChanged);
        QTest::mouseClick(toggle, Qt::LeftButton);
        QVERIFY(!section.isExpanded());
        QTest::mouseClick(toggle, Qt::LeftButton);
        QCOMPARE(edit->text(), QStringLiteral("saved setting"));
        QVERIFY(changed.isEmpty());
    }

    /// @brief 验证程序收起正在编辑的分组后，键盘焦点回到可见的展开入口
    /// @return 无返回值
    void restoresKeyboardFocusWhenCollapsed()
    {
        markshot::ui::DisclosureSection section(QStringLiteral("Options"));
        auto *layout = new QVBoxLayout(section.content());
        auto *edit = new QLineEdit(section.content());
        layout->addWidget(edit);
        section.setExpanded(true);
        section.show();
        section.activateWindow();
        edit->setFocus();
        QTRY_VERIFY(edit->hasFocus());
        section.setExpanded(false);
        auto *toggle = section.findChild<QToolButton *>();
        QTRY_VERIFY(toggle->hasFocus());
        QTest::keyClick(toggle, Qt::Key_Space);
        QVERIFY(section.isExpanded());
    }
};

QTEST_MAIN(DisclosureSectionTest)

#include "disclosure_section_test.moc"
