#include "ocr_result_window/ocr_text_pane.h"
#include "ui/i18n.h"

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTextEdit>
#include <QtTest/QtTest>

namespace markshot::i18n {

/// @brief 使用英文源串隔离系统语言和用户配置对组件断言的影响
/// @param source 界面源串
/// @return 原始英文文本
QString translate(const QString &source)
{
    return source;
}

}

class OcrTextPaneTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 提供空文本、换行、代理对和组合字符样本
    /// @return 无返回值
    void countsVisibleCharacters_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("characters");
        QTest::addColumn<int>("lines");
        QTest::newRow("empty") << QString() << 0 << 0;
        QTest::newRow("chinese") << QStringLiteral("文字") << 2 << 1;
        QTest::newRow("line-break") << QStringLiteral("A\nB") << 3 << 2;
        QTest::newRow("surrogate-pair") << QString::fromUtf8("\xf0\x9d\x84\x9e") << 1 << 1;
        QTest::newRow("combining-mark") << QStringLiteral("e\u0301") << 1 << 1;
    }

    /// @brief 验证字符统计采用用户可见字素，行数采用原始换行
    /// @return 无返回值
    void countsVisibleCharacters()
    {
        QFETCH(QString, text);
        QFETCH(int, characters);
        QFETCH(int, lines);
        markshot::shot::OcrTextPane pane(QStringLiteral("Source"), {});
        pane.setText(text);
        const auto *statistics = pane.findChild<QLabel *>(QStringLiteral("ocrTextStatistics"));
        QVERIFY(statistics);
        QCOMPARE(statistics->text(), QStringLiteral("%1 characters").arg(characters));
        QCOMPARE(statistics->toolTip(), QStringLiteral("%1 characters · %2 lines").arg(characters).arg(lines));
    }

    /// @brief 验证统计移入窗口状态栏后仍跟随编辑更新，不占用正文标题行
    /// @return 无返回值
    void movesStatisticsToWindowFooter()
    {
        QWidget footer;
        markshot::shot::OcrTextPane pane(QStringLiteral("Recognized"), {});
        auto *original = pane.findChild<QLabel *>(QStringLiteral("ocrTextStatistics"));
        QVERIFY(original->isHidden());
        auto *statistics = pane.takeStatisticsLabel(&footer);
        QCOMPARE(statistics, original);
        QCOMPARE(statistics->parentWidget(), &footer);
        QCOMPARE(statistics->accessibleName(), QStringLiteral("Recognized"));
        QVERIFY(!pane.findChild<QLabel *>(QStringLiteral("ocrTextStatistics")));
        pane.setText(QStringLiteral("A\nB"));
        QCOMPARE(statistics->text(), QStringLiteral("3 characters"));
        QCOMPARE(statistics->toolTip(), QStringLiteral("3 characters · 2 lines"));
        pane.setNotice(QStringLiteral("Source changed"));
        auto *notice = pane.findChild<QLabel *>(QStringLiteral("ocrPaneNotice"));
        QCOMPARE(notice->text(), QStringLiteral("Source changed"));
        QVERIFY(!notice->isHidden());
    }

    /// @brief 验证分区复制按钮始终复制全文并保留空白，不受编辑器选区影响
    /// @return 无返回值
    void copiesFullTextWithWhitespace()
    {
        markshot::shot::OcrTextPane pane(QStringLiteral("Source"), {});
        const QString original = QStringLiteral("  first line\nsecond line  \n");
        pane.setText(original);
        QTextCursor cursor = pane.editor()->textCursor();
        cursor.select(QTextCursor::WordUnderCursor);
        pane.editor()->setTextCursor(cursor);
        auto *copy = pane.findChild<QPushButton *>(QStringLiteral("ocrCopyButton"));
        QVERIFY(copy);
        QSignalSpy copied(&pane, &markshot::shot::OcrTextPane::copyRequested);
        copy->click();
        QCOMPARE(copied.size(), 1);
        QCOMPARE(copied.first().first().toString(), original);
    }

    /// @brief 验证清空文本后复制入口立即禁用，继续编辑后恢复
    /// @return 无返回值
    void disablesCopyForBlankText()
    {
        markshot::shot::OcrTextPane pane(QStringLiteral("Source"), {});
        auto *copy = pane.findChild<QPushButton *>(QStringLiteral("ocrCopyButton"));
        QVERIFY(copy);
        QSignalSpy copied(&pane, &markshot::shot::OcrTextPane::copyRequested);
        pane.setText(QStringLiteral(" \t\n"));
        copy->click();
        QVERIFY(!copy->isEnabled());
        QVERIFY(copied.isEmpty());
        pane.editor()->insertPlainText(QStringLiteral("content"));
        QVERIFY(copy->isEnabled());
    }

    /// @brief 验证翻译期间暂停译文编辑，取消或完成后能够继续输入
    /// @return 无返回值
    void resumesEditingAfterBusyState()
    {
        markshot::shot::OcrTextPane pane(QStringLiteral("Translation"), {});
        pane.setText(QStringLiteral("result"));
        pane.setBusy(true);
        QTest::keyClicks(pane.editor(), "blocked");
        QCOMPARE(pane.text(), QStringLiteral("result"));
        pane.setBusy(false);
        pane.editor()->moveCursor(QTextCursor::End);
        QTest::keyClicks(pane.editor(), " edited");
        QCOMPARE(pane.text(), QStringLiteral("result edited"));
    }

    /// @brief 验证原文页可以去掉标题并把复制按钮交给标题栏，标题行不再占高度
    /// @return 无返回值
    void collapsesHeaderWhenTitleAndCopyLeave()
    {
        markshot::shot::OcrTextPane pane(QStringLiteral("Recognized"), {});
        pane.setText(QStringLiteral("hello"));
        QWidget host;
        auto *copy = pane.takeCopyButton(&host);
        pane.setSectionTitleVisible(false);
        auto *header = pane.findChild<QWidget *>(QStringLiteral("ocrPaneHeader"));
        QVERIFY(header);
        QVERIFY(header->isHidden());
        QCOMPARE(copy->parentWidget(), &host);
        QVERIFY(!copy->text().isEmpty());
    }
};

QTEST_MAIN(OcrTextPaneTest)

#include "ocr_text_pane_test.moc"
