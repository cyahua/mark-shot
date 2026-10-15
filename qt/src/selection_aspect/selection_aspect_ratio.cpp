#include "selection_aspect/selection_aspect_ratio.h"
#include "geometry_bridge.h"

#include <algorithm>
#include <cmath>

namespace markshot::selection_aspect
{
namespace
{
// 判断两个比例相同的容差
constexpr qreal kRatioEpsilon = 1e-6;
} // namespace

/** 返回界面使用的比例标签与数值，无参数。 */
QVector<AspectRatioPreset> aspectRatioPresets()
{
    return {{QStringLiteral("Free"), 0.0},        {QStringLiteral("1:1"), 1.0},
            {QStringLiteral("4:3"), 4.0 / 3.0},   {QStringLiteral("16:9"), 16.0 / 9.0},
            {QStringLiteral("9:16"), 9.0 / 16.0}, {QStringLiteral("21:9"), 21.0 / 9.0}};
}
/** 按 ratio 查找预设下标，自由比例返回 0，未命中的自定义比例返回 -1。 */
int presetIndexForRatio(qreal ratio)
{
    if (ratio <= 0.0) {
        return 0;
    }
    const QVector<AspectRatioPreset> presets = aspectRatioPresets();
    for (int i = 1; i < presets.size(); ++i) {
        if (std::abs(presets.at(i).ratio - ratio) < kRatioEpsilon) {
            return i;
        }
    }
    return -1;
}
/** 将 anchor、pointer、ratio 和 imageSize 交给 Rust，返回比例选区。 */
QRectF constrainedCreationRect(QPointF anchor, QPointF pointer, qreal ratio, QSize imageSize)
{
    return rust::fromCore(
        shot_selection_create(rust::toCore(anchor), rust::toCore(pointer), ratio, rust::toCore(imageSize)));
}
/** 将 before、handle、adjusted、ratio 和 imageSize 交给 Rust，返回调整后的选区。 */
QRectF constrainedAdjustedRect(QRectF before, markshot::shot::types::SelectionDrag handle, QRectF adjusted,
                               qreal ratio, QSize imageSize)
{
    return rust::fromCore(shot_selection_adjust(rust::toCore(before), static_cast<int>(handle),
                                                rust::toCore(adjusted), ratio, rust::toCore(imageSize)));
}
/** 将 current、size 和 imageSize 交给 Rust，返回精确尺寸选区。 */
QRectF resizedSelection(QRectF current, QSize size, QSize imageSize)
{
    return rust::fromCore(
        shot_selection_resize(rust::toCore(current), rust::toCore(size), rust::toCore(imageSize)));
}
/** 由 width 和 ratio 返回对应高度，非法比例返回 0。 */
int heightForWidth(int width, qreal ratio) { return shot_selection_side(width, ratio, 1); }
/** 由 height 和 ratio 返回对应宽度，非法比例返回 0。 */
int widthForHeight(int height, qreal ratio) { return shot_selection_side(height, ratio, 0); }
/** 按 edited 一侧和 ratio 联动另一侧，并把结果限制在 1 与 maximum 之间。 */
QSize linkedSize(QSize requested, SizeField edited, qreal ratio, QSize maximum)
{
    // 1. 先把输入限制在可用范围内
    const int maxWidth = std::max(1, maximum.width());
    const int maxHeight = std::max(1, maximum.height());
    int width = std::clamp(requested.width(), 1, maxWidth);
    int height = std::clamp(requested.height(), 1, maxHeight);
    if (ratio <= 0.0) {
        return QSize(width, height);
    }

    // 2. 按被修改的一侧推算另一侧
    if (edited == SizeField::Width) {
        height = heightForWidth(width, ratio);
    } else {
        width = widthForHeight(height, ratio);
    }

    // 3. 推算结果越界时以越界一侧为上限反推
    if (height > maxHeight) {
        height = maxHeight;
        width = widthForHeight(height, ratio);
    }
    if (width > maxWidth) {
        width = maxWidth;
        height = heightForWidth(width, ratio);
    }
    return QSize(std::clamp(width, 1, maxWidth), std::clamp(height, 1, maxHeight));
}
} // namespace markshot::selection_aspect
