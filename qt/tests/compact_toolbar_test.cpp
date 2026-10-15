#include "ui/compact_toolbar.h"

#include <QPushButton>
#include <QtTest/QtTest>

namespace {

/// @brief 创建测试用按钮描述
/// @param key 按钮动作键
/// @return 按钮描述
markshot::ui::ToolbarButtonSpec spec(const char *key)
{
    markshot::ui::ToolbarButtonSpec result;
    result.actionKey = QString::fromLatin1(key);
    result.tooltip = result.actionKey;
    return result;
}

/// @brief 统计工具栏中显式显示的分隔线数量
/// @param toolbar 待检查工具栏
/// @return 可见分隔线数量
int visibleSeparatorCount(const markshot::ui::CompactToolbar &toolbar)
{
    int count = 0;
    for (const QWidget *child : toolbar.findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (child != toolbar.grip() && !qobject_cast<const QPushButton *>(child) && !child->isHidden()) {
            ++count;
        }
    }
    return count;
}

}

class CompactToolbarTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证按钮属性、分组与分隔线随分组显隐同步
    /// @return 无返回值
    void groupsControlSeparators()
    {
        markshot::ui::CompactToolbar toolbar(QStringLiteral("shotToolbar"), QStringLiteral("toolbarGrip"),
                                             Qt::Horizontal, {20, 6, 3});
        toolbar.addGroup();
        toolbar.addButton(spec("Pen"));
        toolbar.addButton(spec("Line"));
        toolbar.addGroup();
        auto save = spec("Save As");
        save.role = markshot::ui::ToolbarButtonRole::Primary;
        QPushButton *saveButton = toolbar.addButton(save);
        const int outputGroup = toolbar.groupCount() - 1;
        toolbar.show();

        QCOMPARE(toolbar.groupCount(), 2);
        QCOMPARE(toolbar.buttons().size(), 3);
        QCOMPARE(toolbar.button(QStringLiteral("Save As")), saveButton);
        QCOMPARE(saveButton->property("role").toString(), QStringLiteral("primary"));
        // 复制使用次级强调角色，单独工具栏验证，避免影响本例的按钮计数
        markshot::ui::CompactToolbar accentToolbar(QStringLiteral("actionToolbar"), QStringLiteral("actionToolbarGrip"),
                                                   Qt::Vertical, {20, 4, 2});
        auto copy = spec("Copy");
        copy.role = markshot::ui::ToolbarButtonRole::Accent;
        QCOMPARE(accentToolbar.addButton(copy)->property("role").toString(), QStringLiteral("accent"));
        QCOMPARE(saveButton->iconSize(), QSize(20, 20));
        QCOMPARE(saveButton->focusPolicy(), Qt::NoFocus);
        QCOMPARE(visibleSeparatorCount(toolbar), 1);

        // 隐藏分组后分隔线随之隐藏，重新显示后恢复
        toolbar.setGroupVisible(outputGroup, false);
        QVERIFY(!toolbar.isGroupVisible(outputGroup));
        QCOMPARE(visibleSeparatorCount(toolbar), 0);
        toolbar.setGroupVisible(outputGroup, true);
        QCOMPARE(visibleSeparatorCount(toolbar), 1);

        // 单独隐藏首组全部按钮时，后续分组前不再出现孤立分隔线
        toolbar.button(QStringLiteral("Pen"))->hide();
        toolbar.button(QStringLiteral("Line"))->hide();
        QCOMPARE(visibleSeparatorCount(toolbar), 0);
    }

    /// @brief 验证统一选中态只更新状态变化的按钮
    /// @return 无返回值
    void appliesActiveState()
    {
        markshot::ui::CompactToolbar toolbar(QStringLiteral("shotToolbar"), QStringLiteral("toolbarGrip"),
                                             Qt::Horizontal, {20, 6, 3});
        QPushButton *pen = toolbar.addButton(spec("Pen"));
        QPushButton *line = toolbar.addButton(spec("Line"));
        toolbar.setActiveActions({QStringLiteral("Pen")});
        QVERIFY(pen->property("active").toBool());
        QVERIFY(!line->property("active").toBool());
        toolbar.setActiveActions({QStringLiteral("Line")});
        QVERIFY(!pen->property("active").toBool());
        QVERIFY(line->property("active").toBool());
    }

    /// @brief 验证横纵切换改变排列方向与握柄形态，并保留拖拽标记
    /// @return 无返回值
    void switchesOrientation()
    {
        markshot::ui::CompactToolbar toolbar(QStringLiteral("shotToolbar"), QStringLiteral("toolbarGrip"),
                                             Qt::Horizontal, {20, 6, 3});
        for (const char *key : {"Pen", "Line", "Text", "Number"}) {
            toolbar.addButton(spec(key));
        }
        toolbar.show();
        QWidget *grip = toolbar.grip();
        QVERIFY(grip);
        QCOMPARE(grip->objectName(), QStringLiteral("toolbarGrip"));
        QVERIFY(grip->property("dragHandle").toBool());
        QCOMPARE(grip->cursor().shape(), Qt::OpenHandCursor);

        const QSize horizontal = toolbar.sizeHint();
        QVERIFY(horizontal.width() > horizontal.height());
        QCOMPARE(grip->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);

        toolbar.setOrientation(Qt::Vertical);
        QCOMPARE(toolbar.orientation(), Qt::Vertical);
        const QSize vertical = toolbar.sizeHint();
        QVERIFY(vertical.height() > vertical.width());
        QCOMPARE(grip->sizePolicy().verticalPolicy(), QSizePolicy::Fixed);

        // 纵向排列时按钮自上而下排列
        const QVector<QPushButton *> buttons = toolbar.buttons();
        toolbar.resize(vertical);
        QCoreApplication::processEvents();
        QVERIFY(buttons.at(1)->geometry().top() > buttons.at(0)->geometry().top());
        QCOMPARE(buttons.at(1)->geometry().left(), buttons.at(0)->geometry().left());
    }
};

QTEST_MAIN(CompactToolbarTest)
#include "compact_toolbar_test.moc"
