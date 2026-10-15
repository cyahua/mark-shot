#include "ui/context_menu_style.h"

namespace markshot::ui {

ContextMenuStyle contextMenuStyle(bool dark)
{
    ContextMenuStyle style;
    if (dark) {
        // 1. 暗色主题沿用截图浮层的深色玻璃与青色强调
        style.background = QColor(24, 27, 35, 246);
        style.border = QColor(255, 255, 255, 30);
        style.text = QColor(0xE5, 0xE7, 0xEB);
        style.secondaryText = QColor(0x94, 0xA3, 0xB8);
        style.disabledText = QColor(0x64, 0x74, 0x8B);
        style.hoverBackground = QColor(45, 212, 191, 46);
        style.hoverText = QColor(0xF8, 0xFA, 0xFC);
        style.dangerText = QColor(0xF8, 0x71, 0x71);
        style.dangerHoverBackground = QColor(239, 68, 68, 52);
        style.dangerHoverText = QColor(0xFC, 0xA5, 0xA5);
        style.separator = QColor(255, 255, 255, 22);
        style.accent = QColor(0x5E, 0xEA, 0xD4);
        style.checkedBackground = QColor(45, 212, 191, 36);
        return style;
    }

    // 2. 亮色主题使用白色浮层、浅灰悬停与深青色强调
    style.background = QColor(255, 255, 255, 250);
    style.border = QColor(15, 23, 42, 26);
    style.text = QColor(0x1F, 0x29, 0x37);
    style.secondaryText = QColor(0x64, 0x74, 0x8B);
    style.disabledText = QColor(0xA0, 0xAE, 0xC0);
    style.hoverBackground = QColor(15, 23, 42, 14);
    style.hoverText = QColor(0x0F, 0x17, 0x2A);
    style.dangerText = QColor(0xDC, 0x26, 0x26);
    style.dangerHoverBackground = QColor(254, 226, 226);
    style.dangerHoverText = QColor(0xB9, 0x1C, 0x1C);
    style.separator = QColor(15, 23, 42, 20);
    style.accent = QColor(0x0D, 0x94, 0x88);
    style.checkedBackground = QColor(13, 148, 136, 30);
    return style;
}

}  // namespace markshot::ui
