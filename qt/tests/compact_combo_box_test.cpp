#include "ui/compact_combo_box.h"
#include "ui/context_menu.h"

#include <QSignalSpy>
#include <QtTest>

using markshot::ui::CompactComboBox;
using markshot::ui::ContextMenu;

/// @brief 截图属性栏紧凑下拉框测试
class CompactComboBoxTest : public QObject {
    Q_OBJECT

private slots:
    /** @brief 验证宽度按最长选项固定，切换选项时不跳动；无参数，无返回值 */
    void sizeStaysStable()
    {
        CompactComboBox combo;
        combo.addItems({QStringLiteral("A"), QStringLiteral("Very long option")});
        const QSize first = combo.sizeHint();
        combo.setCurrentIndex(1);
        QCOMPARE(combo.sizeHint(), first);
        QVERIFY(first.height() >= 22);
    }

    /** @brief 验证弹出自绘菜单，当前项带勾选，选择后发出 activated；无参数，无返回值 */
    void popupSelectsItem()
    {
        QWidget host;
        CompactComboBox combo(&host);
        combo.addItems({QStringLiteral("Stroke"), QStringLiteral("Highlight"), QStringLiteral("Invert")});
        host.resize(200, 80);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));
        QSignalSpy activated(&combo, QOverload<int>::of(&QComboBox::activated));

        combo.showPopup();
        auto *menu = combo.findChild<ContextMenu *>();
        QVERIFY(menu);
        QVERIFY(menu->isVisible());
        const QList<QAction *> actions = menu->actions();
        QCOMPARE(actions.size(), 3);
        QVERIFY(actions.at(0)->isChecked());
        QVERIFY(!actions.at(0)->actionGroup());

        actions.at(2)->trigger();
        QCOMPARE(combo.currentIndex(), 2);
        QCOMPARE(activated.size(), 1);
        QCOMPARE(activated.takeFirst().at(0).toInt(), 2);

        // 再次打开时勾选状态跟随当前项
        menu->dismiss();
        combo.showPopup();
        QVERIFY(menu->actions().at(2)->isChecked());
        QVERIFY(!menu->actions().at(0)->isChecked());
        combo.hidePopup();
        QVERIFY(!menu->isVisible());
    }
};

QTEST_MAIN(CompactComboBoxTest)
#include "compact_combo_box_test.moc"
