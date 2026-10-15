#include "ui/context_menu_painter.h"

#include "ui/context_menu_model.h"
#include "ui/context_menu_style.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace markshot::ui {
namespace {

/**
 * 在图标列绘制勾选标记，互斥组使用圆点，其他可勾选动作使用对勾。
 * @param painter 画笔。
 * @param box 图标列矩形。
 * @param exclusive 是否属于互斥动作组。
 * @param color 标记颜色。
 * @return 无返回值。
 */
void paintCheckMark(QPainter &painter, const QRectF &box, bool exclusive, const QColor &color)
{
    painter.save();
    if (exclusive) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(box.center(), box.width() * 0.2, box.width() * 0.2);
        painter.restore();
        return;
    }
    QPen pen(color, 1.8);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(box.left() + box.width() * 0.2, box.top() + box.height() * 0.52);
    path.lineTo(box.left() + box.width() * 0.42, box.top() + box.height() * 0.72);
    path.lineTo(box.left() + box.width() * 0.8, box.top() + box.height() * 0.3);
    painter.drawPath(path);
    painter.restore();
}

/**
 * 绘制子菜单右向箭头。
 * @param painter 画笔。
 * @param box 箭头列矩形。
 * @param color 箭头颜色。
 * @return 无返回值。
 */
void paintSubmenuArrow(QPainter &painter, const QRectF &box, const QColor &color)
{
    painter.save();
    QPen pen(color, 1.5);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    const QPointF c = box.center();
    const qreal h = std::min(box.width(), box.height()) * 0.28;
    QPainterPath path;
    path.moveTo(c.x() - h * 0.5, c.y() - h);
    path.lineTo(c.x() + h * 0.5, c.y());
    path.lineTo(c.x() - h * 0.5, c.y() + h);
    painter.drawPath(path);
    painter.restore();
}

/**
 * 绘制单个可交互菜单行。
 * @param painter 画笔。
 * @param layout 菜单布局。
 * @param rect 菜单行矩形。
 * @param entry 菜单行快照。
 * @param style 菜单令牌。
 * @param active 是否高亮。
 * @return 无返回值。
 */
void paintItem(QPainter &painter,
               const ContextMenuLayout &layout,
               const QRect &rect,
               const ContextMenuEntry &entry,
               const ContextMenuStyle &style,
               bool active)
{
    // 1. 根据可用、危险与高亮状态确定配色
    QColor textColor = style.text;
    QColor shortcutColor = style.secondaryText;
    if (!entry.enabled) {
        textColor = style.disabledText;
        shortcutColor = style.disabledText;
    } else if (entry.danger) {
        textColor = active ? style.dangerHoverText : style.dangerText;
        shortcutColor = textColor;
    } else if (active) {
        textColor = style.hoverText;
    }

    // 2. 高亮背景使用圆角块，危险动作使用危险色背景
    if (active && entry.enabled) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(entry.danger ? style.dangerHoverBackground : style.hoverBackground);
        painter.drawRoundedRect(QRectF(rect), style.itemRadius, style.itemRadius);
    }

    // 3. 图标列：图标优先，勾选状态在图标下方加底色或单独绘制标记
    const QRectF iconBox(rect.left() + style.itemHorizontalPadding,
                         rect.center().y() + 1 - style.iconSize / 2.0,
                         style.iconSize,
                         style.iconSize);
    if (!entry.icon.isNull()) {
        if (entry.checkable && entry.checked) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(style.checkedBackground);
            painter.drawRoundedRect(iconBox.adjusted(-3, -3, 3, 3), 4, 4);
        }
        const QIcon::Mode mode = !entry.enabled ? QIcon::Disabled : (active ? QIcon::Active : QIcon::Normal);
        entry.icon.paint(&painter, iconBox.toRect(), Qt::AlignCenter, mode,
                         entry.checked ? QIcon::On : QIcon::Off);
    } else if (entry.checkable && entry.checked) {
        paintCheckMark(painter, iconBox, entry.exclusive, entry.enabled ? style.accent : style.disabledText);
    }

    // 4. 文案按剩余宽度省略，快捷键右对齐
    const QFontMetrics metrics = painter.fontMetrics();
    const int shortcutWidth = entry.shortcut.isEmpty() ? 0 : metrics.horizontalAdvance(entry.shortcut);
    const int textRight = layout.shortcutRight - (shortcutWidth > 0 ? shortcutWidth + style.shortcutGap / 2 : 0);
    const QRect textRect(layout.textX, rect.top(), std::max(0, textRight - layout.textX), rect.height());
    painter.setPen(textColor);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                     metrics.elidedText(entry.text, Qt::ElideRight, textRect.width()));
    if (shortcutWidth > 0) {
        painter.setPen(shortcutColor);
        const QRect shortcutRect(layout.shortcutRight - shortcutWidth, rect.top(), shortcutWidth, rect.height());
        painter.drawText(shortcutRect, Qt::AlignVCenter | Qt::AlignRight | Qt::TextSingleLine, entry.shortcut);
    }

    // 5. 子菜单箭头位于行最右侧
    if (entry.hasSubmenu) {
        const QRectF arrowBox(rect.right() + 1 - style.itemHorizontalPadding - style.arrowWidth,
                              rect.top(), style.arrowWidth, rect.height());
        paintSubmenuArrow(painter, arrowBox, entry.enabled ? style.secondaryText : style.disabledText);
    }
}

}  // namespace

void paintContextMenu(QPainter &painter,
                      const ContextMenuLayout &layout,
                      const QVector<ContextMenuEntry> &entries,
                      const ContextMenuStyle &style,
                      int activeIndex)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 1. 无阴影圆角面板，边框绘制在半像素位置保证清晰
    const QRectF panel = QRectF(layout.panelRect);
    painter.setPen(QPen(style.border, 1));
    painter.setBrush(style.background);
    painter.drawRoundedRect(panel.adjusted(0.5, 0.5, -0.5, -0.5), style.panelRadius, style.panelRadius);

    // 2. 逐行绘制，分隔线为水平细线
    const int count = std::min<int>(entries.size(), layout.itemRects.size());
    for (int i = 0; i < count; ++i) {
        const QRect rect = layout.itemRects.at(i);
        const ContextMenuEntry &entry = entries.at(i);
        if (entry.separator) {
            const qreal y = rect.center().y() + 0.5;
            painter.setPen(QPen(style.separator, 1));
            painter.drawLine(QPointF(rect.left() + style.itemHorizontalPadding / 2.0, y),
                             QPointF(rect.right() + 1 - style.itemHorizontalPadding / 2.0, y));
            continue;
        }
        paintItem(painter, layout, rect, entry, style, i == activeIndex);
    }
}

}  // namespace markshot::ui
