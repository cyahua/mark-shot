#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace markshot::ui {

/// @brief SVG 图标在不同交互状态下使用的着色方案
struct SvgIconPalette {
    /// @brief 常态颜色
    QColor normal;
    /// @brief 选中态颜色，无效时沿用常态颜色
    QColor checked;
    /// @brief 禁用态颜色，无效时由常态颜色降低透明度得到
    QColor disabled;
};

/**
 * 判断统一图标资源中是否存在指定名称的 SVG
 * @param name 图标名称，不含扩展名，例如 tool-pen
 * @return 资源存在时返回 true
 */
bool hasSvgIcon(const QString &name);

/**
 * 读取图标 SVG 源文本，并将 currentColor 替换为指定颜色
 * @param name 图标名称，不含扩展名
 * @param color 图标着色颜色
 * @return 着色后的 SVG 文本，资源不存在时返回空字节数组
 */
QByteArray tintedSvgData(const QString &name, const QColor &color);

/**
 * 将指定 SVG 图标渲染为位图
 * @param name 图标名称，不含扩展名
 * @param color 图标着色颜色
 * @param logicalSize 逻辑像素尺寸
 * @param devicePixelRatio 设备像素比，用于高分屏清晰渲染
 * @return 渲染结果，资源不存在时返回空位图
 */
QPixmap renderSvgIcon(const QString &name, const QColor &color, QSize logicalSize, qreal devicePixelRatio = 1.0);

/**
 * 创建按需矢量渲染的主题图标，任意尺寸与缩放比例都直接从 SVG 绘制
 * @param name 图标名称，不含扩展名
 * @param palette 常态、选中态与禁用态着色方案
 * @return 可直接设置到按钮、菜单项的图标
 */
QIcon makeSvgIcon(const QString &name, const SvgIconPalette &palette);

/**
 * 使用单一颜色创建主题图标，禁用态自动降低透明度
 * @param name 图标名称，不含扩展名
 * @param color 常态与选中态颜色
 * @return 可直接设置到按钮、菜单项的图标
 */
QIcon makeSvgIcon(const QString &name, const QColor &color);

}  // namespace markshot::ui
