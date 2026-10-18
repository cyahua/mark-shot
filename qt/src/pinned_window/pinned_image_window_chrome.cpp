#include "pinned_window/pinned_image_window.h"

#include "pinned_window/pinned_non_window_chrome.h"

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
    Q_UNUSED(event);
}

bool PinnedImageWindow::paintPinnedNonWindowChromeIfEnabled(QPainter &painter, const QRectF &imageRect)
{
    Q_UNUSED(painter);
    Q_UNUSED(imageRect);
    // 边框只使用配置的颜色和粗细，不再叠加阴影或另一套描边
    return false;
}

namespace {

constexpr bool FEATURE_PINNED_SHADOW_CONFIG_ENABLED = true;
constexpr int kPinnedShadowMargin = 28;

}  // namespace

void PinnedImageWindow::applyPinnedShadowLayoutIfEnabled()
{
    if (!FEATURE_PINNED_SHADOW_CONFIG_ENABLED || !m_config.shadowEnabled || m_shadowMargin > 0) {
        return;
    }
    // 1. 【贴图】【阴影边距】向外扩展窗口，图片仍停在原来的屏幕位置
    m_shadowMargin = kPinnedShadowMargin;
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    const QRect expanded = m_logicalGeometry.adjusted(-m_shadowMargin, -m_shadowMargin,
                                                      m_shadowMargin, m_shadowMargin);
    setPinnedGeometry(expanded, true);
}

void PinnedImageWindow::insetPinnedImageForShadowIfEnabled(QRectF *rect) const
{
    if (!FEATURE_PINNED_SHADOW_CONFIG_ENABLED || !m_config.shadowEnabled || m_shadowMargin <= 0 || !rect) {
        return;
    }
    const qreal margin = m_shadowMargin;
    *rect = rect->adjusted(margin, margin, -margin, -margin);
}

void PinnedImageWindow::expandPinnedGeometryForShadowIfEnabled(QPoint *topLeft, QSize *size) const
{
    if (!FEATURE_PINNED_SHADOW_CONFIG_ENABLED || !m_config.shadowEnabled || m_shadowMargin <= 0
        || !topLeft || !size) {
        return;
    }
    *topLeft -= QPoint(m_shadowMargin, m_shadowMargin);
    *size += QSize(m_shadowMargin * 2, m_shadowMargin * 2);
}

void PinnedImageWindow::paintPinnedShadowIfEnabled(QPainter &painter, const QRectF &imageRect) const
{
    if (!FEATURE_PINNED_SHADOW_CONFIG_ENABLED || !m_config.shadowEnabled || m_shadowMargin <= 0
        || imageRect.isEmpty()) {
        return;
    }
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // 1. 【贴图】【阴影】由外向内叠若干层透明色块，形成柔和投影
    for (int layer = 16; layer >= 1; --layer) {
        QColor color(0, 0, 0, 7);
        painter.setBrush(color);
        const qreal spread = layer;
        painter.drawRoundedRect(imageRect.adjusted(-spread, -spread + 6.0, spread, spread + 8.0), 14.0, 14.0);
    }
    painter.restore();
}

}  // namespace markshot::shot
