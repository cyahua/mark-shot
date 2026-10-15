#pragma once

#include "ui/context_menu.h"

namespace markshot::ui {

/**
 * 按应用配置中的界面主题（跟随系统、暗色、亮色）解析上下文菜单主题。
 * 用于托盘、贴图等不继承设置窗口调色板的菜单。
 * @return 明确的暗色或亮色主题。
 */
ContextMenu::ColorScheme configuredContextMenuScheme();

}  // namespace markshot::ui
