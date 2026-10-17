#include "pinned_window/pinned_text_selection_metrics.h"

#include <QtTest/QtTest>

using namespace markshot::shot;

class PinnedTextSelectionMetricsTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证 ASCII 字符按单列宽计算。
     * @return 无返回值。
     */
    void returnsSingleWidthForAscii()
    {
        QCOMPARE(pinnedTextSelectionCharacterWeight(QLatin1Char('A')), 1.0);
        QCOMPARE(pinnedTextSelectionCharacterWeight(QLatin1Char(',')), 1.0);
        QCOMPARE(pinnedTextSelectionCharacterWeight(QLatin1Char(' ')), 1.0);
    }

    /**
     * 验证中文字符和全角标点按双列宽计算。
     * @return 无返回值。
     */
    void returnsDoubleWidthForCjkAndFullWidth()
    {
        QCOMPARE(pinnedTextSelectionCharacterWeight(QStringLiteral("中").front()), 2.0);
        QCOMPARE(pinnedTextSelectionCharacterWeight(QStringLiteral("。").front()), 2.0);
        QCOMPARE(pinnedTextSelectionCharacterWeight(QStringLiteral("Ａ").front()), 2.0);
    }

    /**
     * 验证组合标记不额外占用选择背景宽度。
     * @return 无返回值。
     */
    void returnsZeroWidthForCombiningMarks()
    {
        QCOMPARE(pinnedTextSelectionCharacterWeight(QChar(0x0301)), 0.0);
    }

    /**
     * 验证半宽中文选区矩形会按双列宽补齐。
     * @return 无返回值。
     */
    void expandsNarrowFullWidthHighlightRect()
    {
        const QRectF sourceRect(10.0, 20.0, 8.0, 16.0);
        const QRectF highlightRect =
            pinnedTextSelectionHighlightRect(sourceRect, QStringLiteral("中"));

        QCOMPARE(highlightRect.left(), sourceRect.left());
        QCOMPARE(highlightRect.top(), sourceRect.top());
        QVERIFY(highlightRect.width() >= 16.0);
        QCOMPARE(highlightRect.height(), sourceRect.height());
    }

    /**
     * 验证普通英文选区矩形不被补宽。
     * @return 无返回值。
     */
    void keepsAsciiHighlightRectUnchanged()
    {
        const QRectF sourceRect(10.0, 20.0, 8.0, 16.0);
        const QRectF highlightRect =
            pinnedTextSelectionHighlightRect(sourceRect, QStringLiteral("A"));

        QCOMPARE(highlightRect, sourceRect);
    }

    /**
     * 验证词内拖选按列宽切开，末字右缘贴齐原框。
     * @return 无返回值。
     */
    void splitsCharactersToTheTokenEdge()
    {
        const QRectF word(10.0, 4.0, 90.0, 20.0);
        const QString text = QStringLiteral("iW");
        const QRectF first = pinnedTextSelectionCharacterRect(word, text, 0);
        const QRectF second = pinnedTextSelectionCharacterRect(word, text, 1);

        QCOMPARE(first.left(), word.left());
        QCOMPARE(first.width(), 45.0);
        QCOMPARE(second.left(), 55.0);
        QCOMPARE(second.right(), word.right());
        QCOMPARE(first.height(), word.height());
    }

    /**
     * 验证中文占两列、英文占一列时切分位置跟列宽一致。
     * @return 无返回值。
     */
    void weightsFullWidthCharactersWider()
    {
        const QRectF word(0.0, 0.0, 90.0, 18.0);
        const QString text = QStringLiteral("中A");
        const QRectF first = pinnedTextSelectionCharacterRect(word, text, 0);
        const QRectF second = pinnedTextSelectionCharacterRect(word, text, 1);

        QCOMPARE(first.width(), 60.0);
        QCOMPARE(second.left(), 60.0);
        QCOMPARE(second.right(), word.right());
    }
};

QTEST_APPLESS_MAIN(PinnedTextSelectionMetricsTest)

#include "pinned_text_selection_metrics_test.moc"
