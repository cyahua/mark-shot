#pragma once

#include "layer_shell_runtime.h"

class QWidget;

namespace markshot::shot {

/// 关闭时贴图仍是普通置顶窗口
inline constexpr bool FEATURE_PINNED_NON_WINDOW_CHROME_ENABLED = true;

/**
 * 显示贴图。功能开启时不激活窗口，避免自动隐藏的任务栏被唤出。
 * @param window 贴图窗口。
 * @return 无返回值。
 */
void presentPinnedImage(QWidget *window);

/**
 * 调整 layer-shell 贴图，显示时不请求键盘焦点。
 * @param config 浮动覆盖层配置。
 * @return 无返回值。
 */
void applyPinnedNonWindowLayerShellPolicy(markshot::layershell::FloatingOverlayConfig *config);

}  // namespace markshot::shot
