#include "pinned_window/pinned_image_window.h"

#include "pinned_window/pinned_layer_shell_drag_preview.h"

#include <QImage>
#include <QPainterPath>
#include <QPen>
#include <QPointer>
#include <QTimer>

#include <algorithm>

namespace markshot::shot {
namespace {

/**
 * 把路径画成均匀色块。重叠区域只保留一层颜色，再整体套上透明度。
 * @param painter 目标画笔。
 * @param path 要填充的路径。
 * @param color 不透明时的颜色，透明度由 alpha 统一控制。
 * @param alpha 整层透明度，0 到 255。
 * @param clearPath 需要从色块中挖掉的路径，可为空。
 * @return 无返回值。
 */
void paintSolidWash(QPainter &painter,
                    const QPainterPath &path,
                    const QColor &color,
                    int alpha,
                    const QPainterPath *clearPath)
{
    if (path.isEmpty() || alpha <= 0) {
        return;
    }

    const qreal dpr = std::max<qreal>(1.0, painter.device() ? painter.device()->devicePixelRatioF() : 1.0);
    const QRectF bounds = path.controlPointRect().adjusted(-2.0, -2.0, 2.0, 2.0);
    if (bounds.isEmpty()) {
        return;
    }

    // 1. 先用完全不透明的颜色铺满，重叠处不会越叠越深
    QImage layer(qCeil(bounds.width() * dpr), qCeil(bounds.height() * dpr), QImage::Format_ARGB32_Premultiplied);
    layer.setDevicePixelRatio(dpr);
    layer.fill(Qt::transparent);
    QPainter layerPainter(&layer);
    layerPainter.setRenderHint(QPainter::Antialiasing, true);
    layerPainter.translate(-bounds.topLeft());
    layerPainter.setPen(Qt::NoPen);
    layerPainter.setBrush(QColor(color.red(), color.green(), color.blue()));
    layerPainter.drawPath(path);
    if (clearPath && !clearPath->isEmpty()) {
        layerPainter.setCompositionMode(QPainter::CompositionMode_Clear);
        layerPainter.setBrush(Qt::black);
        layerPainter.drawPath(*clearPath);
    }
    layerPainter.end();

    // 2. 整层只乘一次透明度
    painter.save();
    painter.setOpacity(painter.opacity() * (alpha / 255.0));
    painter.drawImage(bounds.topLeft(), layer);
    painter.restore();
}

}  // namespace

void PinnedImageWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (m_layerShellDragPreviewActive && m_layerShellDragPreview->isReady()) {
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), Qt::transparent);
        m_layerShellDragPreview->update();
        return;
    }
    paintImageContents(painter, QRectF(rect()));
    if (m_layerShellDragPreview && !m_layerShellDragPreviewActive) {
        const QPointer<PinnedLayerShellDragPreview> preview = m_layerShellDragPreview;
        // 1. 【钉图】【拖动预览】原图完成绘制和缓冲区提交后，再移除最后一帧预览
        QTimer::singleShot(0, this, [this, preview] {
            if (preview && m_layerShellDragPreview == preview && !m_layerShellDragPreviewActive) {
                m_layerShellDragPreview = nullptr;
                preview->hide();
                preview->deleteLater();
            }
        });
    }
}

void PinnedImageWindow::paintImageContents(QPainter &painter, QRectF viewport)
{
    painter.setClipRect(viewport, Qt::IntersectClip);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRectF imageRect = displayedImageRect();
    painter.drawPixmap(imageRect, m_pixmap, QRectF(QPointF(0.0, 0.0), QSizeF(m_pixmap.size())));
    paintOcrRecognitionPlatesIfEnabled(painter);
    if (m_translationActive) {
        drawTranslationOverlay(painter);
    }

    auto drawBorder = [this, &painter, &imageRect] {
        if (paintPinnedNonWindowChromeIfEnabled(painter, imageRect)) {
            return;
        }
        if (!m_config.borderEnabled || !m_config.borderColor.isValid() || m_config.borderWidth <= 0.0) {
            return;
        }
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(m_config.borderColor, m_config.borderWidth));
        const qreal inset = m_config.borderWidth / 2.0;
        painter.drawRect(imageRect.adjusted(inset, inset, -inset, -inset));
        painter.restore();
    };

    if (!hasTextSelection()) {
        drawBorder();
        return;
    }

    paintSolidWash(painter, textSelectionWidgetPath(viewport), QColor(72, 132, 245), 96, nullptr);
    drawBorder();
}

