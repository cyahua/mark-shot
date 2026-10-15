#include "ocr_result_window/ocr_result_window_style.h"
#include "scroll/scroll_preview_chrome.h"

#include <QImage>
#include <QPushButton>
#include <QTabBar>
#include <QVBoxLayout>
#include <QtTest/QtTest>

class OcrScrollWindowStyleTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 提供明暗两种应用调色板，确保灰色胶囊不退化亮色主题
    /// @return 无返回值
    void preservesTheme_data()
    {
        QTest::addColumn<bool>("light");
        QTest::newRow("light") << true;
        QTest::newRow("dark") << false;
    }

    /// @brief 验证窗口配色、灰色胶囊、无边框图标与置顶浅底激活规则
    /// @return 无返回值
    void preservesTheme()
    {
        QFETCH(bool, light);
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(light ? "#F8FAFC" : "#0F172A"));
        palette.setColor(QPalette::Base, QColor(light ? "#FFFFFF" : "#172033"));
        palette.setColor(QPalette::Text, QColor(light ? "#0F172A" : "#E2E8F0"));
        palette.setColor(QPalette::Button, QColor(light ? "#FFFFFF" : "#1E293B"));
        palette.setColor(QPalette::AlternateBase, QColor(light ? "#E2E8F0" : "#1E293B"));
        palette.setColor(QPalette::Highlight, QColor(light ? "#0D9488" : "#5EEAD4"));
        const QString style = markshot::shot::ocrWindowStyleSheet(palette);
        QVERIFY(style.contains("background: " + palette.color(QPalette::Window).name()));
        QVERIFY(style.contains("QTabBar#ocrViewTabs { background: "
                                + QString(light ? "#E8E8E8" : "#2D2D2D")));
        QVERIFY(style.contains("QTabBar#ocrViewTabs::tab:selected { background: "
                                + QString(light ? "#FFFFFF" : "#474747")));
        QVERIFY(style.contains("QPushButton[role=\"icon\"] { background: transparent; border: 0;"));
        QVERIFY(style.contains("QPushButton#ocrPinButton:checked { background: "
                                + QString(light ? "#CCFBF1" : "#163D40") + "; border: 0;"));
        QVERIFY(!style.contains("QPushButton#ocrPinButton:checked { background: "
                                 + palette.color(QPalette::Highlight).name()));
        const QIcon translate = markshot::shot::ocrTranslationIcon(
            palette.color(QPalette::Text), palette.color(QPalette::Highlight));
        QVERIFY(!translate.isNull());
        QVERIFY(translate.pixmap(18, 18, QIcon::Normal, QIcon::Off).toImage()
                != translate.pixmap(18, 18, QIcon::Normal, QIcon::On).toImage());
    }

    /// @brief 验证深灰背景、细边框及青点白字状态行，无整行青色填充
    /// @return 无返回值
    void paintsCompactScrollStatus()
    {
        QImage image(240, 80, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            markshot::scroll::drawScrollPreviewSurface(painter, image.rect());
            markshot::scroll::drawScrollPreviewStatus(painter, QRect(12, 10, 216, 22),
                                                      QStringLiteral("Capturing"));
        }
        QCOMPARE(image.pixelColor(120, 60), QColor("#1E1E1E"));
        QCOMPARE(image.pixelColor(0, 40), QColor("#3A3A3A"));
        QCOMPARE(image.pixelColor(16, 20), QColor("#2DD4BF"));
        int tealPixels = 0;
        int whitePixels = 0;
        for (int y = 10; y < 32; ++y) {
            for (int x = 12; x < 228; ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.green() > 170 && pixel.red() < 100) {
                    ++tealPixels;
                }
                if (pixel.red() > 200 && pixel.green() > 200 && pixel.blue() > 200) {
                    ++whitePixels;
                }
            }
        }
        QVERIFY(tealPixels > 10);
        QVERIFY(tealPixels < 50);
        QVERIFY(whitePixels > 10);
        QCOMPARE(image.pixelColor(210, 20), QColor("#1E1E1E"));
    }
};

QTEST_MAIN(OcrScrollWindowStyleTest)

#include "ocr_scroll_window_style_test.moc"
