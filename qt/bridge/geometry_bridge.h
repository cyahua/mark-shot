#pragma once
#include "shot_ffi.h"
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
namespace markshot::rust
{
/** 将 Qt rect 转为整数 ABI 矩形，返回值不包含 Qt 对象。 */
inline ShotRect toCore(QRect rect) { return {rect.x(), rect.y(), rect.width(), rect.height()}; }
/** 将 Qt rect 转为浮点 ABI 矩形，返回值不包含 Qt 对象。 */
inline ShotRectF toCore(QRectF rect) { return {rect.x(), rect.y(), rect.width(), rect.height()}; }
/** 将 Qt point 转为 ABI 坐标，返回值不包含 Qt 对象。 */
inline ShotPoint toCore(QPointF point) { return {point.x(), point.y()}; }
/** 将 Qt size 转为 ABI 尺寸，返回值不包含 Qt 对象。 */
inline ShotSize toCore(QSize size) { return {size.width(), size.height()}; }
/** 将 ABI rect 转回 Qt 整数矩形，返回可用于裁剪的范围。 */
inline QRect fromCore(ShotRect rect) { return {rect.x, rect.y, rect.width, rect.height}; }
/** 将 ABI rect 转回 Qt 浮点矩形，返回可用于界面的范围。 */
inline QRectF fromCore(ShotRectF rect) { return {rect.x, rect.y, rect.width, rect.height}; }
/** 将 ABI point 转回 Qt 坐标，返回绘制坐标。 */
inline QPointF fromCore(ShotPoint point) { return {point.x, point.y}; }
} // namespace markshot::rust
