#pragma once

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace markshot::scroll {

/// @brief 【滚动截图】【预览背景】绘制深灰背景和单像素细边框，不依赖应用明暗主题
/// @param painter 当前绘图器
/// @param rect 预览区域的逻辑坐标
/// @param rounded 是否绘制圆角面板
/// @return 无返回值
inline void drawScrollPreviewSurface(QPainter &painter, const QRect &rect, bool rounded = false)
{
    if (rect.isEmpty()) {
        return;
    }
    painter.save();
    const QRectF bounds = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(QColor(QStringLiteral("#3A3A3A")), 1));
    painter.setBrush(QColor(QStringLiteral("#1E1E1E")));
    if (rounded) {
        painter.drawRoundedRect(bounds, 10, 10);
    } else {
        painter.drawRect(bounds);
    }
    painter.restore();
}

/// @brief 【滚动截图】【预览状态】绘制静态青色状态点和白色文字，避免动画引起闲置重绘
/// @param painter 当前绘图器
/// @param rect 紧凑状态行的逻辑坐标
/// @param text 当前捕获状态，空间不足时省略尾部
/// @return 无返回值
inline void drawScrollPreviewStatus(QPainter &painter, const QRect &rect, const QString &text)
{
    if (rect.isEmpty()) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    // 1. 【滚动截图】【预览状态】状态含义由文字表达，青点只承担轻量视觉提示
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#2DD4BF")));
    painter.drawEllipse(QPointF(rect.left() + 4, rect.center().y() + 0.5), 3, 3);
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(QColor(QStringLiteral("#F5F5F5")));
    const QRect textRect = rect.adjusted(15, 0, 0, 0);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                     QFontMetrics(font).elidedText(text, Qt::ElideRight, textRect.width()));
    painter.restore();
}

}
