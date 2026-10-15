#include "pinned_window/pinned_layer_shell_drag_preview.h"

#include "layer_shell_runtime.h"
#include "pinned_window/pinned_layer_shell_geometry.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QTimer>

#include <utility>

namespace markshot::shot {

PinnedLayerShellDragPreview::PinnedLayerShellDragPreview(QWidget *owner, PaintImage paintImage)
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput
                         | Qt::WindowDoesNotAcceptFocus)
    , m_owner(owner)
    , m_paintImage(std::move(paintImage))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
}

bool PinnedLayerShellDragPreview::setLogicalGeometry(QRect geometry)
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    QVector<QRect> geometries;
    geometries.reserve(screens.size());
    for (QScreen *screen : screens) {
        geometries.append(screen->geometry());
    }
    const int index = bestPinnedLayerShellScreenIndex(geometry, geometries);
    if (index < 0) {
        return false;
    }
    QScreen *targetScreen = screens.at(index);
    const PinnedLayerShellPlacement placement = pinnedLayerShellPlacement(
        geometry, targetScreen->geometry(), QSize(24, 24));

    // 1. 【钉图】【拖动预览】仅重建预览 surface，原窗口继续在固定原点接收跨屏鼠标输入
    if (screen() != targetScreen) {
        hide();
        destroy();
        m_configured = false;
        setScreen(targetScreen);
    }
    setFixedSize(placement.desiredSize);
    markshot::layershell::FloatingOverlayConfig config;
    config.scope = QStringLiteral("mark-shot-pinned-drag-preview");
    // 2. 【钉图】【拖动预览】协议层也须禁用键盘交互，避免 Hyprland 映射预览时释放原窗口的鼠标按键
    config.keyboardInteractivity = markshot::layershell::KeyboardInteractivity::None;
    config.activateOnShow = false;
    config.desiredSize = placement.desiredSize;
    config.margins = placement.margins;
    const bool configured = m_configured
        ? markshot::layershell::updateFloatingOverlay(this, targetScreen, config)
        : markshot::layershell::configureFloatingOverlay(this, targetScreen, config);
    if (!configured) {
        return false;
    }
    m_configured = true;
    if (!isVisible()) {
        show();
    }
    update();
    return true;
}

bool PinnedLayerShellDragPreview::isReady() const
{
    return m_ready;
}

void PinnedLayerShellDragPreview::paintEvent(QPaintEvent *)
{
    if (!m_owner) {
        close();
        return;
    }
    QPainter painter(this);
    m_paintImage(painter, QRectF(rect()));
    if (!m_ready) {
        m_ready = true;
        // 1. 【钉图】【拖动预览】预览缓冲区提交后再清除原图，避免首次映射时出现空帧
        QTimer::singleShot(0, m_owner, [owner = m_owner] { owner->update(); });
    }
}

}  // namespace markshot::shot
