#include "settings/settings_color_button.h"

#include "settings/settings_field_metrics.h"

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>

namespace markshot::settings {
namespace {

/// @brief 色块逻辑边长
constexpr int kSwatchSide = 14;

/// @brief 绘制圆角色块，带细描边以区分与按钮底色接近的颜色
/// @param color 色块颜色
/// @param ratio 设备像素比
/// @return 已设置设备像素比的位图
QPixmap swatchPixmap(const QColor &color, qreal ratio)
{
    QPixmap pixmap(qRound(kSwatchSide * ratio), qRound(kSwatchSide * ratio));
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    // 1. 【设置】【颜色胶囊】半透明描边在明暗主题下都能勾勒色块边界
    painter.setPen(QPen(QColor(100, 116, 139, 160), 1.0));
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(0.5, 0.5, kSwatchSide - 1.0, kSwatchSide - 1.0), 3.5, 3.5);
    painter.end();
    return pixmap;
}

}  // namespace

void setupColorSwatchButton(QPushButton *button)
{
    if (!button) {
        return;
    }
    // 1. 【设置】【颜色胶囊】固定约 120px 宽度，沿用标准按钮的中性底色与边框
    button->setObjectName(QStringLiteral("settingsColorButton"));
    button->setFixedWidth(metrics::kColorFieldWidth);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setCursor(Qt::PointingHandCursor);
    button->setIconSize(QSize(kSwatchSide, kSwatchSide));
    button->setStyleSheet({});
}

void updateColorSwatchButton(QPushButton *button, const QColor &color, const QColor &fallback)
{
    if (!button) {
        return;
    }
    const QColor shown = color.isValid() ? color : fallback;
    // 1. 【设置】【颜色胶囊】Hex 文本使用大写，色块同时生成 1x 与 2x 位图
    button->setText(shown.name(QColor::HexRgb).toUpper());
    QIcon icon;
    icon.addPixmap(swatchPixmap(shown, 1.0));
    icon.addPixmap(swatchPixmap(shown, 2.0));
    button->setIcon(icon);
    button->setProperty("swatchColor", shown);
}

}  // namespace markshot::settings
