#pragma once

#include <QVector>

class QPainter;

namespace markshot::ui {

struct ContextMenuEntry;
struct ContextMenuLayout;
struct ContextMenuStyle;

/**
 * 绘制上下文菜单：阴影、圆角面板、菜单行、图标、勾选标记、快捷键列与子菜单箭头。
 * @param painter 已设置菜单字体的画笔。
 * @param layout 菜单布局。
 * @param entries 菜单行快照。
 * @param style 菜单令牌。
 * @param activeIndex 当前高亮行下标，-1 表示无高亮。
 * @return 无返回值。
 */
void paintContextMenu(QPainter &painter,
                      const ContextMenuLayout &layout,
                      const QVector<ContextMenuEntry> &entries,
                      const ContextMenuStyle &style,
                      int activeIndex);

}  // namespace markshot::ui
