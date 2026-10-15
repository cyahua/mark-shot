#include "ui/svg_icon.h"

#include <QFile>
#include <QHash>
#include <QIconEngine>
#include <QPainter>
#include <QPixmapCache>
#include <QSvgRenderer>
#include <QtMath>

namespace markshot::ui {

namespace {

/// @brief 统一图标资源前缀
const QString kIconPrefix = QStringLiteral(":/icons/ui/");

/// @brief 禁用态相对常态颜色的透明度
constexpr int kDisabledAlpha = 96;

/**
 * 读取并缓存 SVG 原始文本，避免重复访问资源系统
 * @param name 图标名称，不含扩展名
 * @return SVG 原始文本，资源不存在时为空
 */
QByteArray rawSvgData(const QString &name)
{
    static QHash<QString, QByteArray> cache;
    const auto found = cache.constFind(name);
    if (found != cache.constEnd()) {
        return found.value();
    }
    QFile file(kIconPrefix + name + QStringLiteral(".svg"));
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
    }
    cache.insert(name, data);
    return data;
}

/**
 * 将颜色转换为 SVG 可识别的十六进制颜色，透明度由绘制时整体设置
 * @param color 目标颜色
 * @return 形如 #rrggbb 的颜色文本
 */
QByteArray svgColorValue(const QColor &color)
{
    return color.name(QColor::HexRgb).toLatin1();
}

/**
 * 按图标模式与状态选择着色颜色
 * @param palette 图标着色方案
 * @param mode 图标模式
 * @param state 图标开关状态
 * @return 实际绘制颜色
 */
QColor colorFor(const SvgIconPalette &palette, QIcon::Mode mode, QIcon::State state)
{
    if (mode == QIcon::Disabled) {
        if (palette.disabled.isValid()) {
            return palette.disabled;
        }
        QColor color = palette.normal;
        color.setAlpha(qMin(color.alpha(), kDisabledAlpha));
        return color;
    }
    if (state == QIcon::On && palette.checked.isValid()) {
        return palette.checked;
    }
    return palette.normal;
}

/// @brief 按需从 SVG 绘制的图标引擎，保证所有尺寸与缩放比例都清晰
class SvgIconEngine final : public QIconEngine {
public:
    /**
     * 构造图标引擎
     * @param name 图标名称
     * @param palette 着色方案
     */
    SvgIconEngine(QString name, SvgIconPalette palette)
        : m_name(std::move(name))
        , m_palette(std::move(palette))
    {
    }

    /**
     * 在指定区域绘制图标
     * @param painter 目标画笔
     * @param rect 绘制区域
     * @param mode 图标模式
     * @param state 图标开关状态
     */
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
        painter->drawPixmap(rect, scaledPixmap(rect.size(), mode, state, dpr));
    }

    /**
     * 返回逻辑像素尺寸的位图
     * @param size 逻辑尺寸
     * @param mode 图标模式
     * @param state 图标开关状态
     * @return 渲染位图
     */
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        return scaledPixmap(size, mode, state, 1.0);
    }

    /**
     * 返回指定设备像素比的高清位图
     * @param size 逻辑尺寸
     * @param mode 图标模式
     * @param state 图标开关状态
     * @param scale 设备像素比
     * @return 渲染位图
     */
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override
    {
        return renderSvgIcon(m_name, colorFor(m_palette, mode, state), size, scale);
    }

    /**
     * 矢量图标在任意尺寸下都保持请求尺寸
     * @param size 请求尺寸
     * @return 实际尺寸
     */
    QSize actualSize(const QSize &size, QIcon::Mode, QIcon::State) override
    {
        return size;
    }

    /**
     * 复制图标引擎
     * @return 新的图标引擎实例
     */
    QIconEngine *clone() const override
    {
        return new SvgIconEngine(m_name, m_palette);
    }

    /**
     * 返回图标引擎标识
     * @return 引擎键名
     */
    QString key() const override
    {
        return QStringLiteral("markshot-svg");
    }

    /**
     * 返回图标名称
     * @return 图标名称
     */
    QString iconName() override
    {
        return m_name;
    }

    /**
     * 判断图标是否为空
     * @return 资源不存在时返回 true
     */
    bool isNull() override
    {
        return !hasSvgIcon(m_name);
    }

private:
    /// @brief 图标名称
    QString m_name;
    /// @brief 着色方案
    SvgIconPalette m_palette;
};

}  // namespace

bool hasSvgIcon(const QString &name)
{
    return !rawSvgData(name).isEmpty();
}

QByteArray tintedSvgData(const QString &name, const QColor &color)
{
    QByteArray data = rawSvgData(name);
    if (data.isEmpty()) {
        return data;
    }
    data.replace("currentColor", svgColorValue(color));
    return data;
}

QPixmap renderSvgIcon(const QString &name, const QColor &color, QSize logicalSize, qreal devicePixelRatio)
{
    if (logicalSize.isEmpty() || !color.isValid()) {
        return {};
    }
    const qreal dpr = qMax<qreal>(1.0, devicePixelRatio);
    const QString cacheKey = QStringLiteral("markshot-svg:%1:%2:%3x%4@%5")
                                 .arg(name, color.name(QColor::HexArgb))
                                 .arg(logicalSize.width())
                                 .arg(logicalSize.height())
                                 .arg(dpr);
    QPixmap cached;
    if (QPixmapCache::find(cacheKey, &cached)) {
        return cached;
    }

    // 1. 将 currentColor 替换为目标颜色后加载矢量数据
    const QByteArray data = tintedSvgData(name, color);
    if (data.isEmpty()) {
        return {};
    }
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return {};
    }

    // 2. 按设备像素比放大画布，保证高分屏边缘清晰
    const QSize deviceSize(qCeil(logicalSize.width() * dpr), qCeil(logicalSize.height() * dpr));
    QPixmap pixmap(deviceSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    // 3. 颜色自带透明度时整体作用到图标，保留图标内部的主副层次
    painter.setOpacity(color.alphaF());
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(deviceSize)));
    painter.end();
    pixmap.setDevicePixelRatio(dpr);

    QPixmapCache::insert(cacheKey, pixmap);
    return pixmap;
}

QIcon makeSvgIcon(const QString &name, const SvgIconPalette &palette)
{
    return QIcon(new SvgIconEngine(name, palette));
}

QIcon makeSvgIcon(const QString &name, const QColor &color)
{
    return makeSvgIcon(name, SvgIconPalette{color, color, QColor()});
}

}  // namespace markshot::ui
