#include "ui/settings_card.h"

#include "ui/disclosure_section.h"
#include "ui/theme.h"

#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace markshot::ui {
namespace {

// 卡片内边距：常规与紧凑两档，水平值在两种模式间保持一致，保证跨卡片字段对齐
constexpr int kPaddingX = 16;
constexpr int kPaddingY = 12;
constexpr int kCompactPaddingX = 12;
constexpr int kCompactPaddingY = 10;
// 折叠入口按钮自带上下留白，卡片垂直内边距相应减小
constexpr int kDisclosureInsetY = 6;
constexpr int kBodySpacing = 10;

}  // namespace

SettingsCard::SettingsCard(const QString &title, const QString &description, Mode mode, QWidget *parent)
    : QFrame(parent)
    , m_mode(mode)
    , m_title(title)
    , m_description(description)
{
    setObjectName(QStringLiteral("settingsCard"));
    setProperty("settingsCardMode",
                mode == Mode::Collapsible ? QStringLiteral("collapsible") : QStringLiteral("static"));
    setAccessibleName(title);
    setAccessibleDescription(description);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(kBodySpacing);

    if (mode == Mode::Static) {
        // 1. 【设置】【卡片标题】常驻标题使用主题字体，说明作为悬停提示保持界面紧凑
        m_titleLabel = new QLabel(title, this);
        m_titleLabel->setObjectName(QStringLiteral("settingsCardTitle"));
        m_titleLabel->setFont(markshot::theme::uiFont(11, QFont::DemiBold));
        m_titleLabel->setTextFormat(Qt::PlainText);
        m_titleLabel->setWordWrap(true);
        m_titleLabel->setToolTip(description);
        m_titleLabel->setAccessibleDescription(description);
        layout->addWidget(m_titleLabel);
        m_body = new QVBoxLayout;
        m_body->setContentsMargins(0, 0, 0, 0);
        m_body->setSpacing(kBodySpacing);
        layout->addLayout(m_body);
    } else {
        // 2. 【设置】【折叠卡片】复用折叠分组，展开后先显示说明再显示字段
        m_disclosure = new DisclosureSection(title, this);
        m_disclosure->setToolTip(description);
        layout->addWidget(m_disclosure);
        m_body = new QVBoxLayout(m_disclosure->content());
        m_body->setContentsMargins(0, 4, 0, 4);
        m_body->setSpacing(kBodySpacing);
        if (!description.isEmpty()) {
            auto *help = new QLabel(description, m_disclosure->content());
            help->setObjectName(QStringLiteral("settingsCardDescription"));
            help->setTextFormat(Qt::PlainText);
            help->setWordWrap(true);
            m_body->addWidget(help);
        }
    }
    m_compact = true;
    applyDensity(false);
}

SettingsCard::Mode SettingsCard::mode() const
{
    return m_mode;
}

QString SettingsCard::title() const
{
    return m_title;
}

QString SettingsCard::description() const
{
    return m_description;
}

QLabel *SettingsCard::titleLabel() const
{
    return m_titleLabel;
}

DisclosureSection *SettingsCard::disclosure() const
{
    return m_disclosure;
}

QVBoxLayout *SettingsCard::bodyLayout() const
{
    return m_body;
}

bool SettingsCard::isCompact() const
{
    return m_compact;
}

void SettingsCard::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    applyDensity(event->size().width() < kCompactWidth);
}

void SettingsCard::applyDensity(bool compact)
{
    // 1. 【设置】【空闲布局】密度未变化时直接返回，避免尺寸事件中反复使布局失效
    if (compact == m_compact) {
        return;
    }
    m_compact = compact;
    const int x = compact ? kCompactPaddingX : kPaddingX;
    const int y = compact ? kCompactPaddingY : kPaddingY;
    const int vertical = m_mode == Mode::Collapsible ? qMax(0, y - kDisclosureInsetY) : y;
    layout()->setContentsMargins(x, vertical, x, vertical);
    setProperty("compact", compact);
}

}  // namespace markshot::ui
