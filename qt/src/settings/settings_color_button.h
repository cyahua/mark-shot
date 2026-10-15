#pragma once

#include <QColor>

class QPushButton;

namespace markshot::settings {

/// @brief 将按钮配置为颜色胶囊：固定宽度、中性底色，左侧色块预览、右侧 Hex 文本
/// @param button 目标按钮
/// @return 无返回值
void setupColorSwatchButton(QPushButton *button);

/// @brief 刷新颜色胶囊的色块与 Hex 文本
/// @param button 已配置的颜色按钮
/// @param color 当前颜色，无效时使用 fallback
/// @param fallback 颜色无效时显示的备用颜色
/// @return 无返回值
void updateColorSwatchButton(QPushButton *button, const QColor &color, const QColor &fallback);

}  // namespace markshot::settings
