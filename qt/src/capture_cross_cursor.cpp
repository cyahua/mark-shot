#include "capture_cross_cursor.h"

#include <QHash>
#include <QPainter>
#include <QPixmap>

#include <algorithm>
#include <cmath>

namespace markshot::shot {
namespace {

constexpr int kLogicalCanvasSize = 64;
constexpr int kArgbBytesPerPixel = 4;
constexpr int kRequiredRowAlignment = 256;
constexpr int kCanvasPixelAlignment = kRequiredRowAlignment / kArgbBytesPerPixel;

int cursorResourceScale(qreal devicePixelRatio)
{
    return std::max(1, static_cast<int>(std::ceil(devicePixelRatio)));
}

QCursor createCaptureCrossCursor(int resourceScale)
{
    const int canvasSize = kLogicalCanvasSize * resourceScale;
    static_assert(kLogicalCanvasSize % kCanvasPixelAlignment == 0);
    const int logicalCenter = kLogicalCanvasSize / 2;

    QPixmap pixmap(canvasSize, canvasSize);
    pixmap.setDevicePixelRatio(resourceScale);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    drawCaptureCrossCursor(painter, QPointF(logicalCenter, logicalCenter));
    painter.end();
    return QCursor(pixmap, logicalCenter, logicalCenter);
}

}

void drawCaptureCrossCursor(QPainter &painter, QPointF position)
{
    // 1. 【标注】【十字光标】热点对齐像素中心，四臂留出中心取样区域
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(QPointF(position.toPoint()) + QPointF(0.5, 0.5));
    const QLineF arms[] = {
        QLineF(-9, 0, -3, 0), QLineF(3, 0, 9, 0),
        QLineF(0, -9, 0, -3), QLineF(0, 3, 0, 9),
    };
    // 2. 【标注】【十字光标】浅色细线与深色轮廓在明暗背景上保持一致，不依赖标注颜色
    painter.setPen(QPen(QColor(17, 24, 39, 235), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLines(arms, 4);
    painter.setPen(QPen(QColor(243, 244, 246), 1, Qt::SolidLine, Qt::RoundCap));
    painter.drawLines(arms, 4);
    painter.restore();
}

QRect captureCrossCursorRect(QPointF position)
{
    return QRect(position.toPoint() - QPoint(12, 12), QSize(25, 25));
}

QCursor captureCrossCursor(qreal devicePixelRatio)
{
    // 1. 【标注】【十字光标】按整数资源倍率缓存，兼顾高分屏清晰度与 256 字节行步长
    static QHash<int, QCursor> cursors;
    const int resourceScale = cursorResourceScale(devicePixelRatio);
    const auto existing = cursors.constFind(resourceScale);
    if (existing != cursors.cend()) {
        return *existing;
    }
    return cursors.insert(resourceScale, createCaptureCrossCursor(resourceScale)).value();
}

}
