#include "capture_cross_cursor.h"
#include "selection_loupe.h"

#include <QImage>
#include <QPainter>
#include <QtTest>

using namespace markshot::shot;

/// @brief 验证系统光标与软件指针共用图案，同时保留硬件缓冲区对齐约束
class CaptureCrossCursorTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 提供整数与小数输出缩放，检查资源倍率向上取整
    void keepsAlignedHardwareBuffer_data()
    {
        QTest::addColumn<qreal>("requestedDpr");
        QTest::addColumn<int>("resourceScale");
        QTest::newRow("one-x") << 1.0 << 1;
        QTest::newRow("one-and-quarter-x") << 1.25 << 2;
        QTest::newRow("one-and-half-x") << 1.5 << 2;
        QTest::newRow("two-x") << 2.0 << 2;
    }

    /// @brief 检查不同输出缩放下的硬件光标行步长与热点合法性
    void keepsAlignedHardwareBuffer()
    {
        QFETCH(qreal, requestedDpr);
        QFETCH(int, resourceScale);
        const QCursor cursor = captureCrossCursor(requestedDpr);
        const QPixmap pixmap = cursor.pixmap();
        const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
        QVERIFY(!image.isNull());
        QCOMPARE(pixmap.devicePixelRatio(), static_cast<qreal>(resourceScale));
        QCOMPARE(image.size(), QSize(64 * resourceScale, 64 * resourceScale));
        QCOMPARE(pixmap.deviceIndependentSize(), QSizeF(64, 64));
        QCOMPARE(image.bytesPerLine() % 256, 0);
        // QCursor 的热点使用逻辑坐标（surface-local 坐标），应始终位于 64×64 逻辑画布中心 (32, 32)
        const QSize logicalSize = pixmap.deviceIndependentSize().toSize();
        QCOMPARE(cursor.hotSpot(), QPoint(32, 32));
        QCOMPARE(cursor.hotSpot(), QPoint(logicalSize.width() / 2, logicalSize.height() / 2));
        QVERIFY(image.rect().contains(cursor.hotSpot() * resourceScale));
    }

    /// @brief 提供整数与小数输出缩放，比较软硬件十字图案
    void softwarePointerMatchesHardwareCursor_data()
    {
        keepsAlignedHardwareBuffer_data();
    }

    /// @brief 比较不同输出缩放下的软件指针与硬件 cursor 图案
    void softwarePointerMatchesHardwareCursor()
    {
        QFETCH(qreal, requestedDpr);
        QFETCH(int, resourceScale);
        const QCursor cursor = captureCrossCursor(requestedDpr);
        const QPixmap pixmap = cursor.pixmap();
        QCOMPARE(pixmap.devicePixelRatio(), static_cast<qreal>(resourceScale));
        const QImage expected = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QImage actual(expected.size(), QImage::Format_ARGB32_Premultiplied);
        actual.setDevicePixelRatio(resourceScale);
        actual.fill(Qt::transparent);
        QPainter painter(&actual);
        drawSelectionPointer(painter, QPointF(cursor.hotSpot()));
        painter.end();
        QCOMPARE(actual, expected);
    }
};

QTEST_MAIN(CaptureCrossCursorTest)
#include "capture_cross_cursor_test.moc"
