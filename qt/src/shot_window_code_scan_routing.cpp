#include "shot_window_module.h"

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>

using namespace markshot::shot;

namespace {

/// 关闭时扫码模式留在收到快捷键的覆盖层
inline constexpr bool FEATURE_CODE_SCANNER_FOLLOWS_CURSOR_ENABLED = true;

}  // namespace

markshot::shot::types::StartupTool ShotWindow::startupTool() const
{
    return m_startupTool;
}

QScreen *ShotWindow::captureScreen() const
{
    return m_captureScreen ? m_captureScreen.data() : screen();
}

void ShotWindow::applyRoutedStartupTool(markshot::shot::types::StartupTool tool)
{
    // 1. 会话转发期间不再根据光标二次发射，避免两个覆盖层来回切换
    m_applyingRoutedStartupTool = true;
    if (tool == markshot::shot::types::StartupTool::None) {
        if (m_startupTool != markshot::shot::types::StartupTool::None) {
            leaveStartupTool();
        }
    } else if (m_startupTool != tool) {
        setStartupTool(tool);
    }
    m_applyingRoutedStartupTool = false;
}

void ShotWindow::routeCodeScannerToCursorScreenIfEnabled()
{
    if (!FEATURE_CODE_SCANNER_FOLLOWS_CURSOR_ENABLED || m_applyingRoutedStartupTool) {
        return;
    }
    if (m_mode != Mode::Selecting || m_startupTool != StartupTool::CodeScanner) {
        return;
    }
    QScreen *cursorScreen = QGuiApplication::screenAt(QCursor::pos());
    QScreen *ownScreen = captureScreen();
    if (!cursorScreen || !ownScreen || cursorScreen == ownScreen) {
        return;
    }
    emit codeScannerRouteRequested(this);
}

void ShotWindow::claimCodeScannerFromPeersIfEnabled(QEvent *event)
{
    if (!FEATURE_CODE_SCANNER_FOLLOWS_CURSOR_ENABLED || !event) {
        return;
    }
    if (event->type() != QEvent::Enter) {
        return;
    }
    if (m_mode != Mode::Selecting || m_startupTool == StartupTool::CodeScanner) {
        return;
    }
    emit codeScannerClaimRequested(this);
}
