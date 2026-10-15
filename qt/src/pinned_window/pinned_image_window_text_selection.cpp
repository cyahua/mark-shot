#include "pinned_window/pinned_image_window.h"

#include "clipboard_image.h"
#include "ocr_result.h"
#include "pinned_window/pinned_text_selection_metrics.h"
#include "pinned_window_top.h"

#include <QApplication>
#include <QCursor>
#include <QWindow>

#include <algorithm>
#include <limits>

namespace markshot::shot {

bool PinnedImageWindow::deferTextSelection(QPointF widgetPoint, QPoint globalPoint)
{
    if (!m_config.textSelectionCopyEnabled || !m_config.ocrEnabled || !activeTokens().isEmpty()
        || (m_textSelectionOcrAttempted && !m_ocrTask)) {
        return false;
    }
    // 1. 【贴图】【按需拖选】缓存首次手势，复用已进行的 OCR，避免重复启动任务
    m_deferredTextSelection = DeferredTextSelection{widgetPoint, widgetPoint, globalPoint, pinnedTopLeft()};
    setCursor(Qt::BusyCursor);
    if (!m_ocrTask) {
        startOcr();
    }
    if (!m_ocrTask) {
        finishDeferredTextSelection();
    }
    return true;
}

void PinnedImageWindow::finishDeferredTextSelection()
{
    if (!m_deferredTextSelection) {
        return;
    }
    const DeferredTextSelection gesture = *m_deferredTextSelection;
    m_deferredTextSelection.reset();
    // 1. 【贴图】【按需拖选】使用原始按下位置判断文字，不把拖入文字区域的移动改为选区
    const auto anchor = tokenAt(widgetToImage(gesture.anchor));
    if (anchor) {
        m_selectionAnchor = *anchor;
        m_selectionFocus = closestToken(widgetToImage(gesture.focus)).value_or(*anchor);
        m_selectingText = !gesture.released;
        if (gesture.copyWhenReady) {
            copySelectedText();
        }
        update();
    } else {
        // 2. 【贴图】【按需拖选】起点没有文字时恢复原本的图片移动手势
        m_moving = !gesture.released;
        m_dragOffset = gesture.globalAnchor - gesture.windowTopLeft;
        const bool layerShell = pinnedWindowHasLayerShellTop(this);
        // 3. 【贴图】【按需拖选】先固定输入 surface，再追上识别期间的指针位移
        if (m_moving && layerShell) {
            beginLayerShellDragPreview();
        }
        const bool systemMove = m_moving && !layerShell && windowHandle()
            && windowHandle()->startSystemMove();
        const QPoint delta = (gesture.focus - gesture.anchor).toPoint();
        if (!systemMove && delta.manhattanLength() >= QApplication::startDragDistance()) {
            setPinnedGeometry(QRect(gesture.windowTopLeft + delta, logicalPinnedSize()),
                              !layerShell);
        }
    }
    updateCursorForPosition(gesture.focus);
}

QPointF PinnedImageWindow::widgetToImage(QPointF point) const
{
    const QRectF imageRect = displayedImageRect();
    if (imageRect.width() <= 0.0 || imageRect.height() <= 0.0 || m_imageSize.isEmpty()) {
        return {};
    }
    return QPointF((point.x() - imageRect.left()) * static_cast<qreal>(m_imageSize.width()) / imageRect.width(),
                   (point.y() - imageRect.top()) * static_cast<qreal>(m_imageSize.height()) / imageRect.height());
}

QRectF PinnedImageWindow::imageToWidget(QRectF imageRect) const
{
    if (m_imageSize.isEmpty()) {
        return {};
    }
    const QRectF displayRect = displayedImageRect();
    const qreal sx = displayRect.width() / static_cast<qreal>(m_imageSize.width());
    const qreal sy = displayRect.height() / static_cast<qreal>(m_imageSize.height());
    return QRectF(displayRect.left() + imageRect.left() * sx,
                  displayRect.top() + imageRect.top() * sy,
                  imageRect.width() * sx,
                  imageRect.height() * sy);
}

std::optional<int> PinnedImageWindow::tokenAt(QPointF imagePoint) const
{
    const QVector<OcrToken> &tokens = activeTokens();
    for (int i = 0; i < tokens.size(); ++i) {
        const QRectF hitRect = selectionImageRectForToken(tokens.at(i)).adjusted(-2.0, -2.0, 2.0, 2.0);
        if (hitRect.contains(imagePoint)) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<int> PinnedImageWindow::closestToken(QPointF imagePoint) const
{
    const QVector<OcrToken> &tokens = activeTokens();
    if (tokens.isEmpty()) {
        return std::nullopt;
    }

    int bestIndex = 0;
    qreal bestDistance = std::numeric_limits<qreal>::max();
    for (int i = 0; i < tokens.size(); ++i) {
        const QRectF rect = selectionImageRectForToken(tokens.at(i));
        const qreal dx = imagePoint.x() < rect.left()
            ? rect.left() - imagePoint.x()
            : imagePoint.x() > rect.right() ? imagePoint.x() - rect.right() : 0.0;
        const qreal dy = imagePoint.y() < rect.top()
            ? rect.top() - imagePoint.y()
            : imagePoint.y() > rect.bottom() ? imagePoint.y() - rect.bottom() : 0.0;
        const qreal distance = dx * dx + dy * dy;
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    return bestIndex;
}

void PinnedImageWindow::updateCursorForPosition(QPointF widgetPoint)
{
    if (m_deferredTextSelection) {
        setCursor(Qt::BusyCursor);
        return;
    }
    // 1. 【置顶图片】【光标状态】活动操作保持自己的光标，不受后台翻译完成事件覆盖
    if (isPinnedResizeDirection(m_resizeDrag.direction)) {
        setCursor(cursorForPinnedResizeDirection(m_resizeDrag.direction));
        return;
    }
    if (m_selectingText || m_moving) {
        setCursor(m_selectingText ? Qt::IBeamCursor : Qt::ClosedHandCursor);
        return;
    }
    // 2. 【置顶图片】【悬停反馈】缩放和可选文字优先，等待翻译仅影响当前窗口空白处
    const PinnedResizeDirection direction = resizeDirectionAt(widgetPoint);
    if (isPinnedResizeDirection(direction)) {
        setCursor(cursorForPinnedResizeDirection(direction));
        return;
    }

    if (m_config.textSelectionCopyEnabled && tokenAt(widgetToImage(widgetPoint))) {
        setCursor(Qt::IBeamCursor);
    } else {
        setCursor(m_translationBusyCursor ? Qt::BusyCursor : Qt::OpenHandCursor);
    }
}

bool PinnedImageWindow::hasTextSelection() const
{
    const QVector<OcrToken> &tokens = activeTokens();
    return m_selectionAnchor >= 0
        && m_selectionFocus >= 0
        && m_selectionAnchor < tokens.size()
        && m_selectionFocus < tokens.size();
}

std::pair<int, int> PinnedImageWindow::selectionRange() const
{
    const int first = std::min(m_selectionAnchor, m_selectionFocus);
    const int last = std::max(m_selectionAnchor, m_selectionFocus);
    return {first, last};
}

void PinnedImageWindow::clearTextSelection()
{
    if (m_selectionAnchor < 0 && m_selectionFocus < 0) {
        return;
    }
    m_selectionAnchor = -1;
    m_selectionFocus = -1;
    update();
}

QString PinnedImageWindow::selectedText() const
{
    if (!hasTextSelection()) {
        return {};
    }
    const auto [first, last] = selectionRange();
    return tokenRangeText(first, last);
}

void PinnedImageWindow::copySelectedText()
{
    if (!hasTextSelection()) {
        return;
    }
    markshot::copyTextToClipboard(selectedText());
}

QString PinnedImageWindow::allText() const
{
    const QVector<OcrToken> &tokens = activeTokens();
    if (tokens.isEmpty()) {
        return {};
    }
    return tokenRangeText(0, tokens.size() - 1);
}

void PinnedImageWindow::copyImageText()
{
    if (!m_config.ocrEnabled) {
        return;
    }
    if (!activeTokens().isEmpty()) {
        markshot::copyTextToClipboard(allText());
        return;
    }
    m_copyTextAfterOcr = true;
    if (!m_ocrTask) {
        startOcr();
    }
}

QString PinnedImageWindow::tokenRangeText(int first, int last) const
{
    const QVector<OcrToken> &tokens = activeTokens();
    return markshot::ocr::tokenRangeText(sharedOcrTokens(tokens), first, last);
}

QRectF PinnedImageWindow::selectionImageRectForToken(const OcrToken &token) const
{
    return pinnedTextSelectionHighlightRect(token.imageRect, token.text);
}

QVector<markshot::ocr::Token> PinnedImageWindow::sharedOcrTokens(const QVector<OcrToken> &tokens) const
{
    QVector<markshot::ocr::Token> sharedTokens;
    sharedTokens.reserve(tokens.size());
    for (const OcrToken &token : tokens) {
        sharedTokens.append({token.text,
                             token.imageRect,
                             token.line,
                             token.index,
                             token.confidence});
    }
    return sharedTokens;
}

const QVector<PinnedImageWindow::OcrToken> &PinnedImageWindow::activeTokens() const
{
    return m_translationActive ? m_translatedTokens : m_ocrTokens;
}

}  // namespace markshot::shot
