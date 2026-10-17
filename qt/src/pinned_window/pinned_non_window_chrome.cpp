#include "pinned_window/pinned_non_window_chrome.h"

#include <QWidget>

namespace markshot::shot {

void presentPinnedImage(QWidget *window)
{
    if (!window) {
        return;
    }
    window->show();
    window->raise();
    // 1. 非窗口贴图不激活，最大化窗口下的自动隐藏面板保持隐藏
    if (!FEATURE_PINNED_NON_WINDOW_CHROME_ENABLED) {
        window->activateWindow();
    }
}

void applyPinnedNonWindowLayerShellPolicy(markshot::layershell::FloatingOverlayConfig *config)
{
    Q_UNUSED(config);
    // 显示位置继续走原来的 layer-shell 边距，不改锚点和激活策略
}

}  // namespace markshot::shot
