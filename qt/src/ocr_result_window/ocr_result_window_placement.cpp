#include "ocr_result_window/ocr_result_window.h"

#include "app_config_store.h"
#include "debug_log.h"
#include "pinned_window_top.h"
#include "ui/i18n.h"

#include <QApplication>
#include <QJsonValue>
#include <QLabel>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QTabBar>
#include <QWindow>

namespace markshot::shot {

bool OcrResultWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UngrabMouse || event->type() == QEvent::Hide
        || (event->type() == QEvent::Enter && !QApplication::mouseButtons().testFlag(Qt::LeftButton))) {
        resetWindowDrag();
    }
    return QWidget::event(event);
}

void OcrResultWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
    if (!m_logicalGeometry.isValid()) {
        return;
    }
    // 1. 【OCR】【窗口尺寸】平台回报可能滞后于拖动，避免将旧尺寸作为新请求反复提交
    const bool layerShell = pinnedWindowHasLayerShellTop(this);
    if (layerShell && event->spontaneous()) {
        return;
    }
    // 2. 【OCR】【窗口尺寸】只提交应用内发起的浮层尺寸变化，置顶切换继续使用最新目标尺寸
    if (layerShell) {
        m_logicalGeometry.setSize(size());
        syncPinnedWindowTopGeometry(this, m_logicalGeometry);
    } else {
        m_logicalGeometry = geometry();
    }
    setProperty("markShotPinnedGeometry", m_logicalGeometry);
}

void OcrResultWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
        && m_titleBar->geometry().contains(event->position().toPoint())
        && !titleControlContains(event->position().toPoint())) {
        beginWindowDrag(event);
        return;
    }
    QWidget::mousePressEvent(event);
}

void OcrResultWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!updateWindowDrag(event)) {
        QWidget::mouseMoveEvent(event);
    }
}

void OcrResultWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (!finishWindowDrag(event)) {
        QWidget::mouseReleaseEvent(event);
    }
}

bool OcrResultWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_titleBar) {
        const bool pointerReleased = event->type() == QEvent::MouseMove
            ? !static_cast<QMouseEvent *>(event)->buttons().testFlag(Qt::LeftButton)
            : event->type() == QEvent::Enter && !QApplication::mouseButtons().testFlag(Qt::LeftButton);
        if (pointerReleased) {
            resetWindowDrag();
        }
        if (event->type() == QEvent::MouseButtonPress) {
            return beginWindowDrag(static_cast<QMouseEvent *>(event));
        }
        if (event->type() == QEvent::MouseMove && m_dragging) {
            return updateWindowDrag(static_cast<QMouseEvent *>(event));
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            return finishWindowDrag(static_cast<QMouseEvent *>(event));
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool OcrResultWindow::titleControlContains(QPoint windowPoint) const
{
    const QList<QWidget *> controls{m_viewTabs, m_translationToggle, m_moreButton, m_pinButton, m_closeButton};
    for (QWidget *control : controls) {
        if (!control->isVisible()) {
            continue;
        }
        const QRect bounds(control->mapTo(this, QPoint()), control->size());
        if (bounds.contains(windowPoint)) {
            return true;
        }
    }
    return false;
}

void OcrResultWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_alwaysOnTop || pinnedWindowHasLayerShellTop(this)) {
        return;
    }
    // 1. 【OCR】【窗口置顶】等待窗口映射完成，供 GNOME 等桌面按标题查找新窗口
    for (int delayMs : {0, 80, 250, 600}) {
        QTimer::singleShot(delayMs, this, [this] {
            if (isVisible() && m_alwaysOnTop && !pinnedWindowHasLayerShellTop(this)) {
                raisePinnedWindowOnPlatform(this);
            }
        });
    }
}

void OcrResultWindow::recreateWindowSurface()
{
    const bool visible = isVisible();
    const bool resumeDrag = m_dragging && QApplication::mouseButtons().testFlag(Qt::LeftButton);
    QScreen *target = pinnedWindowTargetLayerShellScreen(m_logicalGeometry);
    const QPointer<QWidget> focused = focusWidget();

    // 1. 【OCR】【窗口模式】销毁旧协议窗口，解除 layer-shell 对窗口管理器操作的限制
    if (QWidget::mouseGrabber() == this) {
        releaseMouse();
    }
    hide();
    destroy();
    setProperty("markShotPinnedLayerShellActive", false);
    if (target) {
        setScreen(target);
    }

    // 2. 【OCR】【窗口模式】按当前置顶开关重新创建窗口并恢复逻辑几何
    setGeometry(m_logicalGeometry);
    setProperty("markShotPinnedGeometry", m_logicalGeometry);
    applyPinnedWindowTopState(this, m_alwaysOnTop);
    if (visible) {
        show();
        raise();
        activateWindow();
        if (focused) {
            focused->setFocus(Qt::OtherFocusReason);
        }
    }
    if (resumeDrag) {
        m_dragging = true;
        m_titleBar->setCursor(Qt::ClosedHandCursor);
        setCursor(Qt::ClosedHandCursor);
        grabMouse();
    }
}

