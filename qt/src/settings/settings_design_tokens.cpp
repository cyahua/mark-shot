#include "settings/settings_design_tokens.h"

namespace markshot::settings::tokens {

QString settingsStyleSheet()
{
    return settingsStyleSheet(markshot::ui::UiThemeMode::Dark);
}

QString settingsStyleSheet(markshot::ui::UiThemeMode mode)
{
    // 1. 【界面】【视觉层级】同一底色承载内容，以留白和字重分组，强调色只用于操作状态
    const QPalette colors = settingsPalette(mode);
    const bool light = mode == markshot::ui::UiThemeMode::Light;
    const QString muted = light ? QStringLiteral("#64748B") : kTextSecondary.name();
    const QString border = light ? QStringLiteral("#CBD5E1") : kCardBorder.name();
    const QString hover = light ? QStringLiteral("#EEF2F6") : QStringLiteral("#1E293B");
    const QString selected = light ? QStringLiteral("#E0F2EF") : QStringLiteral("#173936");
    const QString error = light ? QStringLiteral("#B91C1C") : QStringLiteral("#FCA5A5");
    // 2. 【界面】【分类卡片】卡片表面比窗口底色略亮，细边框区分分组，输入控件仍保留自身边框
    const QString cardSurface = light ? QStringLiteral("#FFFFFF") : QStringLiteral("#152033");
    const QString cardBorder = light ? QStringLiteral("#E2E8F0") : QStringLiteral("#233044");
    // 3. 【界面】【快捷键胶囊】浅色底加细边形成输入边界，录入时边框切换为强调色
    const QString capsule = light ? QStringLiteral("#F3F4F6") : QStringLiteral("#1E293B");
    const QString capsuleBorder = light ? QStringLiteral("#E5E7EB") : QStringLiteral("#334155");
    return QStringLiteral(
        "QDialog#settingsDialog, QDialog#recordingConfigDialog { background: %1; color: %2; }"
        "QWidget#settingsSidebar, QFrame#settingsSidebar { background: transparent; border: 0; }"
        "QFrame#settingsFooter { background: %1; border-top: 1px solid %4; }"
        "QLabel { color: %2; background: transparent; }"
        "QLabel#settingsHeroTitle, QLabel#settingsCardTitle { font-weight: 600; }"
        "QLabel#settingsStatus, QLabel#settingsCardDescription, QLabel[role=\"muted\"] { color: %3; }"
        "QLabel#settingsStatus[tone=\"error\"], QLabel[role=\"error\"] { color: %11; }"
        "QListWidget#settingsNavigation { background: transparent; border: 0; padding: 0; outline: 0; }"
        "QListWidget#settingsNavigation::item { color: %3; border-radius: 6px; padding: 0 8px; margin: 0; }"
        "QListWidget#settingsNavigation::item:hover { background: %9; color: %2; }"
        "QListWidget#settingsNavigation::item:selected { background: %10; color: %2; }"
        "QListWidget#settingsNavigation::item:disabled { background: transparent; color: transparent; }"
        "QFrame#settingsCard { background: %15; border: 1px solid %16; border-radius: 10px; }"
        "QFrame#settingsCard QFrame#disclosureSection, QFrame#disclosureSection { background: transparent; border: 0; }"
        "QWidget#settingsRowLabel { background: transparent; }"
        "QLabel#settingsRowTitle { color: %2; }"
        "QLabel#settingsRowDescription { color: %3; font-size: 9pt; }"
        "QLabel#settingsRowTitle:disabled, QLabel#settingsRowDescription:disabled { color: %3; }"
        "QWidget#pluginDiagnosticsViewport { background: transparent; }"
        "QFrame#pluginDiagnosticItem { background: transparent; border: 0; }"
        "QLabel#pluginDiagnosticProvider { color: %2; font-weight: 600; }"
        "QLabel#pluginDiagnosticMeta, QLabel#pluginDiagnosticFieldTitle, QLabel#pluginDiagnosticEmpty { color: %3; }"
        "QLabel#pluginDiagnosticFieldValue { color: %2; }"
        "QLabel#pluginDiagnosticStatus { color: %3; border: 0; padding: 2px 4px; }"
        "QLabel#pluginDiagnosticStatus[tone=\"success\"] { color: %7; }"
        "QLabel#pluginDiagnosticStatus[tone=\"error\"] { color: %11; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QKeySequenceEdit, QPlainTextEdit {"
        " min-height: 24px; border: 1px solid %4; border-radius: 6px; padding: 3px 8px;"
        " background: %5; color: %2; selection-background-color: %7; selection-color: %8; }"
        "QKeySequenceEdit QLineEdit { border: 0; padding: 0; background: transparent; }"
        "QKeySequenceEdit[settingsShortcut=\"true\"] { min-height: 26px; padding: 1px 4px 1px 8px;"
        " border: 1px solid %18; border-radius: 4px; background: %17; }"
        "QKeySequenceEdit[settingsShortcut=\"true\"]:hover { border-color: %3; }"
        "QKeySequenceEdit[settingsShortcut=\"true\"][recording=\"true\"] { border-color: %7; }"
        "QKeySequenceEdit[settingsShortcut=\"true\"]:disabled { background: %9; }"
        "QKeySequenceEdit[settingsShortcut=\"true\"] QLineEdit { min-height: 22px; font-weight: 500; }"
        "QPlainTextEdit#settingsEnvironmentEdit { padding: 2px 6px; }"
        "QPushButton#settingsColorButton { text-align: left; padding: 3px 10px; }"
        "QPushButton#settingsPathBrowse { padding: 3px 10px; }"
        "QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover,"
        " QKeySequenceEdit:hover, QPlainTextEdit:hover { border-color: %3; }"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus,"
        " QKeySequenceEdit:focus, QPlainTextEdit:focus { border-color: %7; }"
        "QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled,"
        " QKeySequenceEdit:disabled, QPlainTextEdit:disabled { color: %3; background: %9; }"
        "QComboBox { padding-right: 26px; }"
        "QComboBox QAbstractItemView { background: %5; color: %2; border: 1px solid %4;"
        " selection-background-color: %10; selection-color: %2; outline: 0; padding: 4px; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right;"
        " width: 22px; border: 0; background: transparent; }"
        "QComboBox::down-arrow { image: url(%13); width: 12px; height: 12px; }"
        "QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-origin: content;"
        " subcontrol-position: top right; width: 20px; border: 0; background: transparent; }"
        "QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-origin: content;"
        " subcontrol-position: bottom right; width: 20px; border: 0; background: transparent; }"
        "QSpinBox::up-arrow, QDoubleSpinBox::up-arrow { image: url(%14); width: 11px; height: 11px; }"
        "QSpinBox::down-arrow, QDoubleSpinBox::down-arrow { image: url(%13); width: 11px; height: 11px; }"
        "QCheckBox { color: %2; spacing: 8px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %4; border-radius: 4px; background: %5; }"
        "QCheckBox::indicator:hover { border-color: %7; }"
        "QCheckBox::indicator:checked { background: %7; border-color: %7; image: url(%12); }"
        "QCheckBox::indicator:disabled { background: %9; }"
        "QPushButton { min-height: 24px; border-radius: 6px; border: 1px solid %4;"
        " padding: 4px 12px; background: %6; color: %2; font-weight: 500; }"
        "QPushButton:hover, QPushButton:focus { border-color: %7; }"
        "QPushButton:pressed { background: %9; }"
        "QPushButton:disabled { color: %3; border-color: %4; background: %9; }"
        "QPushButton[role=\"primary\"] { background: %7; color: %8; border-color: %7; font-weight: 600; }"
        "QPushButton[role=\"primary\"]:hover { border-color: %2; }"
        "QPushButton[role=\"primary\"]:disabled { background: %9; color: %3; border-color: %4; }"
        "QPushButton[role=\"quiet\"] { background: transparent; border-color: transparent; }"
        "QPushButton[role=\"quiet\"]:hover, QPushButton[role=\"quiet\"]:focus { background: %9; border-color: %4; }"
        "QToolButton[disclosureHeader=\"true\"] { border: 1px solid transparent; border-radius: 4px;"
        " background: transparent; color: %3; padding: 4px 2px; }"
        "QToolButton[disclosureHeader=\"true\"]:hover, QToolButton[disclosureHeader=\"true\"]:checked { color: %2; }"
        "QToolButton[disclosureHeader=\"true\"]:focus { border-color: %7; }"
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 1px; }"
        "QScrollBar::handle:vertical { background: %4; border-radius: 3px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: %3; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QMenu { background: %5; color: %2; border: 1px solid %4; padding: 4px; }"
        "QMenu::item { padding: 5px 22px; }"
        "QMenu::item:selected { background: %10; }"
        "QMenu::item:disabled { color: %3; }")
        .arg(colors.color(QPalette::Window).name(), colors.color(QPalette::WindowText).name(),
             muted, border, colors.color(QPalette::Base).name(), colors.color(QPalette::Button).name(),
             colors.color(QPalette::Highlight).name(), colors.color(QPalette::HighlightedText).name(), hover)
        .arg(selected, error,
             light ? QStringLiteral(":/icons/check-light.svg") : QStringLiteral(":/icons/check-dark.svg"),
             light ? QStringLiteral(":/icons/chevron-down-light.svg") : QStringLiteral(":/icons/chevron-down.svg"),
             light ? QStringLiteral(":/icons/chevron-up-light.svg") : QStringLiteral(":/icons/chevron-up.svg"))
        .arg(cardSurface, cardBorder)
        .arg(capsule, capsuleBorder);
}

