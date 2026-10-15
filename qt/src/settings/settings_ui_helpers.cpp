#include "settings/settings_ui_helpers.h"
#include "settings/settings_field_metrics.h"
#include "settings/settings_form_layout.h"
#include "settings/settings_shortcut_edit.h"
#include "settings/settings_text_context_menu.h"

#include "ui/settings_card.h"
#include "ui/settings_form_row.h"
#include "ui/theme.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QVBoxLayout>

namespace markshot::settings {
namespace {

/// @brief 绘制无单选圆点的下拉行：悬停浅底，当前项用强调色和勾号
class ComboPopupDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const bool selected = option.state.testFlag(QStyle::State_Selected);
        const bool hover = option.state.testFlag(QStyle::State_MouseOver);
        const QRect row = option.rect.adjusted(4, 1, -4, -1);
        if (selected || hover) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(selected ? option.palette.highlight() : option.palette.alternateBase());
            painter->drawRoundedRect(row, 6, 6);
        }
        const QString text = index.data(Qt::DisplayRole).toString();
        const QRect textRect = row.adjusted(10, 0, selected ? -28 : -10, 0);
        painter->setPen(selected ? option.palette.color(QPalette::HighlightedText)
                                 : option.palette.color(QPalette::Text));
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                          option.fontMetrics.elidedText(text, Qt::ElideRight, std::max(0, textRect.width())));
        if (selected) {
            painter->setPen(QPen(option.palette.color(QPalette::HighlightedText), 1.7, Qt::SolidLine,
                                 Qt::RoundCap, Qt::RoundJoin));
            const QPointF origin(row.right() - 16, row.center().y());
            painter->drawLine(origin + QPointF(-5, 0), origin + QPointF(-1.5, 3.5));
            painter->drawLine(origin + QPointF(-1.5, 3.5), origin + QPointF(4.5, -3.5));
        }
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const QSize base = QStyledItemDelegate::sizeHint(option, index);
        return {std::max(base.width(), 120), std::max(30, base.height())};
    }
};

}

void polishComboPopup(QComboBox *combo)
{
    if (!combo) {
        return;
    }
    // 1. 【界面】【下拉列表】自绘条目，避免平台样式在当前项左侧画单选圆点
    auto *view = combo->view();
    view->setItemDelegate(new ComboPopupDelegate(combo));
    view->setMouseTracking(true);
    view->setFrameShape(QFrame::NoFrame);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    combo->setMaxVisibleItems(8);
}

QVBoxLayout *createSettingsPageLayout(QWidget *parent)
{
    // 1. 【设置】【页面布局】水平边距与表单视区扣减值保持一致，卡片之间以紧凑间距分组
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);
    return layout;
}

QFrame *createSettingsCard(const QString &title, const QString &description, QWidget *parent)
{
    auto *card = new markshot::ui::SettingsCard(title, description, markshot::ui::SettingsCard::Mode::Static, parent);
    createSettingsFormLayout(card->bodyLayout());
    return card;
}

QFrame *createAdvancedSettingsCard(const QString &title, const QString &description, QWidget *parent)
{
    auto *card =
        new markshot::ui::SettingsCard(title, description, markshot::ui::SettingsCard::Mode::Collapsible, parent);
    createSettingsFormLayout(card->bodyLayout());
    return card;
}

QFormLayout *settingsCardForm(QFrame *card)
{
    return card ? card->findChild<QFormLayout *>(QStringLiteral("settingsCardForm")) : nullptr;
}

QCheckBox *addSwitchRow(QFormLayout *form, const QString &label, const QString &description)
{
    // 1. 【设置】【开关行】开关只保留指示器，标题与说明进入统一标签区，与其它字段同列对齐
    auto *box = new QCheckBox;
    box->setObjectName(QStringLiteral("settingsSwitch"));
    box->setAccessibleName(label);
    box->setAccessibleDescription(description);
    box->setMinimumHeight(28);
    box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    box->setCursor(Qt::PointingHandCursor);
    markshot::ui::addSettingsFormRow(form, label, description, box);
    return box;
}

QLineEdit *addTextRow(QFormLayout *form, const QString &label, const QString &placeholder)
{
    // 1. 【设置】【文本行】普通单行输入统一宽度上限，靠卡片右侧对齐
    auto *edit = new QLineEdit;
    edit->setAccessibleName(label);
    edit->setPlaceholderText(placeholder);
    edit->setMaximumWidth(metrics::kTextFieldMaxWidth);
    installLineEditContextMenu(edit);
    markshot::ui::addSettingsFormRow(form, label, {}, edit);
    return edit;
}

