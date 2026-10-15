#include "settings/settings_form_layout.h"

#include <QAbstractScrollArea>
#include <QCheckBox>
#include <QFontMetrics>
#include <QFormLayout>
#include <QLabel>
#include <QScopedValueRollback>
#include <QTextLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace markshot::settings {
namespace {

/// @brief 按可用宽度换行开关文字，同时保留完整的可访问名称
/// @param text 原始开关说明
/// @param font 当前表单字体
/// @param width 扣除指示器后的文本可用宽度
/// @return 适合当前行宽的完整文本
QString wrappedSwitchText(const QString &text, const QFont &font, int width)
{
    QTextLayout layout(text, font);
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(option);
    QStringList lines;
    layout.beginLayout();
    for (QTextLine line = layout.createLine(); line.isValid(); line = layout.createLine()) {
        line.setLineWidth(std::max(40, width));
        lines.append(text.mid(line.textStart(), line.textLength()).trimmed());
    }
    layout.endLayout();
    return lines.join(QLatin1Char('\n'));
}

/// @brief 让复合标签的标题行与右侧单行控件等高，使标题与控件垂直居中、说明位于下方
/// @param labelWidget 标题加说明的复合标签
/// @param field 同一行的字段控件
/// @param compact 是否处于上下排列模式
/// @return 无返回值
void alignRowTitle(QWidget *labelWidget, QWidget *field, bool compact)
{
    auto *title = labelWidget->findChild<QLabel *>(QStringLiteral("settingsRowTitle"));
    if (!title) {
        return;
    }
    // 1. 【设置】【垂直对齐】多行输入等高控件保持顶部对齐，只为单行控件补齐标题行高度
    int target = 0;
    if (!compact && field) {
        const int fieldHeight = field->sizeHint().height();
        const int lineHeight = title->fontMetrics().height();
        target = fieldHeight <= lineHeight * 5 / 2 ? fieldHeight : 0;
    }
    // 2. 【设置】【空闲布局】高度未变化时不写入，避免几何计算中重复使布局失效
    if (title->minimumHeight() != target) {
        title->setMinimumHeight(target);
    }
}

/// @brief 根据实际视区宽度排列设置字段，避免旧最小宽度阻止窄屏换行
class SettingsFormLayout final : public QFormLayout {
public:
    /// @brief 应用标签列宽并计算表单几何
    /// @param rect 父布局分配的表单范围
    /// @return 无返回值
    void setGeometry(const QRect &rect) override
    {
        if (m_arranging || !parentWidget()) {
            QFormLayout::setGeometry(rect);
            return;
        }
        const QScopedValueRollback<bool> arranging(m_arranging, true);

        // 1. 【设置】【表单排布】优先使用滚动视区的真实宽度，窄窗口统一上下排列
        int availableWidth = rect.width();
        for (QWidget *ancestor = parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
            if (auto *scroll = qobject_cast<QAbstractScrollArea *>(ancestor)) {
                availableWidth = std::min(availableWidth, scroll->viewport()->width() - 40);
                break;
            }
        }
        const bool compact = availableWidth < 440;
        const auto wrapPolicy = compact ? QFormLayout::WrapAllRows : QFormLayout::DontWrapRows;
        // 2. 【设置】【空闲布局】Qt 即使收到相同策略也会使布局失效，避免在几何计算中循环排队
        if (rowWrapPolicy() != wrapPolicy) {
            setRowWrapPolicy(wrapPolicy);
        }
        const int labelWidth = settingsLabelColumnWidth(parentWidget()->fontMetrics().height(), availableWidth);

        // 3. 【设置】【标签对齐】各分组采用相同标签列，长标签换行且保留控件关联
        for (int row = 0; row < rowCount(); ++row) {
            QLayoutItem *field = itemAt(row, QFormLayout::SpanningRole);
            auto *box = field ? qobject_cast<QCheckBox *>(field->widget()) : nullptr;
            if (box && box->property("settingsSwitchLabel").isValid()) {
                box->setText(wrappedSwitchText(box->property("settingsSwitchLabel").toString(),
                                              box->font(), availableWidth - 32));
            }
            QLayoutItem *item = itemAt(row, QFormLayout::LabelRole);
            QWidget *labelWidget = item ? item->widget() : nullptr;
            if (!labelWidget) {
                continue;
            }
            // 4. 【设置】【标签区】普通标签开启换行，标题加说明的复合标签按标题行与控件居中
            if (auto *label = qobject_cast<QLabel *>(labelWidget); label && !label->wordWrap()) {
                label->setWordWrap(true);
            } else if (labelWidget->objectName() == QLatin1String("settingsRowLabel")) {
                QLayoutItem *fieldItem = itemAt(row, QFormLayout::FieldRole);
                alignRowTitle(labelWidget, fieldItem ? fieldItem->widget() : nullptr, compact);
            }
            labelWidget->setMinimumWidth(compact ? 0 : labelWidth);
            labelWidget->setMaximumWidth(compact ? QWIDGETSIZE_MAX : labelWidth);
        }
        QFormLayout::setGeometry(rect);
        // 5. 【设置】【右侧对齐】左右排列时受宽度上限约束的字段贴齐卡片右缘
        if (!compact) {
            alignFieldsRight(rect);
        }
    }

private:
    /// @brief 将未占满字段列的控件平移到字段列右缘，宽度保持 QFormLayout 计算结果
    /// @param rect 表单范围
    /// @return 无返回值
    void alignFieldsRight(const QRect &rect)
    {
        const bool rightToLeft = parentWidget()->layoutDirection() == Qt::RightToLeft;
        const QRect content = rect.marginsRemoved(contentsMargins());
        for (int row = 0; row < rowCount(); ++row) {
            QLayoutItem *item = itemAt(row, QFormLayout::FieldRole);
            if (!item || item->isEmpty()) {
                continue;
            }
            // 1. 【设置】【右侧对齐】按书写方向计算剩余空隙，空隙为零时不写入几何，避免空闲重排
            const QRect current = item->geometry();
            const int gap = rightToLeft ? current.left() - content.left() : content.right() - current.right();
            if (gap <= 0) {
                continue;
            }
            item->setGeometry(current.translated(rightToLeft ? -gap : gap, 0));
        }
    }

    bool m_arranging = false;
};

}

int settingsLabelColumnWidth(int lineHeight, int availableWidth)
{
    // 1. 【设置】【标签列宽】宽窗口给说明文字更多空间，上下限保证跨分组对齐与字段可用宽度
    const int minimum = lineHeight * 7;
    const int maximum = std::max(minimum, std::min(lineHeight * 13, availableWidth / 2));
    return std::clamp(availableWidth * 2 / 5, minimum, maximum);
}

QFormLayout *createSettingsFormLayout(QVBoxLayout *parentLayout)
{
    auto *form = new SettingsFormLayout;
    form->setObjectName(QStringLiteral("settingsCardForm"));
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(10);
    parentLayout->addLayout(form);
    return form;
}

}
