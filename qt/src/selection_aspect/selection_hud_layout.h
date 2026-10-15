#pragma once

#include <QRect>
#include <QSize>
#include <QVector>

#include <optional>

namespace markshot::selection_aspect {

/**
 * 选区尺寸 HUD 的定位输入，全部使用截图窗口坐标。
 */
struct SelectionHudLayoutInput {
    // 选区矩形
    QRect selection;
    // HUD 尺寸
    QSize hudSize;
    // 可用区域，通常为截图窗口矩形
    QRect bounds;
    // 需要避让的控件矩形，例如标注文本输入框和工具栏
    QVector<QRect> obstacles;
    // 与选区及可用区域边缘保持的间距
    int gap = 6;
};

/**
 * 计算 HUD 位置：优先放在选区左上角外侧，空间不足时依次尝试选区左上角内侧、选区左下角外侧，
 * 并把横坐标限制在可用区域内，保证 HUD 跟随左上角且不贴出屏幕边缘。
 * @param input 定位输入。
 * @return 第一个完整位于可用区域且不与避让矩形相交的位置；全部冲突时返回 std::nullopt。
 */
std::optional<QRect> placeSelectionHud(const SelectionHudLayoutInput &input);

}  // namespace markshot::selection_aspect
