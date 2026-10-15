#include "settings/settings_inline_icons.h"

#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace markshot::settings {
namespace {

/// @brief 返回图标路径数据
/// @param glyph 图标类型
/// @return SVG path 元素片段
QByteArray glyphPaths(SettingsGlyph glyph)
{
    switch (glyph) {
    case SettingsGlyph::Clear:
        return QByteArrayLiteral("<path d=\"M7 7l10 10M17 7L7 17\"/>");
    case SettingsGlyph::Folder:
        return QByteArrayLiteral("<path d=\"M3.5 7.5a2 2 0 0 1 2-2h4l2 2.5h7a2 2 0 0 1 2 2v7.5a2 2 0 0 1-2 2h-13"
                                 "a2 2 0 0 1-2-2z\"/>");
    case SettingsGlyph::Reset:
        return QByteArrayLiteral("<path d=\"M5 12a7 7 0 1 0 2.05-4.95\"/><path d=\"M5 4.5V8h3.5\"/>");
    }
    return {};
}

/// @brief 渲染单个尺寸的图标位图
/// @param glyph 图标类型
/// @param color 描边颜色
/// @param side 逻辑边长
/// @param ratio 设备像素比
/// @return 已设置设备像素比的透明位图
QPixmap renderGlyph(SettingsGlyph glyph, const QColor &color, int side, qreal ratio)
{
    // 1. 【设置】【图标】按目标颜色生成 SVG，描边规范与统一图标资源一致
    const QByteArray svg = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" "
                                             "fill=\"none\" stroke=\"")
        + color.name(QColor::HexRgb).toLatin1() + QByteArrayLiteral("\" stroke-opacity=\"")
        + QByteArray::number(color.alphaF(), 'f', 3)
        + QByteArrayLiteral("\" stroke-width=\"1.75\" stroke-linecap=\"round\" stroke-linejoin=\"round\">")
        + glyphPaths(glyph) + QByteArrayLiteral("</svg>");
    // 2. 【设置】【图标】按设备像素比放大绘制，保证高分屏清晰
    QPixmap pixmap(qRound(side * ratio), qRound(side * ratio));
    pixmap.fill(Qt::transparent);
    QSvgRenderer renderer(svg);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();
    pixmap.setDevicePixelRatio(ratio);
    return pixmap;
}

}  // namespace

QIcon settingsGlyphIcon(SettingsGlyph glyph, const QColor &color)
{
    QIcon icon;
    QColor disabled = color;
    disabled.setAlphaF(color.alphaF() * 0.4);
    // 1. 【设置】【图标】常用尺寸分别生成常态与禁用态
    for (const int side : {14, 16}) {
        for (const qreal ratio : {1.0, 2.0}) {
            icon.addPixmap(renderGlyph(glyph, color, side, ratio), QIcon::Normal);
            icon.addPixmap(renderGlyph(glyph, disabled, side, ratio), QIcon::Disabled);
        }
    }
    return icon;
}

}  // namespace markshot::settings
