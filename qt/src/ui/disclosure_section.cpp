#include "ui/disclosure_section.h"

#include "ui/theme.h"

#include <QBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QToolButton>

namespace markshot::ui {

DisclosureSection::DisclosureSection(const QString &title, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("disclosureSection"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    auto *header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(10);
    m_toggle = new QToolButton(this);
    m_toggle->setObjectName(QStringLiteral("disclosureToggle"));
    m_toggle->setProperty("disclosureHeader", true);
    m_toggle->setText(title);
    m_toggle->setAccessibleName(title);
    m_toggle->setFont(markshot::theme::uiFont(10, QFont::DemiBold));
    m_toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggle->setCheckable(true);
    m_toggle->setArrowType(Qt::RightArrow);
    header->addWidget(m_toggle);
    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("disclosureSummary"));
    m_summary->setProperty("role", QStringLiteral("muted"));
    m_summary->setFont(markshot::theme::uiFont(9));
    m_summary->setTextFormat(Qt::PlainText);
    m_summary->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_summary->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    header->addWidget(m_summary, 1);
    layout->addLayout(header);
    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("disclosureContent"));
    m_content->hide();
    layout->addWidget(m_content);
    connect(m_toggle, &QToolButton::toggled, this, &DisclosureSection::setExpanded);
}

QWidget *DisclosureSection::content() const
{
    return m_content;
}

bool DisclosureSection::isExpanded() const
{
    return !m_content->isHidden();
}

void DisclosureSection::setSummary(const QString &summary)
{
    m_summary->setText(summary);
    m_summary->setToolTip(summary);
    m_toggle->setAccessibleDescription(summary);
}

void DisclosureSection::setExpanded(bool expanded)
{
    const bool changed = expanded != isExpanded();
    const QSignalBlocker blocker(m_toggle);
    m_toggle->setChecked(expanded);
    m_toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    // 1. 【界面】【渐进呈现】收起控件时保留输入，并使键盘焦点回到可见入口
    if (!expanded && m_content->isAncestorOf(focusWidget())) {
        m_toggle->setFocus(Qt::OtherFocusReason);
    }
    m_content->setVisible(expanded);
    if (changed) {
        emit expandedChanged(expanded);
    }
}

}
