#pragma once

#include "annotation_state_store.h"

#include <QJsonObject>

namespace markshot::rust {

/** 将 state 转为 Rust 样式协议对象，返回完整字段映射，不进行业务校验。 */
inline QJsonObject encodeAnnotationStyle(const AnnotationState &state)
{
    return {
        {QStringLiteral("currentColor"), state.currentColor.name(QColor::HexArgb)},
        {QStringLiteral("strokeWidth"), state.strokeWidth},
        {QStringLiteral("highlighterWidth"), state.highlighterWidth},
        {QStringLiteral("numberWidth"), state.numberWidth},
        {QStringLiteral("textSize"), state.textSize},
        {QStringLiteral("mosaicBlockSize"), state.mosaicBlockSize},
        {QStringLiteral("mosaicStyle"), static_cast<int>(state.mosaicStyle)},
        {QStringLiteral("shapeFilled"), state.shapeFilled},
        {QStringLiteral("rectangleCornerRadius"), state.rectangleCornerRadius},
        {QStringLiteral("rectangleStyle"), static_cast<int>(state.rectangleStyle)},
        {QStringLiteral("markerShape"), static_cast<int>(state.markerShape)},
        {QStringLiteral("markerFilled"), state.markerFilled},
        {QStringLiteral("magnifierScale"), state.magnifierScale},
        {QStringLiteral("magnifierShape"), static_cast<int>(state.magnifierShape)},
        {QStringLiteral("arrowStyle"), static_cast<int>(state.arrowStyle)},
        {QStringLiteral("arrowLineStyle"), static_cast<int>(state.arrowLineStyle)},
        {QStringLiteral("highlighterStyle"), static_cast<int>(state.highlighterStyle)},
        {QStringLiteral("numberStyle"), static_cast<int>(state.numberStyle)},
        {QStringLiteral("textFontFamily"), state.textFontFamily},
        {QStringLiteral("textFontWeight"), static_cast<int>(state.textFontWeight)},
        {QStringLiteral("textItalic"), state.textItalic},
        {QStringLiteral("textBackgroundColor"), state.textBackgroundColor.name(QColor::HexArgb)},
    };
}

/** 将 Rust 已校验的 root 转为 Qt 样式，返回完整状态；缺省值与范围处理由 Rust 负责。 */
inline AnnotationState decodeAnnotationStyle(const QJsonObject &root)
{
    AnnotationState state;
    state.currentColor = QColor(root.value(QStringLiteral("currentColor")).toString());
    state.strokeWidth = root.value(QStringLiteral("strokeWidth")).toDouble();
    state.highlighterWidth = root.value(QStringLiteral("highlighterWidth")).toDouble();
    state.numberWidth = root.value(QStringLiteral("numberWidth")).toDouble();
    state.textSize = root.value(QStringLiteral("textSize")).toDouble();
    state.mosaicBlockSize = root.value(QStringLiteral("mosaicBlockSize")).toDouble();
    state.mosaicStyle = static_cast<ShotWindow::MosaicStyle>(root.value(QStringLiteral("mosaicStyle")).toInt());
    state.shapeFilled = root.value(QStringLiteral("shapeFilled")).toBool();
    state.rectangleCornerRadius = root.value(QStringLiteral("rectangleCornerRadius")).toDouble();
    state.rectangleStyle = static_cast<ShotWindow::RectangleStyle>(root.value(QStringLiteral("rectangleStyle")).toInt());
    state.markerShape = static_cast<ShotWindow::MarkerShape>(root.value(QStringLiteral("markerShape")).toInt());
    state.markerFilled = root.value(QStringLiteral("markerFilled")).toBool();
    state.magnifierScale = root.value(QStringLiteral("magnifierScale")).toDouble();
    state.magnifierShape = static_cast<ShotWindow::MagnifierShape>(root.value(QStringLiteral("magnifierShape")).toInt());
    state.arrowStyle = static_cast<ShotWindow::ArrowStyle>(root.value(QStringLiteral("arrowStyle")).toInt());
    state.arrowLineStyle = static_cast<ShotWindow::ArrowLineStyle>(root.value(QStringLiteral("arrowLineStyle")).toInt());
    state.highlighterStyle = static_cast<ShotWindow::HighlighterStyle>(root.value(QStringLiteral("highlighterStyle")).toInt());
    state.numberStyle = static_cast<ShotWindow::NumberStyle>(root.value(QStringLiteral("numberStyle")).toInt());
    state.textFontFamily = root.value(QStringLiteral("textFontFamily")).toString();
    state.textFontWeight = static_cast<QFont::Weight>(root.value(QStringLiteral("textFontWeight")).toInt());
    state.textItalic = root.value(QStringLiteral("textItalic")).toBool();
    state.textBackgroundColor = QColor(root.value(QStringLiteral("textBackgroundColor")).toString());
    return state;
}

} // namespace markshot::rust
