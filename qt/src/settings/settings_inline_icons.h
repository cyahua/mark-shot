#pragma once

#include <QColor>
#include <QIcon>

namespace markshot::settings {

/// @brief 设置字段使用的内置矢量图标
enum class SettingsGlyph {
    /// @brief 清除（叉号）
    Clear,
    /// @brief 文件夹
    Folder,
    /// @brief 恢复（逆时针箭头）
    Reset,
};

/// @brief 生成设置字段图标，与 /icons/ui 一致采用 24 网格、1.75 描边
/// @param glyph 图标类型
/// @param color 常态描边颜色，禁用态自动降低透明度
/// @return 包含 1x 与 2x 位图的图标
QIcon settingsGlyphIcon(SettingsGlyph glyph, const QColor &color);

}  // namespace markshot::settings
