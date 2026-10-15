#include "ocr_result_window/ocr_source_preview.h"

#include "ui/i18n.h"

#include <QPainter>
#include <QPaintEvent>

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
    setCursor(Qt::ArrowCursor);
    m_image.setDevicePixelRatio(1.0);
}

void OcrSourcePreview::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    if (m_image.isNull()) {
        return;
    }
    // 1. 【OCR】【原图页面】保留原始图像，绘制时按视区等比缩放以适配高分辨率屏幕
    const QRect bounds = contentsRect().adjusted(4, 4, -4, -4);
    const QSize fitted = m_image.size().scaled(bounds.size(), Qt::KeepAspectRatio);
    QRect target(QPoint(), fitted);
    target.moveCenter(bounds.center());
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(target, m_image);
}

}
