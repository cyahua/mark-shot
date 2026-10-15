#pragma once

#include <QIcon>
#include <QList>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>

class QAction;
class QFontMetrics;

namespace markshot::ui {

struct ContextMenuStyle;

// QAction 动态属性名：标记危险动作，菜单以危险色绘制
inline constexpr char kContextMenuDangerProperty[] = "markshotContextMenuDanger";

/**
 * 单个菜单行的展示快照，由 QAction 状态派生，与绘制解耦以便测试。
 */
struct ContextMenuEntry {
    bool separator = false;
    bool enabled = true;
    bool checkable = false;
    bool checked = false;
    bool exclusive = false;
    bool danger = false;
    bool hasSubmenu = false;
    QIcon icon;
    QString text;
    QString shortcut;
};

/**
 * 菜单整体布局结果，所有矩形均为菜单控件本地坐标。
 */
struct ContextMenuLayout {
    QRect panelRect;
    QVector<QRect> itemRects;
    QSize widgetSize;
    int textX = 0;
    int shortcutRight = 0;
};

/**
 * 去除助记符标记，"&&" 还原为单个 "&"，并丢弃制表符之后的快捷键部分。
 * @param text QAction 原始文本。
 * @return 可直接显示的文案。
 */
QString contextMenuDisplayText(const QString &text);

/**
 * 读取 QAction 文本中的助记符字符。
 * @param text QAction 原始文本。
 * @return 小写助记符字符；无助记符时返回空字符。
 */
QChar contextMenuMnemonic(const QString &text);

/**
 * 计算菜单行右侧快捷键列文本：优先使用文本中制表符后的内容，其次使用 QAction 快捷键。
 * @param action 菜单动作。
 * @return 本地化的快捷键文本，不显示时返回空串。
 */
QString contextMenuShortcutText(const QAction *action);

/**
 * 过滤不可见动作，并折叠首尾与连续的分隔线，行为与 QMenu 默认一致。
 * @param actions 菜单持有的全部动作。
 * @return 实际需要显示的动作序列。
 */
QList<QAction *> contextMenuVisibleActions(const QList<QAction *> &actions);

/**
 * 从 QAction 构建菜单行快照。
 * @param action 菜单动作。
 * @param hasSubmenu 该动作是否挂载子菜单。
 * @return 菜单行快照。
 */
ContextMenuEntry contextMenuEntryFromAction(const QAction *action, bool hasSubmenu);

/**
 * 判断菜单行能否获得键盘或鼠标高亮。
 * @param entry 菜单行快照。
 * @return 非分隔线且可用时返回 true。
 */
bool contextMenuEntrySelectable(const ContextMenuEntry &entry);

/**
 * 计算菜单面板、各行矩形与控件尺寸。
 * @param entries 菜单行快照。
 * @param style 菜单令牌。
 * @param metrics 菜单字体度量。
 * @return 菜单布局结果。
 */
ContextMenuLayout layoutContextMenu(const QVector<ContextMenuEntry> &entries,
                                    const ContextMenuStyle &style,
                                    const QFontMetrics &metrics);

/**
 * 查找本地坐标命中的菜单行。
 * @param layout 菜单布局。
 * @param localPoint 菜单控件本地坐标。
 * @return 命中行下标；未命中时返回 -1。
 */
int contextMenuIndexAt(const ContextMenuLayout &layout, const QPoint &localPoint);

}  // namespace markshot::ui
