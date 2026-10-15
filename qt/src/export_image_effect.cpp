#include "export_image_effect.h"

#include "image_bridge.h"
#include "json_bridge.h"
#include "shot_export_ffi.h"

#include <QColorSpace>

namespace markshot {
namespace {

/** 将 config 的 Qt 类型映射到 ABI 参数，返回完整外框选项。 */
ShotExportConfig frameOptions(const ExportImageEffectConfig &config)
{
    return {config.enabled ? 1 : 0, config.padding, config.shadowRadius, config.shadowOffsetY,
            config.cornerRadius, config.shadowOpacity, config.shadowColor.rgba(), 0};
}

} // namespace

/** 从 root 读取兼容的导出配置，返回 Qt 参数；规则与范围校验由 Rust 配置核心负责。 */
ExportImageEffectConfig exportImageEffectConfigFromRoot(const QJsonObject &root)
{
    ShotExportConfig output{};
    if (!rust::succeeded(shot_export_config(
            rust::byteView(QJsonDocument(root).toJson(QJsonDocument::Compact)), &output))) {
        return {};
    }
    return {output.enabled != 0, output.padding, output.corner_radius, output.shadow_radius,
            output.shadow_offset_y, output.shadow_opacity, QColor::fromRgba(output.shadow_color)};
}

/** 将 source 的物理像素交给 Rust 合成 config 外框，返回保留 DPR 与色彩信息的图像。 */
QImage applyExportImageEffect(const QImage &source, const ExportImageEffectConfig &config)
{
    if (source.isNull()) {
        return {};
    }
    if (!config.enabled) {
        return source.copy();
    }
    const ShotExportConfig options = frameOptions(config);
    ShotSize size{};
    if (!rust::succeeded(shot_export_size({source.width(), source.height()}, options, &size))) {
        return {};
    }
    const QImage content = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage output(size.width, size.height, QImage::Format_ARGB32_Premultiplied);
    if (content.isNull() || output.isNull()) {
        return {};
    }
    if (!rust::succeeded(shot_image_export(rust::imageView(content), options,
            reinterpret_cast<uint32_t *>(output.bits()), static_cast<size_t>(output.sizeInBytes() / 4)))) {
        return {};
    }
    output.setDevicePixelRatio(source.devicePixelRatio());
    output.setColorSpace(source.colorSpace());
    output.setDotsPerMeterX(source.dotsPerMeterX());
    output.setDotsPerMeterY(source.dotsPerMeterY());
    return output;
}

} // namespace markshot
