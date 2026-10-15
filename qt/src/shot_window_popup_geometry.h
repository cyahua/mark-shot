#pragma once

#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>

#include <algorithm>

/// 【标注界面】【弹层几何】字体弹层、形状网格弹层与选区控制点的纯几何计算，便于离屏单测
namespace markshot::shot_popup {

// 弹层与锚点面板之间的间距，取 6-8px 区间
inline constexpr int kPopupGap = 7;
// 弹层与窗口边缘保持的最小留白
inline constexpr int kPopupEdgeMargin = 8;
// 选区控制点边长
inline constexpr qreal kSelectionHandleSize = 6.0;

/// @brief 字体弹层纵向分区结果，全部使用弹层内部坐标
struct FontPanelLayout {
    // 弹层整体尺寸
    QSize panelSize;
    // 字体列表区域
    QRect listRect;
    // 底部字号与 B/I/U 控件行区域
    QRect controlsRect;
};

/**
 * 计算字体弹层尺寸，并把列表与底部控件行切分为互不重叠的上下两段。
 * @param preferredWidth 期望宽度。
 * @param rowHeight 字体列表单行高度。
 * @param rowCount 字体数量。
 * @param controlsHeight 底部控件行高度（字号输入框与 B/I/U 按钮取较大值）。
 * @param available 窗口可用尺寸。
 * @param margin 弹层内边距。
 * @param spacing 列表与控件行之间的间距。
 * @return 弹层尺寸与分区矩形；列表区域位于控件行上方并留出 spacing 间距。
 */
inline FontPanelLayout fontPanelLayout(int preferredWidth,
                                       int rowHeight,
                                       int rowCount,
                                       int controlsHeight,
                                       QSize available,
                                       int margin = 6,
                                       int spacing = 6)
{
    // 1. 列表最多显示 10 行，至少保留 3 行可滚动高度
    constexpr int kMaxVisibleRows = 10;
    constexpr int kMinVisibleRows = 3;
    rowHeight = std::max(1, rowHeight);
    controlsHeight = std::max(0, controlsHeight);
    const int visibleRows = std::clamp(rowCount, 1, kMaxVisibleRows);
    const int chromeHeight = margin * 2 + spacing + controlsHeight;
    const int preferredHeight = chromeHeight + visibleRows * rowHeight;

    // 2. 宽高受窗口可用区域约束，列表高度不得低于最小行数
    const int width = std::min(preferredWidth, std::max(180, available.width() - kPopupEdgeMargin * 2));
    const int maxHeight = std::max(chromeHeight + kMinVisibleRows * rowHeight,
                                   available.height() - kPopupEdgeMargin * 2);
    const int height = std::min(preferredHeight, maxHeight);

    // 3. 控件行贴底，列表占据剩余高度
    FontPanelLayout layout;
    layout.panelSize = QSize(width, height);
    const int innerWidth = std::max(0, width - margin * 2);
    layout.controlsRect = QRect(margin, height - margin - controlsHeight, innerWidth, controlsHeight);
    const int listHeight = std::max(rowHeight, layout.controlsRect.top() - spacing - margin);
    layout.listRect = QRect(margin, margin, innerWidth, listHeight);
    return layout;
}

/**
 * 把弹层放在锚点面板下方 kPopupGap 处，下方空间不足时翻转到锚点面板上方，并限制在窗口内。
 * @param anchorRect 锚点面板矩形（通常为属性栏）。
 * @param preferredLeft 期望左边界，通常对齐触发按钮。
 * @param popupSize 弹层尺寸。
 * @param bounds 窗口可用区域。
 * @return 弹层最终矩形。
 */
inline QRect popupBelowAnchor(const QRect &anchorRect, int preferredLeft, QSize popupSize, const QRect &bounds)
{
    // 1. 优先放在锚点下方，超出底边时放到锚点上方
    int y = anchorRect.bottom() + 1 + kPopupGap;
    if (y + popupSize.height() > bounds.bottom() + 1 - kPopupEdgeMargin) {
        y = anchorRect.top() - kPopupGap - popupSize.height();
    }
    // 2. 横纵坐标都限制在窗口留白范围内
    const int minX = bounds.left() + kPopupEdgeMargin;
    const int minY = bounds.top() + kPopupEdgeMargin;
    const int x = std::clamp(preferredLeft, minX,
                             std::max(minX, bounds.right() + 1 - kPopupEdgeMargin - popupSize.width()));
    y = std::clamp(y, minY, std::max(minY, bounds.bottom() + 1 - kPopupEdgeMargin - popupSize.height()));
    return QRect(QPoint(x, y), popupSize);
}

/**
 * 计算以控制点为中心的方形绘制区域。
 * @param center 控制点中心（窗口坐标）。
 * @return 边长为 kSelectionHandleSize 的矩形。
 */
inline QRectF selectionHandleRect(QPointF center)
{
    const qreal half = kSelectionHandleSize / 2.0;
    return QRectF(center.x() - half, center.y() - half, kSelectionHandleSize, kSelectionHandleSize);
}

}  // namespace markshot::shot_popup
