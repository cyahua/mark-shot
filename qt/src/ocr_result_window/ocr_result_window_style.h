#pragma once

#include "shot_window_types.h"

#include <QIcon>
#include <QPalette>
#include <QString>

namespace markshot::shot {

/// @brief 根据应用调色板生成 OCR 窗口的统一样式
/// @param palette 当前明暗主题对应的调色板
/// @return 仅应用于 OCR 窗口及其子控件的样式
QString ocrWindowStyleSheet(const QPalette &palette);

/// @brief 按当前主题重着色应用已有的工具图标
/// @param action 已有工具图标编号
/// @param ink 普通状态颜色
/// @param checkedInk 选中状态颜色，无效时沿用普通颜色
/// @return 包含普通、选中与禁用状态的图标
QIcon ocrActionIcon(types::Action action, QColor ink, QColor checkedInk = {});

/// @brief 创建翻译操作图标，使用主题文字与强调色区分展开状态
/// @param ink 常态文字颜色
/// @param checkedInk 展开状态颜色
/// @return 含常态与展开状态的翻译图标
QIcon ocrTranslationIcon(QColor ink, QColor checkedInk);

}
