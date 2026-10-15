#include "ui/context_menu.h"
#include "ui/context_menu_model.h"
#include "ui/context_menu_native_bridge.h"
#include "ui/context_menu_placement.h"

#include <QMenu>
#include <QSignalSpy>

#include <memory>
#include <QtTest/QtTest>

using markshot::ui::ContextMenu;

class ContextMenuTest final : public QObject {
    Q_OBJECT
private slots:
    /** @brief 验证助记符转义与快捷键列。@return 无返回值。 */
    void textAndShortcut()
    {
        QCOMPARE(markshot::ui::contextMenuDisplayText(QStringLiteral("&Save && Close\tCtrl+S")),
                 QStringLiteral("Save & Close"));
        QCOMPARE(markshot::ui::contextMenuMnemonic(QStringLiteral("&& test &Save")), QChar('s'));
        ContextMenu menu;
        QAction *action = menu.addAction(QStringLiteral("Copy"));
        action->setShortcut(QKeySequence::Copy);
        action->setShortcutVisibleInContextMenu(true);
        QVERIFY(!markshot::ui::contextMenuShortcutText(action).isEmpty());
        action->setText(QStringLiteral("Copy\tCustom"));
        QCOMPARE(markshot::ui::contextMenuShortcutText(action), QStringLiteral("Custom"));
    }

    /** @brief 验证首尾和连续分隔线折叠，以及危险动作快照。@return 无返回值。 */
    void actionModel()
    {
        ContextMenu menu;
        menu.addSeparator();
        QAction *first = menu.addAction(QStringLiteral("First"));
        menu.addSeparator();
        menu.addSeparator();
        QAction *danger = menu.addAction(QStringLiteral("Delete"));
        menu.setActionDanger(danger);
        menu.addSeparator();
        QCOMPARE(markshot::ui::contextMenuVisibleActions(menu.actions()).size(), 3);
        QVERIFY(markshot::ui::contextMenuEntryFromAction(danger, false).danger);
        first->setVisible(false);
        QCOMPARE(markshot::ui::contextMenuVisibleActions(menu.actions()), QList<QAction *>{danger});
    }

    /** @brief 验证快捷键列、图标列对齐与面板翻转。@return 无返回值。 */
    void layoutAndPlacement()
    {
        ContextMenu menu;
        QAction *first = menu.addAction(QStringLiteral("First"));
        first->setCheckable(true);
        QAction *second = menu.addAction(QStringLiteral("Second\tCtrl+Shift+S"));
        QVector<markshot::ui::ContextMenuEntry> entries{
            markshot::ui::contextMenuEntryFromAction(first, false),
            markshot::ui::contextMenuEntryFromAction(second, false)};
        const auto style = markshot::ui::contextMenuStyle(true);
        const auto layout = markshot::ui::layoutContextMenu(entries, style, QFontMetrics(menu.font()));
        QCOMPARE(layout.itemRects.size(), 2);
        QCOMPARE(layout.widgetSize, layout.panelRect.size());
        QVERIFY(layout.textX > layout.itemRects.first().left() + style.iconSize);
        QCOMPARE(layout.itemRects.first().width(), layout.itemRects.last().width());
        const QRect bounds(0, 0, 640, 480);
        const QRect placed = markshot::ui::placeContextMenuAtPoint(QPoint(630, 470), QSize(180, 100), bounds);
        QVERIFY(bounds.contains(placed));
        QVERIFY(placed.left() < 630);
        QVERIFY(placed.top() < 470);
    }

    /** @brief 验证键盘跳过禁用项、循环导航及勾选动作交付。@return 无返回值。 */
    void keyboardAndDelivery()
    {
        ContextMenu menu;
        QAction *disabled = menu.addAction(QStringLiteral("Disabled"));
        disabled->setEnabled(false);
        menu.addSeparator();
        QAction *check = menu.addAction(QStringLiteral("Check"));
        check->setCheckable(true);
        QAction *last = menu.addAction(QStringLiteral("Last"));
        QSignalSpy triggered(&menu, &ContextMenu::triggered);
        bool hiddenDuringDelivery = false;
        connect(check, &QAction::triggered, &menu, [&] { hiddenDuringDelivery = !menu.isVisible(); });
        menu.popup(QPoint(100, 100));
        QVERIFY(menu.isVisible());
        QTest::keyClick(&menu, Qt::Key_Down);
        QCOMPARE(menu.activeAction(), check);
        QTest::keyClick(&menu, Qt::Key_Down);
        QCOMPARE(menu.activeAction(), last);
        QTest::keyClick(&menu, Qt::Key_Down);
        QCOMPARE(menu.activeAction(), check);
        QTest::keyClick(&menu, Qt::Key_Return);
        QVERIFY(!menu.isVisible());
        QVERIFY(check->isChecked());
        QVERIFY(hiddenDuringDelivery);
        QCOMPARE(triggered.size(), 1);
    }

