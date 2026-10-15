#include "ui/transparency_background.h"

#include <QPainter>
#include <QPixmap>

namespace markshot::ui {

void paintTransparencyBackground(QPainter &painter, const QRect &rect)
{
    // 1. 缓存一个完整周期的灰色棋盘格，避免逐格重绘大画布
    constexpr int cellSize = 12;
    static const QPixmap tile = [] {
        QPixmap result(cellSize * 2, cellSize * 2);
        result.fill(QColor(51, 55, 62));
        QPainter tilePainter(&result);
        tilePainter.fillRect(QRect(0, 0, cellSize, cellSize), QColor(60, 64, 72));
        tilePainter.fillRect(QRect(cellSize, cellSize, cellSize, cellSize), QColor(60, 64, 72));
        return result;
    }();
    // 2. 网格锚定视图原点，不随图片缩放改变大小
    painter.save();
    painter.drawTiledPixmap(rect, tile, rect.topLeft());
    painter.restore();
}

}  // namespace markshot::ui
