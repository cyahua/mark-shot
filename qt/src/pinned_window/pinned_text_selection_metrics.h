#pragma once

#include <QChar>
#include <QRectF>
#include <QString>

namespace markshot::shot {

/**
 * 计算文本选择拆分时使用的显示列宽权重。
 * @param character 输入字符。
 * @return 半角字符返回 1，全角字符返回 2，零宽字符返回 0。
 */
qreal pinnedTextSelectionCharacterWeight(QChar character);

/**
 * 计算文本选择拆分时使用的显示列宽权重总和。
 * @param text 输入文本。
 * @return 文本显示列宽权重。
 */
qreal pinnedTextSelectionTextWeight(const QString &text);

/**
 * 根据文本显示列宽补正鼠标选中文本的背景矩形。
 * @param tokenRect OCR token 原始矩形。
 * @param text token 文本。
 * @return 用于绘制和命中的选区矩形。
 */
QRectF pinnedTextSelectionHighlightRect(QRectF tokenRect, const QString &text);

/**
 * 按显示列宽切出 token 中单个字符的图像矩形。
 * @param tokenRect OCR token 原始矩形。
 * @param text token 文本。
 * @param characterIndex 字符下标，从 0 开始。
 * @return 该字符的矩形；下标无效时返回空矩形。末字右缘贴齐 token 右缘。
 */
QRectF pinnedTextSelectionCharacterRect(const QRectF &tokenRect, const QString &text, int characterIndex);

}  // namespace markshot::shot