    /** @brief 验证子菜单键盘打开、退回与整棵菜单动作交付。@return 无返回值。 */
    void submenuNavigation()
    {
        ContextMenu menu;
        ContextMenu *child = menu.addMenu(QStringLiteral("Child"));
        QAction *action = child->addAction(QStringLiteral("Action"));
        QSignalSpy triggered(&menu, &ContextMenu::triggered);
        menu.popup(QPoint(80, 80));
        QTest::keyClick(&menu, Qt::Key_Down);
        QTest::keyClick(&menu, Qt::Key_Right);
        QVERIFY(child->isVisible());
        QCOMPARE(child->activeAction(), action);
        QTest::keyClick(child, Qt::Key_Left);
        QVERIFY(!child->isVisible());
        QVERIFY(menu.isVisible());
        QTest::keyClick(&menu, Qt::Key_Right);
        QTest::keyClick(child, Qt::Key_Return);
        QVERIFY(!menu.isVisible());
        QVERIFY(!child->isVisible());
        QCOMPARE(triggered.size(), 1);
    }

    /** @brief 验证同步会话拒绝重入并在关闭时退出。@return 无返回值。 */
    void execReentry()
    {
        ContextMenu menu;
        menu.addAction(QStringLiteral("Action"));
        QTimer::singleShot(0, &menu, [&] {
            QVERIFY(menu.isVisible());
            QCOMPARE(menu.exec(QPoint(0, 0)), nullptr);
            menu.dismiss();
        });
        QCOMPARE(menu.exec(QPoint(100, 100)), nullptr);
    }

    /** @brief 验证点击面板外区域关闭整棵菜单且不触发动作。@return 无返回值。 */
    void outsideClickCloses()
    {
        ContextMenu menu;
        QAction *action = menu.addAction(QStringLiteral("Action"));
        QSignalSpy triggered(action, &QAction::triggered);
        QSignalSpy finished(&menu, &ContextMenu::popupFinished);
        menu.popup(QPoint(100, 100));
        QTest::mouseClick(&menu, Qt::LeftButton, {}, QPoint(-1, -1));
        QVERIFY(!menu.isVisible());
        QCOMPARE(triggered.size(), 0);
        QCOMPARE(finished.size(), 1);
    }

    /** @brief 验证原生镜像共享 QAction 并把 aboutToShow 转发回源菜单。@return 无返回值。 */
    void nativeMirror()
    {
        ContextMenu menu;
        QAction *action = menu.addAction(QStringLiteral("Action"));
        ContextMenu *child = menu.addMenu(QStringLiteral("Child"));
        child->addAction(QStringLiteral("Nested"));
        std::unique_ptr<QMenu> mirror(markshot::ui::createNativeMenuMirror(&menu));
        QCOMPARE(mirror->actions().first(), action);
        QVERIFY(mirror->actions().last()->menu());
        QSignalSpy shown(child, &ContextMenu::aboutToShow);
        emit mirror->actions().last()->menu()->aboutToShow();
        QCOMPARE(shown.size(), 1);
    }

    /** @brief 验证动作回调中删除菜单不会访问已销毁的状态。@return 无返回值。 */
    void deletionDuringDelivery()
    {
        // 模拟“关闭贴图窗口”：动作由外部持有，回调中同步销毁菜单所属对象
        auto *owner = new QWidget;
        QPointer<ContextMenu> menu = new ContextMenu(owner);
        QAction action(QStringLiteral("Delete owner"));
        menu->addAction(&action);
        connect(&action, &QAction::triggered, qApp, [owner] { delete owner; });
        menu->popup(QPoint(100, 100));
        menu->setActiveAction(&action);
        QTest::keyClick(menu, Qt::Key_Return);
        QVERIFY(!menu);
    }
};

QTEST_MAIN(ContextMenuTest)
#include "context_menu_test.moc"
