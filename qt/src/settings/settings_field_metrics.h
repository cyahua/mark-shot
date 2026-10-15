#pragma once

namespace markshot::settings::metrics {

// 设置表单字段尺寸规范：字段统一靠卡片右侧对齐，宽度按控件类型分档

/// @brief 普通单行输入与下拉框的最大宽度
inline constexpr int kTextFieldMaxWidth = 280;
/// @brief 快捷键胶囊的固定宽度
inline constexpr int kShortcutFieldWidth = 240;
/// @brief 颜色胶囊按钮的固定宽度
inline constexpr int kColorFieldWidth = 120;
/// @brief 数值输入框的最大宽度
inline constexpr int kNumberFieldMaxWidth = 144;
/// @brief 环境变量等多行文本的默认可见行数
inline constexpr int kEnvironmentVisibleLines = 4;

}  // namespace markshot::settings::metrics
