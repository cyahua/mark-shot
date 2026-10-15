#pragma once

#include <QColor>

namespace markshot::ui {

/**
 * 自绘上下文菜单的尺寸与颜色令牌。
 * 尺寸单位为设备无关像素，绘制时由 Qt 按屏幕缩放比自动换算。
 */
struct ContextMenuStyle {
    // 尺寸令牌
    int itemHeight = 28;
    int separatorHeight = 9;
    int panelPadding = 4;
    int itemHorizontalPadding = 8;
    int iconSize = 16;
    int iconTextGap = 8;
    int shortcutGap = 24;
    int arrowWidth = 12;
    int panelRadius = 8;
    int itemRadius = 5;
    int shadowMargin = 0;
    int minimumWidth = 168;
    int maximumWidth = 420;
    int submenuOverlap = 2;

    // 颜色令牌
    QColor background;
    QColor border;
    QColor text;
    QColor secondaryText;
    QColor disabledText;
    QColor hoverBackground;
    QColor hoverText;
    QColor dangerText;
    QColor dangerHoverBackground;
    QColor dangerHoverText;
    QColor separator;
    QColor accent;
    QColor checkedBackground;
    QColor shadow = Qt::transparent;
};

/**
 * 根据明暗主题生成上下文菜单令牌。
 * @param dark 为 true 时返回暗色令牌，否则返回亮色令牌。
 * @return 完整的上下文菜单令牌。
 */
ContextMenuStyle contextMenuStyle(bool dark);

}  // namespace markshot::ui