bool OcrResultWindow::beginWindowDrag(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        return false;
    }
    m_titleBar->setCursor(Qt::ClosedHandCursor);

    // 1. 【OCR】【窗口拖动】普通窗口优先交给窗口管理器移动
    const bool layerShell = pinnedWindowHasLayerShellTop(this);
    if (!layerShell && windowHandle() && windowHandle()->startSystemMove()) {
        event->accept();
        return true;
    }

    // 2. 【OCR】【窗口拖动】置顶浮层使用记录的逻辑位置计算拖动偏移
    if (!layerShell) {
        m_logicalGeometry = geometry();
    }
    m_dragging = true;
    m_dragOffset = event->globalPosition().toPoint() - m_logicalGeometry.topLeft();
    setCursor(Qt::ClosedHandCursor);
    grabMouse();
    event->accept();
    return true;
}

bool OcrResultWindow::updateWindowDrag(QMouseEvent *event)
{
    if (!event) {
        return false;
    }
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        resetWindowDrag();
        return false;
    }
    if (!m_dragging) {
        return false;
    }

    // 1. 【OCR】【窗口拖动】同步普通窗口位置与浮层边距
    m_logicalGeometry.moveTopLeft(event->globalPosition().toPoint() - m_dragOffset);
    m_logicalGeometry.setSize(size());
    setGeometry(m_logicalGeometry);
    if (pinnedWindowHasLayerShellTop(this)) {
        if (pinnedWindowNeedsLayerShellScreenRebind(this, m_logicalGeometry)) {
            recreateWindowSurface();
        } else {
            syncPinnedWindowTopGeometry(this, m_logicalGeometry);
        }
    }
    event->accept();
    return true;
}

bool OcrResultWindow::finishWindowDrag(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        return false;
    }
    const bool wasDragging = m_dragging;
    resetWindowDrag();
    if (!wasDragging) {
        return false;
    }
    event->accept();
    return true;
}

void OcrResultWindow::resetWindowDrag()
{
    // 【OCR】【拖动恢复】先清理状态，再释放捕获，避免 UngrabMouse 同步重入时继续移动
    m_dragging = false;
    if (QWidget::mouseGrabber() == this) {
        releaseMouse();
    }
    if (m_titleBar) {
        m_titleBar->setCursor(Qt::OpenHandCursor);
    }
    unsetCursor();
}

void OcrResultWindow::setAlwaysOnTop(bool alwaysOnTop)
{
    if (m_alwaysOnTop == alwaysOnTop) {
        return;
    }

    // 1. 【OCR】【置顶配置】只保存 OCR 窗口的开关，不修改钉图配置
    QString error;
    if (!markshot::writeAppConfigValue({QStringLiteral("ocrResultWindow"), QStringLiteral("alwaysOnTop")},
                                       QJsonValue(alwaysOnTop), &error)) {
        const QSignalBlocker blocker(m_pinButton);
        m_pinButton->setChecked(m_alwaysOnTop);
        showToast(MS_TR("Failed to save settings"));
        markshot::debugLog("ocr", "【OCR】【置顶配置】保存失败: %s", error.toUtf8().constData());
        return;
    }

    // 2. 【OCR】【窗口模式】只有切换协议角色时才重建窗口
    const bool wasLayerShell = pinnedWindowHasLayerShellTop(this);
    const bool changeSurface = wasLayerShell || (alwaysOnTop && pinnedWindowUsesLayerShellTop());
    if (!wasLayerShell) {
        if (changeSurface && QGuiApplication::platformName().contains(QStringLiteral("wayland")) && screen()) {
            // 3. 【OCR】【窗口位置】普通 Wayland 窗口不提供全局位置，置顶时在当前屏幕居中
            const QRect available = screen()->availableGeometry();
            m_logicalGeometry = QRect(QPoint(), size().boundedTo(available.size()));
            m_logicalGeometry.moveCenter(available.center());
        } else {
            m_logicalGeometry = geometry();
        }
    }
    m_alwaysOnTop = alwaysOnTop;
    if (changeSurface) {
        recreateWindowSurface();
    } else {
        applyPinnedWindowTopState(this, m_alwaysOnTop);
    }
    m_pinButton->setToolTip(alwaysOnTop ? MS_TR("Always on Top: On") : MS_TR("Always on Top: Off"));
}

}
