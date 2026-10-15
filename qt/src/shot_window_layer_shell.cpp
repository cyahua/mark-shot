#include "shot_window_module.h"

bool ShotWindow::configureLayerShell(QScreen *screen)
{
    const QSize desiredSize = m_sourceGeometry.isValid() && !m_sourceGeometry.isEmpty()
        ? m_sourceGeometry.size()
        : m_frozenFrame.size();
    if (!desiredSize.isEmpty()) {
        resize(desiredSize);
    }

    if (screen) {
        setScreen(screen);
    }

    return markshot::layershell::configureOverlay(
        this,
        screen,
        {QStringLiteral("dock"),
         markshot::layershell::KeyboardInteractivity::Exclusive,
         true,
         true});
}

void ShotWindow::updateLayerShellForIme()
{
    const bool imeActive = m_textEditor && m_textEditor->isVisible();
    markshot::layershell::setLayer(
        this, imeActive ? markshot::layershell::Layer::Top : markshot::layershell::Layer::Overlay);
    if (imeActive) {
        // 1. 【截图】【输入法定位】图层调整通过 Wayland 异步配置，在下一轮事件循环重新发布光标区域
        QTimer::singleShot(0, this, [this]() {
            if (m_textEditor && m_textEditor->isVisible()) {
                if (QInputMethod *im = QGuiApplication::inputMethod()) {
                    im->update(Qt::ImCursorRectangle);
                }
            }
        });
    }
}
