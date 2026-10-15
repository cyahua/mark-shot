#pragma once

#include <QRect>

class QPainter;

namespace markshot::ui {

/**
 * 绘制低对比度透明棋盘格，仅用于编辑视图，不参与图片导出
 * @param painter 目标画笔
 * @param rect 视图中需要填充的区域
 * @return 无返回值
 */
void paintTransparencyBackground(QPainter &painter, const QRect &rect);

}  // namespace markshot::ui
