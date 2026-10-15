#pragma once

#include <QCursor>
#include <QPointF>
#include <QRect>

class QPainter;

namespace markshot::shot {

/**
 * 创建截图选区与标注共用的细线十字光标
 * @param devicePixelRatio 当前输出的设备像素比
 * @return 使用对齐透明画布且热点位于留空中心的光标
 */
QCursor captureCrossCursor(qreal devicePixelRatio = 1.0);

/// @brief 绘制与系统光标一致的软件十字，保留中心像素以便精确定位
/// @param painter 目标绘制器，函数返回时恢复原绘制状态
/// @param position 十字热点在窗口内的逻辑位置
/// @return 无返回值
void drawCaptureCrossCursor(QPainter &painter, QPointF position);

/// @brief 计算十字图案需要重绘的区域
/// @param position 十字热点在窗口内的逻辑位置
/// @return 包含描边余量的逻辑矩形
QRect captureCrossCursorRect(QPointF position);

}
