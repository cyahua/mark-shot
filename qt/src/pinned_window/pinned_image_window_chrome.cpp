#include "pinned_window/pinned_image_window.h"

#include "pinned_window/pinned_non_window_chrome.h"

#include <QEvent>
#include <QPainter>
#include <QPen>

namespace markshot::shot {

void PinnedImageWindow::applyPinnedNonWindowChromeIfEnabled()
{
    if (!FEATURE_PINNED_NON_WINDOW_CHROME_ENABLED) {
        return;
    }
    // 1. 只避免激活。Qt::Tool 会变成截图覆盖层的附属窗口，覆盖层关闭时贴图一起消失
    setAttribute(Qt::WA_ShowWithoutActivating, true);
}

void PinnedImageWindow::updatePinnedNonWindowHoverIfEnabled(QEvent *event)
{
    if (!FEATURE_PINNED_NON_WINDOW_CHROME_ENABLED || !event) {
        return;
    }
    if (event->type() != QEvent::Enter && event->type() != QEvent::Leave) {
        return;
    }
    const bool hovered = event->type() == QEvent::Enter;
    if (m_pinChromeHovered == hovered) {
        return;
    }
    m_pinChromeHovered = hovered;
    update();
}

bool PinnedImageWindow::paintPinnedNonWindowChromeIfEnabled(QPainter &painter, const QRectF &imageRect)
{
    if (!FEATURE_PINNED_NON_WINDOW_CHROME_ENABLED || imageRect.isEmpty()) {
        return false;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);
    const QColor shadow = m_pinChromeHovered ? QColor(15, 23, 42, 120) : QColor(15, 23, 42, 48);
    const qreal shadowWidth = m_pinChromeHovered ? 6.0 : 3.0;
    painter.setPen(QPen(shadow, shadowWidth));
    const qreal shadowInset = shadowWidth / 2.0;
    painter.drawRect(imageRect.adjusted(shadowInset, shadowInset, -shadowInset, -shadowInset));
    const QColor border = m_pinChromeHovered ? QColor(37, 99, 235) : QColor(148, 156, 172, 170);
    const qreal borderWidth = m_pinChromeHovered ? 2.0 : 1.0;
    painter.setPen(QPen(border, borderWidth));
    const qreal borderInset = borderWidth / 2.0;
    painter.drawRect(imageRect.adjusted(borderInset, borderInset, -borderInset, -borderInset));
    painter.restore();
    return true;
}

}  // namespace markshot::shot