void PinnedImageWindow::paintOcrRecognitionPlatesIfEnabled(QPainter &painter) const
{
    // 识别结果只托一层轻背景，原文像素继续显示，不再把识别文字画回去
    if (m_translationActive || m_ocrTokens.isEmpty()) {
        return;
    }

    QPainterPath plates;
    plates.setFillRule(Qt::WindingFill);
    QRectF band;
    int bandLine = -1;
    const auto flush = [this, &plates, &band] {
        if (band.isNull()) {
            return;
        }
        // 1. 内边距用窗口像素，避免图片缩放后背景框相对字形漂移
        const QRectF plate = imageToWidget(band).adjusted(-4.0, -2.0, 4.0, 2.0);
        plates.addRoundedRect(plate, 5.0, 5.0);
        band = QRectF();
    };
    for (const OcrToken &token : m_ocrTokens) {
        if (token.imageRect.isEmpty() || token.text.trimmed().isEmpty()) {
            continue;
        }
        if (band.isNull()) {
            band = token.imageRect;
            bandLine = token.line;
            continue;
        }
        const qreal gap = token.imageRect.left() - band.right();
        const qreal limit = std::max<qreal>(8.0, std::min(band.height(), token.imageRect.height()) * 0.65);
        const bool sameLine = token.line == bandLine
            && token.imageRect.top() < band.bottom()
            && token.imageRect.bottom() > band.top();
        if (sameLine && gap <= limit) {
            band = band.united(token.imageRect);
        } else {
            flush();
            band = token.imageRect;
            bandLine = token.line;
        }
    }
    flush();

    // 2. 选区单独着色，先从浅底里挖掉，两种颜色不再叠在一起
    const QPainterPath selection = textSelectionWidgetPath(QRectF(rect()));
    paintSolidWash(painter, plates, QColor(248, 250, 252), 72, selection.isEmpty() ? nullptr : &selection);
}

QPainterPath PinnedImageWindow::textSelectionWidgetPath(QRectF viewport) const
{
    QPainterPath selectionPath;
    if (!hasTextSelection()) {
        return selectionPath;
    }

    selectionPath.setFillRule(Qt::WindingFill);
    const auto [first, last] = selectionRange();
    const QVector<OcrToken> &tokens = activeTokens();
    // 相邻词的 OCR 检测框之间有缝隙、首尾还常常互相重叠。这里按行收成一条带，
    // 再整层填一次色，重叠处保持同一透明度。
    QRectF lineBand;
    const auto flushLineBand = [this, &selectionPath, &lineBand, viewport] {
        if (!lineBand.isNull()) {
            selectionPath.addRect(imageToWidget(lineBand).intersected(viewport));
            lineBand = QRectF();
        }
    };
    for (int i = first; i <= last; ++i) {
        const QRectF tokenRect = selectionImageRectForToken(tokens.at(i));
        if (tokenRect.isEmpty()) {
            continue;
        }
        if (lineBand.isNull()) {
            lineBand = tokenRect;
            continue;
        }
        const qreal overlapHeight = std::min(lineBand.bottom(), tokenRect.bottom())
            - std::max(lineBand.top(), tokenRect.top());
        if (overlapHeight >= std::min(lineBand.height(), tokenRect.height()) * 0.5) {
            lineBand = lineBand.united(tokenRect);
        } else {
            flushLineBand();
            lineBand = tokenRect;
        }
    }
    flushLineBand();
    return selectionPath;
}

}  // namespace markshot::shot
