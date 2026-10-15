#include "selection_aspect/selection_hud_layout.h"

#include <algorithm>

namespace markshot::selection_aspect {
namespace {

/**
 * 判断候选位置是否完整位于可用区域内且不压住任何避让矩形。
 * @param candidate 候选 HUD 矩形。
 * @param input 定位输入。
 * @return 位置可用时返回 true。
 */
bool candidateFits(const QRect &candidate, const SelectionHudLayoutInput &input)
{
    const QRect usable = input.bounds.adjusted(input.gap, input.gap, -input.gap, -input.gap);
    if (!usable.contains(candidate)) {
        return false;
    }
    return std::none_of(input.obstacles.cbegin(), input.obstacles.cend(), [&candidate](const QRect &obstacle) {
        return !obstacle.isEmpty() && candidate.intersects(obstacle);
    });
}

/**
 * 把横坐标限制在可用区域内，避免 HUD 贴出左右屏幕边缘。
 * @param x 期望横坐标。
 * @param input 定位输入。
 * @return 限制后的横坐标。
 */
int clampedX(int x, const SelectionHudLayoutInput &input)
{
    const int minX = input.bounds.left() + input.gap;
    const int maxX = input.bounds.left() + input.bounds.width() - input.gap - input.hudSize.width();
    return std::clamp(x, minX, std::max(minX, maxX));
}

}  // namespace

std::optional<QRect> placeSelectionHud(const SelectionHudLayoutInput &input)
{
    if (input.hudSize.isEmpty() || input.selection.isEmpty()) {
        return std::nullopt;
    }
    const QSize size = input.hudSize;
    const QRect &selection = input.selection;
    const int selectionBottom = selection.top() + selection.height();

    // 1. 左上角外侧、左上角内侧、左下角外侧依次尝试
    const QRect candidates[] = {
        QRect(QPoint(clampedX(selection.left(), input), selection.top() - input.gap - size.height()), size),
        QRect(QPoint(clampedX(selection.left() + input.gap, input), selection.top() + input.gap), size),
        QRect(QPoint(clampedX(selection.left(), input), selectionBottom + input.gap), size),
    };

    // 2. 返回第一个可用位置，全部冲突时由调用方决定隐藏或放宽避让条件
    for (const QRect &candidate : candidates) {
        if (candidateFits(candidate, input)) {
            return candidate;
        }
    }
    return std::nullopt;
}

}  // namespace markshot::selection_aspect
