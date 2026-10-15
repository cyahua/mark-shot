#include "ui/context_menu_placement.h"

#include <algorithm>

namespace markshot::ui {

QRect clampContextMenuRect(QRect rect, const QRect &bounds)
{
    if (!bounds.isValid()) {
        return rect;
    }
    // 先约束右下边界，再约束左上边界，保证超大面板仍从可见的左上角开始
    if (rect.right() > bounds.right()) {
        rect.moveRight(bounds.right());
    }
    if (rect.bottom() > bounds.bottom()) {
        rect.moveBottom(bounds.bottom());
    }
    if (rect.left() < bounds.left()) {
        rect.moveLeft(bounds.left());
    }
    if (rect.top() < bounds.top()) {
        rect.moveTop(bounds.top());
    }
    return rect;
}

QRect placeContextMenuAtPoint(const QPoint &anchor, const QSize &panelSize, const QRect &bounds)
{
    QRect rect(anchor, panelSize);
    if (bounds.isValid()) {
        // 1. 右侧放不下时翻转到锚点左侧
        if (rect.right() > bounds.right() && anchor.x() - panelSize.width() >= bounds.left()) {
            rect.moveRight(anchor.x());
        }
        // 2. 下方放不下时翻转到锚点上方
        if (rect.bottom() > bounds.bottom() && anchor.y() - panelSize.height() >= bounds.top()) {
            rect.moveBottom(anchor.y());
        }
    }
    // 3. 两侧都放不下时贴边显示
    return clampContextMenuRect(rect, bounds);
}

QRect placeContextMenuBelow(const QRect &anchorRect, const QSize &panelSize, const QRect &bounds, int gap)
{
    QRect rect(QPoint(anchorRect.left(), anchorRect.bottom() + 1 + gap), panelSize);
    if (bounds.isValid()) {
        // 1. 下方空间不足且上方足够时翻转到控件上方
        if (rect.bottom() > bounds.bottom() && anchorRect.top() - gap - panelSize.height() >= bounds.top()) {
            rect.moveBottom(anchorRect.top() - 1 - gap);
        }
        // 2. 右侧越界时与控件右边缘对齐
        if (rect.right() > bounds.right()) {
            rect.moveRight(std::max(anchorRect.right(), bounds.left() + panelSize.width() - 1));
        }
    }
    return clampContextMenuRect(rect, bounds);
}

QRect placeContextSubmenu(const QRect &parentPanel,
                          const QRect &parentItem,
                          const QSize &panelSize,
                          const QRect &bounds,
                          int overlap,
                          int itemInset)
{
    // 1. 首选父面板右侧，首行与父菜单行顶部对齐
    QRect rect(QPoint(parentPanel.right() + 1 - overlap, parentItem.top() - itemInset), panelSize);
    if (bounds.isValid() && rect.right() > bounds.right()) {
        // 2. 右侧越界时翻转到父面板左侧
        const int leftX = parentPanel.left() + overlap - panelSize.width();
        if (leftX >= bounds.left()) {
            rect.moveLeft(leftX);
        }
    }
    // 3. 垂直方向越界时上移，最终仍约束在屏幕内
    return clampContextMenuRect(rect, bounds);
}

}  // namespace markshot::ui
