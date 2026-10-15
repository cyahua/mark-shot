#include "pinned_window/pinned_image_window.h"

#include "pinned_window_top.h"
#include "pinned_window/pinned_layer_shell_drag_preview.h"

#include <QApplication>
#include <QScreen>
#include <QScopedValueRollback>
#include <QTimer>

namespace markshot::shot {

void PinnedImageWindow::beginLayerShellDragPreview()
{
    if (m_layerShellDragPreviewActive || !pinnedWindowHasLayerShellTop(this)) {
        return;
    }
    m_layerShellDragInputGeometry = m_layerShellVisibleGeometry;
    if (!m_layerShellDragPreview) {
        m_layerShellDragPreview = new PinnedLayerShellDragPreview(this, [this](QPainter &painter, QRectF viewport) {
            paintImageContents(painter, viewport);
        });
    }
    m_layerShellDragPreviewActive = true;
    if (!m_layerShellDragPreview->setLogicalGeometry(m_logicalGeometry)) {
        m_layerShellDragPreviewActive = false;
        delete m_layerShellDragPreview;
        m_layerShellDragPreview = nullptr;
        m_layerShellDragInputGeometry = {};
    }
}

void PinnedImageWindow::finishLayerShellDragPreview()
{
    if (!m_layerShellDragPreviewActive) {
        return;
    }
    m_layerShellDragPreviewActive = false;
    m_layerShellDragInputGeometry = {};
    setPinnedGeometry(m_logicalGeometry, false);
    if (pinnedWindowNeedsLayerShellScreenRebind(this, m_logicalGeometry)) {
        rebindLayerShellScreen();
    }
    update();
}

QPointF PinnedImageWindow::pinnedLocalPointForInput(QPointF position) const
{
    if (!m_layerShellDragPreviewActive) {
        return position;
    }
    return position + QPointF(m_layerShellDragInputGeometry.topLeft() - m_layerShellVisibleGeometry.topLeft());
}

void PinnedImageWindow::scheduleLayerShellScreenRebind()
{
    if (m_layerShellDragPreviewActive || m_layerShellScreenRebindPending
        || !pinnedWindowNeedsLayerShellScreenRebind(this, m_logicalGeometry)) {
        return;
    }

    m_layerShellScreenRebindPending = true;
    QTimer::singleShot(0, this, [this] {
        m_layerShellScreenRebindPending = false;
        rebindLayerShellScreen();
    });
}

void PinnedImageWindow::rebindLayerShellScreen()
{
    if (m_layerShellDragPreviewActive || !pinnedWindowHasLayerShellTop(this)
        || !pinnedWindowNeedsLayerShellScreenRebind(this, m_logicalGeometry)) {
        return;
    }

    QScreen *targetScreen = pinnedWindowTargetLayerShellScreen(m_logicalGeometry);
    if (!targetScreen) {
        return;
    }

    // 1. 【钉图】【跨屏重建】临时隐藏原窗口期间保留预览，重新绘制后再交还画面
    const QScopedValueRollback<bool> rebindGuard(m_layerShellScreenRebindInProgress, true);
    const QRect logicalGeometry = m_logicalGeometry;
    const bool wasVisible = isVisible();
    const bool continueMouseDrag = QApplication::mouseButtons().testFlag(Qt::LeftButton);
    if (QWidget::mouseGrabber() == this) {
        releaseMouse();
    }

    // 2. 【钉图】【跨屏重建】销毁旧 wl_surface，避免继续向原输出提交不可见内容
    hide();
    destroy();
    setProperty("markShotPinnedLayerShellActive", false);
    setScreen(targetScreen);

    // 3. 【钉图】【跨屏重建】在目标输出重新创建 layer-shell surface，并恢复完整逻辑几何
    setMinimumSize(QSize(24, 24));
    setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    setFixedSize(logicalGeometry.size());
    m_logicalGeometry = logicalGeometry;
    setProperty("markShotPinnedGeometry", m_logicalGeometry);
    applyPinnedWindowTopState(this, m_config.alwaysOnTop);
    setPinnedGeometry(m_logicalGeometry, false);

    // 4. 【钉图】【跨屏重建】恢复可见状态和鼠标抓取，使跨屏拖拽连续进行
    if (wasVisible) {
        show();
    }
    if (continueMouseDrag) {
        grabMouse();
        setCursor(Qt::ClosedHandCursor);
    }
    schedulePinnedWindowRaise();
}

}  // namespace markshot::shot
