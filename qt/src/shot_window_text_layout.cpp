#include "shot_window_module.h"

using namespace markshot::shot;

/**
 * 计算文字标注在图像或窗口坐标中的内容边界。
 * @param annotation 需要测量的文字标注。
 * @param widgetCoordinates 是否返回窗口坐标。
 * @return 包含文字内容与背景内边距的边界矩形。
 */
QRectF ShotWindow::textContentRect(const Annotation &annotation, bool widgetCoordinates) const
{
    const qreal scale = annotationSizeScale(widgetCoordinates);
    const QRectF baseRect = annotation.rect.isEmpty()
        ? QRectF(annotation.points.value(0), QSizeF(360.0, 140.0))
        : annotation.rect.normalized();
    const QPointF topLeft = widgetCoordinates ? imageToWidget(baseRect.topLeft()) : baseRect.topLeft();

    QFont font = markshot::theme::textFont(qRound((19.0 + annotation.width) * scale),
                                           annotation.fontWeight,
                                           annotation.fontFamily);
    font.setItalic(annotation.textItalic);
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    option.setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QTextDocument document;
    document.setDocumentMargin(0.0);
    document.setDefaultFont(font);
    document.setDefaultTextOption(option);
    document.setPlainText(annotation.text);
    // 1. 先按单行测量。编辑器宽度不含内边距，再扣边距会把一行挤成两行
    document.setTextWidth(-1);
    const qreal naturalWidth = std::max<qreal>(1.0, document.idealWidth());
    const qreal boxWidth = baseRect.width() * scale;
    if (boxWidth >= 16.0 && boxWidth + 1.0 < naturalWidth) {
        document.setTextWidth(std::max<qreal>(16.0, boxWidth - kTextBackgroundPaddingX * 2.0 * scale));
    }

    qreal textWidth = 0.0;
    qreal textHeight = 0.0;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        const QTextLayout *layout = block.layout();
        if (!layout) {
            continue;
        }
        for (int i = 0; i < layout->lineCount(); ++i) {
            const QTextLine line = layout->lineAt(i);
            textWidth = std::max(textWidth, line.naturalTextWidth());
            textHeight = std::max(textHeight, layout->position().y() + line.y() + line.height());
        }
    }
    if (textWidth <= 0.0 || textHeight <= 0.0) {
        const QSizeF documentSize = document.size();
        textWidth = documentSize.width();
        textHeight = documentSize.height();
    }

    // 2. 宽度只保留左右对称内边距，避免选区右侧多出一段空白
    const qreal rectWidth =
        std::max<qreal>(1.0, std::ceil(textWidth + kTextBackgroundPaddingX * 2.0 * scale));
    const qreal rectHeight =
        std::max<qreal>(1.0, std::ceil(textHeight + kTextBackgroundPaddingY * 2.0 * scale));
    return QRectF(topLeft, QSizeF(rectWidth, rectHeight));
}
