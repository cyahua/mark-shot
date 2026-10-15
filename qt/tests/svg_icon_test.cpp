#include "ui/icon_manifest.h"
#include "ui/icons.h"
#include "ui/svg_icon.h"

#include <QApplication>
#include <QImage>
#include <QtTest>

namespace {

/**
 * 统计位图中可见像素数量
 * @param pixmap 待检查位图
 * @return alpha 大于 0 的像素数量
 */
int visiblePixelCount(const QPixmap &pixmap)
{
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                ++count;
            }
        }
    }
    return count;
}

/**
 * 返回位图中最不透明像素的颜色
 * @param pixmap 待检查位图
 * @return 不透明度最高的像素颜色
 */
QColor strongestPixel(const QPixmap &pixmap)
{
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    QRgb best = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > qAlpha(best)) {
                best = image.pixel(x, y);
            }
        }
    }
    return QColor::fromRgba(best);
}

}  // namespace

/// @brief 统一 SVG 图标资源、映射与着色测试
class SvgIconTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证所有工具栏动作都映射到存在且可渲染的 SVG 资源
     */
    void everyActionHasRenderableSvg()
    {
        for (int raw = static_cast<int>(ShotWindow::Action::ToolMove);
             raw <= static_cast<int>(ShotWindow::Action::Cancel); ++raw) {
            const auto action = static_cast<ShotWindow::Action>(raw);
            const QString name = markshot::ui::toolIconName(action);
            QVERIFY2(markshot::ui::hasSvgIcon(name), qPrintable(name));
            const QPixmap pixmap = markshot::ui::makeToolIcon(action).pixmap(QSize(24, 24));
            QVERIFY2(visiblePixelCount(pixmap) > 10, qPrintable(name));
        }
    }

    /**
     * 验证所有属性面板图标都映射到可渲染的 SVG 资源
     */
    void everyPropertyIconHasRenderableSvg()
    {
        for (int raw = static_cast<int>(markshot::ui::PropertyIcon::StrokeWidth);
             raw <= static_cast<int>(markshot::ui::PropertyIcon::ResetNumber); ++raw) {
            const auto icon = static_cast<markshot::ui::PropertyIcon>(raw);
            const QString name = markshot::ui::propertyIconName(icon);
            QVERIFY2(markshot::ui::hasSvgIcon(name), qPrintable(name));
            QVERIFY2(visiblePixelCount(markshot::ui::makePropertyIcon(icon).pixmap(QSize(20, 20))) > 10,
                     qPrintable(name));
        }
    }

    /**
     * 验证图标按主题颜色着色，选中态与禁用态使用各自颜色
     */
    void iconsAreTintedPerState()
    {
        const QColor ink(15, 23, 42);
        const QColor checked(20, 184, 166);
        const QIcon icon = markshot::ui::makeToolIcon(ShotWindow::Action::ToolRectangle, ink, checked);

        const QColor normal = strongestPixel(icon.pixmap(QSize(32, 32), QIcon::Normal, QIcon::Off));
        QCOMPARE(normal.rgb(), ink.rgb());
        const QColor on = strongestPixel(icon.pixmap(QSize(32, 32), QIcon::Normal, QIcon::On));
        QCOMPARE(on.rgb(), checked.rgb());
        const QColor disabled = strongestPixel(icon.pixmap(QSize(32, 32), QIcon::Disabled, QIcon::Off));
        QVERIFY(disabled.alpha() < 128);
    }

    /**
     * 验证高分屏位图按设备像素比渲染
     */
    void rendersAtDevicePixelRatio()
    {
        const QPixmap pixmap = markshot::ui::renderSvgIcon(QStringLiteral("check"), Qt::white, QSize(16, 16), 2.0);
        QCOMPARE(pixmap.size(), QSize(32, 32));
        QCOMPARE(pixmap.devicePixelRatio(), 2.0);
    }

    /**
     * 验证缺失资源返回空图标而不是错误图形
     */
    void missingIconIsNull()
    {
        QVERIFY(!markshot::ui::hasSvgIcon(QStringLiteral("not-existing-icon")));
        QVERIFY(markshot::ui::makeSvgIcon(QStringLiteral("not-existing-icon"), Qt::white).isNull());
    }
};

QTEST_MAIN(SvgIconTest)
#include "svg_icon_test.moc"