QLineEdit *addPathRow(QFormLayout *form, const QString &label, SettingsPathField::Kind kind,
                      const QString &placeholder)
{
    // 1. 【设置】【路径行】路径与命令字段按卡片宽度拉伸，右侧提供浏览按钮
    auto *field = new SettingsPathField(kind, label);
    field->lineEdit()->setPlaceholderText(placeholder);
    markshot::ui::addSettingsFormRow(form, label, {}, field);
    return field->lineEdit();
}

QPlainTextEdit *addPlainTextRow(QFormLayout *form, const QString &label, const QString &placeholder)
{
    auto *edit = new QPlainTextEdit;
    edit->setAccessibleName(label);
    edit->setPlaceholderText(placeholder);
    edit->setMinimumHeight(74);
    installPlainTextEditContextMenu(edit);
    markshot::ui::addSettingsFormRow(form, label, {}, edit);
    return edit;
}

QPlainTextEdit *addEnvironmentRow(QFormLayout *form, const QString &label, const QString &placeholder)
{
    // 1. 【设置】【环境变量】等宽字体逐行对齐 KEY=value，不自动换行
    auto *edit = new QPlainTextEdit;
    edit->setObjectName(QStringLiteral("settingsEnvironmentEdit"));
    edit->setAccessibleName(label);
    edit->setPlaceholderText(placeholder);
    edit->setFont(markshot::theme::monospaceFont(10));
    edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    edit->setTabChangesFocus(true);
    // 2. 【设置】【环境变量】默认高度约 4 行，更多内容在框内滚动
    const int lineHeight = edit->fontMetrics().lineSpacing();
    const int chrome = qRound(edit->document()->documentMargin() * 2) + 2 * edit->frameWidth() + 8;
    edit->setFixedHeight(lineHeight * metrics::kEnvironmentVisibleLines + chrome);
    installPlainTextEditContextMenu(edit);
    markshot::ui::addSettingsFormRow(form, label, {}, edit);
    return edit;
}

QSpinBox *addSpinRow(QFormLayout *form, const QString &label, int minimum, int maximum, const QString &suffix)
{
    auto *spin = new QSpinBox;
    spin->setAccessibleName(label);
    spin->setRange(minimum, maximum);
    spin->setSuffix(suffix);
    spin->setMaximumWidth(metrics::kNumberFieldMaxWidth);
    spin->setContextMenuPolicy(Qt::NoContextMenu);
    markshot::ui::addSettingsFormRow(form, label, {}, spin);
    return spin;
}

QDoubleSpinBox *addDoubleRow(QFormLayout *form, const QString &label, double minimum, double maximum, int decimals)
{
    auto *spin = new QDoubleSpinBox;
    spin->setAccessibleName(label);
    spin->setRange(minimum, maximum);
    spin->setDecimals(decimals);
    spin->setMaximumWidth(metrics::kNumberFieldMaxWidth);
    spin->setContextMenuPolicy(Qt::NoContextMenu);
    markshot::ui::addSettingsFormRow(form, label, {}, spin);
    return spin;
}

QComboBox *addComboRow(QFormLayout *form, const QString &label, const QString &description)
{
    // 1. 【设置】【下拉行】与单行输入共用宽度上限，靠卡片右侧对齐
    auto *combo = new QComboBox;
    combo->setAccessibleName(label);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    combo->setMinimumContentsLength(8);
    combo->setMaximumWidth(metrics::kTextFieldMaxWidth);
    combo->setCursor(Qt::PointingHandCursor);
    combo->setContextMenuPolicy(Qt::NoContextMenu);
    polishComboPopup(combo);
    markshot::ui::addSettingsFormRow(form, label, description, combo);
    return combo;
}

QKeySequenceEdit *addShortcutRow(QFormLayout *form, const QString &label)
{
    // 1. 【设置】【快捷键行】胶囊录入框提供输入边界、录入高亮与清除图标
    auto *edit = new SettingsShortcutEdit;
    edit->setAccessibleName(label);
    markshot::ui::addSettingsFormRow(form, label, {}, edit);
    return edit;
}

QWidget *addFieldRow(QFormLayout *form, const QString &label, QWidget *field, const QString &description)
{
    markshot::ui::addSettingsFormRow(form, label, description, field);
    return field;
}

}  // namespace markshot::settings
