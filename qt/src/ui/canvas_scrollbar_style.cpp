#include "ui/canvas_scrollbar_style.h"

namespace markshot::ui {

QString canvasScrollBarStyleSheet(int extent)
{
    // 1. 保留可点击区域，用内边距把视觉滑块缩窄，轨道与箭头保持透明
    return QStringLiteral(
        "QScrollBar { background: transparent; border: none; }"
        "QScrollBar:horizontal { height: %1px; margin: 3px 2px; }"
        "QScrollBar:vertical { width: %1px; margin: 2px 3px; }"
        "QScrollBar::handle { background: rgba(195, 202, 213, 120); border-radius: 4px; }"
        "QScrollBar::handle:horizontal { min-width: 36px; }"
        "QScrollBar::handle:vertical { min-height: 36px; }"
        "QScrollBar::handle:hover { background: rgba(213, 219, 228, 180); }"
        "QScrollBar::handle:pressed { background: rgba(94, 234, 212, 210); }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; background: transparent; border: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }")
        .arg(extent);
}

}  // namespace markshot::ui