QPalette settingsPalette(markshot::ui::UiThemeMode mode)
{
    QPalette pal;
    if (mode == markshot::ui::UiThemeMode::Light) {
        pal.setColor(QPalette::Window, QColor(248, 250, 252));
        pal.setColor(QPalette::WindowText, QColor(15, 23, 42));
        pal.setColor(QPalette::Base, QColor(255, 255, 255));
        pal.setColor(QPalette::AlternateBase, QColor(226, 232, 240));
        pal.setColor(QPalette::Text, QColor(15, 23, 42));
        pal.setColor(QPalette::Button, QColor(255, 255, 255));
        pal.setColor(QPalette::ButtonText, QColor(15, 23, 42));
        pal.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
        pal.setColor(QPalette::ToolTipText, QColor(15, 23, 42));
        pal.setColor(QPalette::BrightText, QColor(13, 148, 136));
        pal.setColor(QPalette::Link, QColor(13, 148, 136));
        pal.setColor(QPalette::Highlight, QColor(13, 148, 136));
        pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        return pal;
    }

    pal.setColor(QPalette::Window, kWindowBackground);
    pal.setColor(QPalette::WindowText, kTextPrimary);
    pal.setColor(QPalette::Base, kInputBackground);
    pal.setColor(QPalette::AlternateBase, kCardSurface);
    pal.setColor(QPalette::Text, kTextPrimary);
    pal.setColor(QPalette::Button, kCardSurface);
    pal.setColor(QPalette::ButtonText, kTextPrimary);
    pal.setColor(QPalette::ToolTipBase, kCardSurface);
    pal.setColor(QPalette::ToolTipText, kTextPrimary);
    pal.setColor(QPalette::BrightText, kAccent);
    pal.setColor(QPalette::Link, kAccent);
    pal.setColor(QPalette::Highlight, kAccent);
    pal.setColor(QPalette::HighlightedText, kAccentInk);
    return pal;
}

}  // namespace markshot::settings::tokens
