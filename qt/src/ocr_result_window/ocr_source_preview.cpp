#include "ocr_result_window/ocr_source_preview.h"

#include "annotation_launch.h"
#include "pinned_window/pinned_image_window.h"
#include "pinned_window/pinned_non_window_chrome.h"
#include "ui/context_menu.h"
#include "ui/context_menu_config.h"
#include "ui/i18n.h"

#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>

#include <algorithm>
#include <utility>

namespace markshot::shot {

OcrSourcePreview::OcrSourcePreview(QImage image, QWidget *parent)
    : QFrame(parent), m_image(std::move(image))
{
    setObjectName(QStringLiteral("ocrSourcePreview"));
    setAccessibleName(MS_TR("Source image"));
    setAccessibleDescription(QStringLiteral("%1 × %2").arg(m_image.width()).arg(m_image.height()));
    setToolTip(accessibleDescription());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
    m_image.setDevicePixelRatio(1.0);
}

QSize OcrSourcePreview::fittedSize() const
{
    const QRect bounds = contentsRect().adjusted(4, 4, -4, -4);
    if (m_image.isNull() || bounds.width() <= 0 || bounds.height() <= 0) {
        return {};
    }
    const QSize fitted = m_image.size().scaled(bounds.size(), Qt::KeepAspectRatio);
    return QSize(std::max(1, qRound(fitted.width() * m_zoom)),
                 std::max(1, qRound(fitted.height() * m_zoom)));
}

QRect OcrSourcePreview::imageRect() const
{
    const QRect bounds = contentsRect().adjusted(4, 4, -4, -4);
    QRect target(QPoint(), fittedSize());
    target.moveCenter(bounds.center() + m_pan.toPoint());
    return target;
}

void OcrSourcePreview::clampPan()
{
    const QRect bounds = contentsRect().adjusted(4, 4, -4, -4);
    if (bounds.isEmpty() || m_zoom <= 1.01) {
        m_pan = {};
        return;
    }
    QRect target(QPoint(), fittedSize());
    target.moveCenter(bounds.center() + m_pan.toPoint());
    constexpr int kMinVisible = 48;
    if (target.right() < bounds.left() + kMinVisible) {
        target.moveRight(bounds.left() + kMinVisible);
    }
    if (target.left() > bounds.right() - kMinVisible) {
        target.moveLeft(bounds.right() - kMinVisible);
    }
    if (target.bottom() < bounds.top() + kMinVisible) {
        target.moveBottom(bounds.top() + kMinVisible);
    }
    if (target.top() > bounds.bottom() - kMinVisible) {
        target.moveTop(bounds.bottom() - kMinVisible);
    }
    m_pan = QPointF(target.center()) - QPointF(bounds.center());
}

void OcrSourcePreview::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    if (m_image.isNull()) {
        return;
    }
    // 1. 【OCR】【原图页面】按视区等比适配后再叠加用户缩放和平移
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(imageRect(), m_image);
}

void OcrSourcePreview::wheelEvent(QWheelEvent *event)
{
    if (m_image.isNull() || event->angleDelta().y() == 0) {
        QFrame::wheelEvent(event);
        return;
    }
    const QRect before = imageRect();
    if (before.isEmpty()) {
        return;
    }
    const QPointF cursor = event->position();
    const qreal fx = (cursor.x() - before.left()) / std::max(1, before.width());
    const qreal fy = (cursor.y() - before.top()) / std::max(1, before.height());
    const qreal factor = event->angleDelta().y() > 0 ? 1.12 : 1.0 / 1.12;
    m_zoom = std::clamp(m_zoom * factor, 1.0, 6.0);
    if (m_zoom <= 1.01) {
        m_zoom = 1.0;
        m_pan = {};
    } else {
        const QRect bounds = contentsRect().adjusted(4, 4, -4, -4);
        const QSize size = fittedSize();
        const QPointF topLeft(cursor.x() - fx * size.width(), cursor.y() - fy * size.height());
        m_pan = topLeft + QPointF(size.width() / 2.0, size.height() / 2.0) - QPointF(bounds.center());
        clampPan();
    }
    setCursor(m_zoom > 1.01 ? Qt::OpenHandCursor : Qt::ArrowCursor);
    update();
    event->accept();
}

void OcrSourcePreview::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_zoom > 1.01) {
        m_dragging = true;
        m_dragAnchor = event->position();
        m_panAnchor = m_pan;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void OcrSourcePreview::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        m_pan = m_panAnchor + (event->position() - m_dragAnchor);
        clampPan();
        update();
        event->accept();
        return;
    }
    setCursor(m_zoom > 1.01 ? Qt::OpenHandCursor : Qt::ArrowCursor);
    QFrame::mouseMoveEvent(event);
}

void OcrSourcePreview::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        setCursor(m_zoom > 1.01 ? Qt::OpenHandCursor : Qt::ArrowCursor);
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void OcrSourcePreview::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_zoom = 1.0;
        m_pan = {};
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
        update();
        event->accept();
        return;
    }
    QFrame::mouseDoubleClickEvent(event);
}

void OcrSourcePreview::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_image.isNull()) {
        return;
    }
    // 1. 【OCR】【原图菜单】编辑进入标注，置顶打开可独立拖动的贴图
    markshot::ui::ContextMenu menu(this);
    menu.setColorScheme(markshot::ui::configuredContextMenuScheme());
    const QImage image = m_image;
    menu.addAction(MS_TR("Edit"), this, [image] {
        markshot::openImageForAnnotation(image, MS_TR("Source image"));
    });
    menu.addAction(MS_TR("Pin image"), this, [image] {
        auto *window = new markshot::shot::PinnedImageWindow(image);
        markshot::shot::presentPinnedImage(window);
    });
    menu.exec(event->globalPos());
}

}
