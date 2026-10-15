#include "pinned_window/pinned_image_window.h"

#include "pinned_window/pinned_layer_shell_drag_preview.h"

#include <QPainterPath>
#include <QPen>
#include <QPointer>
#include <QTimer>

#include <algorithm>

namespace markshot::shot {

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
    if (m_translationActive) {
        drawTranslationOverlay(painter);
    }

    auto drawBorder = [this, &painter, &imageRect] {
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

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    const auto [first, last] = selectionRange();
    const QVector<OcrToken> &tokens = activeTokens();
    // 相邻词的 OCR 检测框之间有缝隙、首尾还常常互相重叠：逐框绘制半透明
    // 高亮会出现断续的空隙和重叠处颜色加深。这里把选中的词框按行合并成
    // 连续条带，全部并入一条路径后一次填充，效果与浏览器文本选中一致。
    QPainterPath selectionPath;
    selectionPath.setFillRule(Qt::WindingFill);
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
        // 垂直方向重叠超过较矮框一半即视为同一行，合并进当前条带
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
    painter.fillPath(selectionPath, QColor(72, 132, 245, 96));
    drawBorder();
}

}  // namespace markshot::shot
