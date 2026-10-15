#include "ui/settings_form_row.h"

#include <QAbstractButton>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace markshot::ui {
namespace {

/// @brief 创建只显示纯文本、可换行的标签，避免富文本控件拦截标签区点击
/// @param text 标签文本
/// @param objectName 样式作用域对象名
/// @param parent 所属控件
/// @return 新建的标签
QLabel *createRowText(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(objectName);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return label;
}

}  // namespace

SettingsRowLabel::SettingsRowLabel(const QString &title, const QString &description, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("settingsRowLabel"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // 1. 【设置】【字段标签】标题始终显示，说明只在提供文本时占用一行
    m_title = createRowText(title, QStringLiteral("settingsRowTitle"), this);
    layout->addWidget(m_title);
    if (!description.isEmpty()) {
        m_description = createRowText(description, QStringLiteral("settingsRowDescription"), this);
        m_description->setProperty("role", QStringLiteral("muted"));
        layout->addWidget(m_description);
    }
    setAccessibleName(title);
    setAccessibleDescription(description);
}

QString SettingsRowLabel::title() const
{
    return m_title->text();
}

QString SettingsRowLabel::description() const
{
    return m_description ? m_description->text() : QString();
}

QLabel *SettingsRowLabel::titleLabel() const
{
    return m_title;
}

QLabel *SettingsRowLabel::descriptionLabel() const
{
    return m_description;
}

void SettingsRowLabel::setBuddy(QWidget *buddy)
{
    if (m_buddy == buddy) {
        return;
    }
    if (m_buddy) {
        m_buddy->removeEventFilter(this);
    }
    m_buddy = buddy;
    if (!m_buddy) {
        return;
    }
    // 1. 【设置】【状态同步】控件禁用时标签同步置灰，避免可读状态与可操作状态不一致
    m_buddy->installEventFilter(this);
    setEnabled(m_buddy->isEnabled());
    // 2. 【设置】【指针反馈】开关类控件允许点击整段标签切换
    if (auto *button = qobject_cast<QAbstractButton *>(m_buddy.data()); button && button->isCheckable()) {
        setCursor(Qt::PointingHandCursor);
    }
}

QWidget *SettingsRowLabel::buddy() const
{
    return m_buddy;
}

void SettingsRowLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_buddy) {
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void SettingsRowLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_buddy || !rect().contains(event->position().toPoint())) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    event->accept();
    if (!m_buddy->isEnabled()) {
        return;
    }
    // 1. 【设置】【标签点击】开关直接切换，其它控件只获取焦点，保持原有编辑流程
    if (auto *button = qobject_cast<QAbstractButton *>(m_buddy.data()); button && button->isCheckable()) {
        button->click();
    }
    m_buddy->setFocus(Qt::MouseFocusReason);
}

bool SettingsRowLabel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_buddy && event->type() == QEvent::EnabledChange) {
        setEnabled(m_buddy->isEnabled());
    }
    return QWidget::eventFilter(watched, event);
}

SettingsRowLabel *addSettingsFormRow(QFormLayout *form, const QString &title,
                                     const QString &description, QWidget *field)
{
    if (!form || !field) {
        return nullptr;
    }
    // 1. 【设置】【可访问性】未单独命名的控件沿用字段标题与说明
    if (field->accessibleName().isEmpty()) {
        field->setAccessibleName(title);
    }
    if (field->accessibleDescription().isEmpty() && !description.isEmpty()) {
        field->setAccessibleDescription(description);
    }
    // 2. 【设置】【表单追加】标签区与控件成对加入表单，便于按字段查找标签与切换可见性
    auto *label = new SettingsRowLabel(title, description);
    label->setBuddy(field);
    form->addRow(label, field);
    return label;
}

}  // namespace markshot::ui
