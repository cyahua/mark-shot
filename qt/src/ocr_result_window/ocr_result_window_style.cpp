#include "ocr_result_window/ocr_result_window_style.h"

#include "settings/settings_design_tokens.h"
#include "ui/icons.h"

#include <QPainter>
#include <QPixmap>

namespace markshot::shot {

QString ocrWindowStyleSheet(const QPalette &palette)
{
    // 1. 【OCR】【界面主题】沿用设置界面的调色板与边框色，保留应用的青绿色强调
    const bool light = palette.color(QPalette::Window).lightness() > 128;
    const QString border = (light ? QColor(203, 213, 225) : settings::tokens::kCardBorder).name();
    const QString muted = (light ? QColor(100, 116, 139) : settings::tokens::kTextSecondary).name();
    const QString accentSurface = light ? QStringLiteral("#CCFBF1") : QStringLiteral("#163D40");
    const QString segmentSurface = light ? QStringLiteral("#E8E8E8") : QStringLiteral("#2D2D2D");
    const QString segmentSelected = light ? QStringLiteral("#FFFFFF") : QStringLiteral("#474747");
    const QString error = light ? QStringLiteral("#B91C1C") : QStringLiteral("#FCA5A5");
    return QStringLiteral(
        "QWidget#ocrResultWindow { background: %1; border: 1px solid %7; border-radius: 10px; }"
        "QLabel { background: transparent; color: %3; border: 0; }"
        "QLabel[role=\"muted\"], QLabel#ocrPaneNotice { color: %8; }"
        "QLabel#ocrPaneNotice[error=\"true\"] { color: %11; }"
        "QFrame[ocrPane=\"true\"] { background: transparent; border: 0; }"
        "QTextEdit { background: transparent; color: %3; border: 1px solid transparent;"
        " border-radius: 4px; padding: 2px; selection-background-color: %5; selection-color: %6; }"
        "QTextEdit:focus { border-color: %7; }"
        "QPushButton { background: %4; color: %3; border: 1px solid %7; border-radius: 6px;"
        " padding: 5px 10px; min-height: 18px; }"
        "QPushButton:hover { background: %9; border-color: %5; }"
        "QPushButton:pressed { background: %10; }"
        "QPushButton:focus { border-color: %5; }"
        "QPushButton#ocrCopyButton, QPushButton#ocrUndoButton { padding: 1px 8px; min-height: 0; }"
        "QPushButton[role=\"quiet\"] { background: transparent; border-color: transparent; }"
        "QPushButton[role=\"quiet\"]:hover { background: %9; border-color: %7; }"
        "QPushButton[role=\"quiet\"]:focus { border-color: %5; }"
        "QPushButton[role=\"icon\"] { background: transparent; border: 0; padding: 0; }"
        "QPushButton[role=\"icon\"]:hover { background: %9; border: 0; }"
        "QPushButton[role=\"icon\"]:focus { background: %9; border: 0; }"
        "QPushButton#ocrPinButton:checked { background: %10; border: 0; }"
        "QPushButton#ocrTranslationToggle:checked { background: %10; border: 0; }"
        "QPushButton#ocrMoreButton::menu-indicator { image: none; width: 0; }"
        "QTabBar#ocrViewTabs { background: %13; border: 0; border-radius: 10px; padding: 2px; }"
        "QTabBar#ocrViewTabs::tab { background: transparent; color: %8; border: 0;"
        " border-radius: 8px; padding: 4px 8px; margin: 1px; }"
        "QTabBar#ocrViewTabs::tab:selected { background: %14; color: %3; }"
        "QTabBar#ocrViewTabs::tab:hover { color: %3; }"
        "QTabBar#ocrViewTabs::tab:focus { color: %5; }"
        "QPushButton[role=\"primary\"] { background: %5; color: %6; border-color: %5; font-weight: 600; }"
        "QPushButton[role=\"primary\"]:hover { background: %5; border-color: %3; }"
        "QPushButton[role=\"primary\"]:pressed { background: %9; color: %3; }"
        "QPushButton:disabled, QPushButton[role=\"primary\"]:disabled {"
        " background: %9; color: %8; border-color: %7; }"
        "QComboBox { background: %2; color: %3; border: 1px solid %7; border-radius: 6px;"
        " padding: 5px 28px 5px 10px; min-height: 18px; }"
        "QComboBox:hover, QComboBox:focus { border-color: %5; }"
        "QComboBox:disabled { color: %8; background: %9; }"
        "QComboBox QLineEdit { background: transparent; color: %3; border: 0; padding: 0;"
        " selection-background-color: %5; selection-color: %6; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right;"
        " width: 24px; border: 0; background: transparent; }"
        "QComboBox::down-arrow { image: url(%12); width: 12px; height: 12px; }"
        "QComboBox QAbstractItemView { background: %2; color: %3; border: 1px solid %7;"
        " selection-background-color: %5; selection-color: %6; outline: 0; padding: 4px; }"
        "QFrame#ocrSourcePreview { background: transparent; border: 0; }"
        "QTableWidget#ocrTable { background: %2; color: %3; border: 1px solid %7; border-radius: 6px;"
        " gridline-color: %7; selection-background-color: %5; selection-color: %6; }"
        "QTableWidget#ocrTable QHeaderView::section { background: %9; color: %8; border: 0;"
        " border-right: 1px solid %7; border-bottom: 1px solid %7; padding: 3px 6px; }"
        "QTableWidget#ocrTable QTableCornerButton::section { background: %9; border: 0; }"
        "QPushButton#ocrTableCopyButton::menu-indicator { image: none; width: 0; }"
        "QSplitter::handle { background: transparent; }"
        "QSplitter::handle:hover { background: %10; border-radius: 3px; }"
        "QProgressBar#ocrTranslationProgress { background: %9; border: 0; border-radius: 1px; }"
        "QProgressBar#ocrTranslationProgress::chunk { background: %5; }"
        "QScrollBar:vertical { background: transparent; width: 6px; margin: 2px; }"
        "QScrollBar::handle:vertical { background: transparent; border-radius: 3px; min-height: 24px; }"
        "QTextEdit:hover QScrollBar::handle:vertical,"
        "QScrollBar::handle:vertical:hover { background: %7; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
        .arg(palette.color(QPalette::Window).name(), palette.color(QPalette::Base).name(),
             palette.color(QPalette::Text).name(), palette.color(QPalette::Button).name(),
             palette.color(QPalette::Highlight).name(), palette.color(QPalette::HighlightedText).name(),
             border, muted, palette.color(QPalette::AlternateBase).name())
        .arg(accentSurface, error,
             light ? QStringLiteral(":/icons/chevron-down-light.svg") : QStringLiteral(":/icons/chevron-down.svg"))
        .arg(segmentSurface, segmentSelected);
}

QIcon ocrActionIcon(types::Action action, QColor ink, QColor checkedInk)
{
    // 1. 【OCR】【图标配色】由矢量图标引擎按主题颜色渲染常态、选中态与禁用态
    return markshot::ui::makeToolIcon(action, ink, checkedInk);
}

QIcon ocrTranslationIcon(QColor ink, QColor checkedInk)
{
    QIcon icon;
    // 1. 【OCR】【翻译图标】使用矢量笔画绘制语言转换符号，不依赖平台字体的特殊字形
    for (const auto state : {QIcon::Off, QIcon::On}) {
        QPixmap pixmap(36, 36);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(1.5, 1.5);
        painter.setPen(QPen(state == QIcon::On ? checkedInk : ink, 1.6, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(3, 6), QPointF(14, 6));
        painter.drawLine(QPointF(8, 3), QPointF(8, 6));
        painter.drawLine(QPointF(11, 6), QPointF(4, 15));
        painter.drawLine(QPointF(5, 9), QPointF(11, 15));
        painter.drawLine(QPointF(12, 21), QPointF(17, 10));
        painter.drawLine(QPointF(17, 10), QPointF(22, 21));
        painter.drawLine(QPointF(14, 17), QPointF(20, 17));
        painter.end();
        pixmap.setDevicePixelRatio(2);
        icon.addPixmap(pixmap, QIcon::Normal, state);
    }
    return icon;
}

}
