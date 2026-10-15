#pragma once

class QWidget;

namespace markshot::shot {

/// @brief 判断当前 Qt 窗口是否需要 KDE 的置顶状态通道
/// @return 启用了 D-Bus 且 KDE 会话使用 Wayland 或 XCB 窗口时返回 true
bool usesKdePinnedKeepAbove();

/// @brief 为指定钉图或 OCR 窗口应用独立的 KWin 置顶状态
/// @param window 目标窗口，标题和脚本身份在其生命周期内保持独立
/// @param alwaysOnTop 是否保持置顶
/// @return 脚本提交成功返回 true，实际状态由 KWin 应用
bool applyKdePinnedWindowKeepAbove(QWidget *window, bool alwaysOnTop);

}  // namespace markshot::shot
