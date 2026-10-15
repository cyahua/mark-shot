#include "settings/settings_shortcut_edit.h"

#include "settings/settings_field_metrics.h"
#include "settings/settings_field_texts.h"
#include "settings/settings_inline_icons.h"

#include <QAction>
#include <QEvent>
#include <QLineEdit>
#include <QStyle>

namespace markshot::settings {

SettingsShortcutEdit::SettingsShortcutEdit(QWidget *parent)
    : QKeySequenceEdit(parent)
{
    // 1. 【设置】【快捷键胶囊】自定义 QWidget 子类需开启样式背景，样式表边框与底色才会绘制
    setObjectName(QStringLiteral("settingsShortcutEdit"));
    setProperty("settingsShortcut", true);
    setProperty("recording", false);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_Hover, true);
    setFixedWidth(metrics::kShortcutFieldWidth);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setContextMenuPolicy(Qt::NoContextMenu);

    // 2. 【设置】【快捷键胶囊】内部输入框只负责显示，监听绘制以校正提示文案
    m_display = findChild<QLineEdit *>();
    if (m_display) {
        m_display->setContextMenuPolicy(Qt::NoContextMenu);
        m_display->setCursor(Qt::PointingHandCursor);
        m_display->installEventFilter(this);
    }

    // 3. 【设置】【快捷键胶囊】右侧清除图标默认隐藏，仅悬停或聚焦时出现
    m_clear = new QAction(this);
    m_clear->setObjectName(QStringLiteral("settingsShortcutClear"));
    m_clear->setToolTip(texts::clearShortcut());
    m_clear->setText(texts::clearShortcut());
    m_clear->setIcon(settingsGlyphIcon(SettingsGlyph::Clear, palette().color(QPalette::WindowText)));
    m_clear->setVisible(false);
    if (m_display) {
        m_display->addAction(m_clear, QLineEdit::TrailingPosition);
    }
    connect(m_clear, &QAction::triggered, this, [this] {
        clear();
        refreshClearAction();
    });
    connect(this, &QKeySequenceEdit::keySequenceChanged, this, [this] {
        applyPlaceholder();
        refreshClearAction();
    });
    applyPlaceholder();
}

bool SettingsShortcutEdit::isRecording() const
{
    return m_recording;
}

bool SettingsShortcutEdit::isClearVisible() const
{
    return m_clear->isVisible();
}

QLineEdit *SettingsShortcutEdit::displayEdit() const
{
    return m_display;
}

QAction *SettingsShortcutEdit::clearAction() const
{
    return m_clear;
}

void SettingsShortcutEdit::focusInEvent(QFocusEvent *event)
{
    QKeySequenceEdit::focusInEvent(event);
    setRecording(true);
}

void SettingsShortcutEdit::focusOutEvent(QFocusEvent *event)
{
    QKeySequenceEdit::focusOutEvent(event);
    setRecording(false);
}

void SettingsShortcutEdit::enterEvent(QEnterEvent *event)
{
    QKeySequenceEdit::enterEvent(event);
    m_hovered = true;
    refreshClearAction();
}

void SettingsShortcutEdit::leaveEvent(QEvent *event)
{
    QKeySequenceEdit::leaveEvent(event);
    m_hovered = false;
    refreshClearAction();
}

void SettingsShortcutEdit::changeEvent(QEvent *event)
{
    QKeySequenceEdit::changeEvent(event);
    // 1. 【设置】【快捷键胶囊】主题切换后图标跟随文本色，禁用时隐藏清除入口
    if (event->type() == QEvent::PaletteChange) {
        m_clear->setIcon(settingsGlyphIcon(SettingsGlyph::Clear, palette().color(QPalette::WindowText)));
    } else if (event->type() == QEvent::EnabledChange) {
        refreshClearAction();
    }
}

bool SettingsShortcutEdit::eventFilter(QObject *watched, QEvent *event)
{
    // 1. 【设置】【快捷键胶囊】Qt 在清除或写入快捷键时会重置英文提示，绘制前统一改为本地化文案
    if (watched == m_display && event->type() == QEvent::Paint) {
        applyPlaceholder();
    }
    return QKeySequenceEdit::eventFilter(watched, event);
}

void SettingsShortcutEdit::setRecording(bool recording)
{
    if (m_recording == recording) {
        return;
    }
    // 1. 【设置】【快捷键胶囊】录入属性驱动青色边框，需重新应用样式
    m_recording = recording;
    setProperty("recording", recording);
    style()->unpolish(this);
    style()->polish(this);
    update();
    applyPlaceholder();
    refreshClearAction();
}

void SettingsShortcutEdit::refreshClearAction()
{
    const bool visible = isEnabled() && !keySequence().isEmpty() && (m_hovered || m_recording);
    if (m_clear->isVisible() != visible) {
        m_clear->setVisible(visible);
    }
}

void SettingsShortcutEdit::applyPlaceholder()
{
    if (!m_display) {
        return;
    }
    const QString text = m_recording ? texts::shortcutRecordingPlaceholder() : texts::shortcutIdlePlaceholder();
    // 1. 【设置】【空闲绘制】文案未变化时不写入，避免重复触发重绘
    if (m_display->placeholderText() != text) {
        m_display->setPlaceholderText(text);
    }
}

}  // namespace markshot::settings
