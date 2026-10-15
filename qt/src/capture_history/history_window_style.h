#pragma once

#include "ui/interface_theme_config.h"

#include <QString>

namespace markshot::history {

/// @brief 复用设置页控件样式并补充历史列表和预览区域
/// @param mode 当前生效的明暗主题
/// @return 历史窗口及其确认对话框使用的样式表
QString historyWindowStyleSheet(markshot::ui::UiThemeMode mode);

}
