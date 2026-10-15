#include "ui/context_menu_model.h"

#include "ui/context_menu_style.h"

#include <QAction>
#include <QActionGroup>
#include <QFontMetrics>
#include <QKeySequence>
#include <QVariant>

#include <algorithm>

namespace markshot::ui {

QString contextMenuDisplayText(const QString &text)
{
    // 1. 制表符之后是 QMenu 约定的快捷键列，正文只保留之前的部分
    const int tab = text.indexOf(QLatin1Char('\t'));
    const QString label = tab >= 0 ? text.left(tab) : text;

    // 2. 去除助记符标记，"&&" 表示字面量 "&"
    QString result;
    result.reserve(label.size());
    for (int i = 0; i < label.size(); ++i) {
        const QChar ch = label.at(i);
        if (ch == QLatin1Char('&')) {
            if (i + 1 < label.size() && label.at(i + 1) == QLatin1Char('&')) {
                result.append(ch);
                ++i;
            }
            continue;
        }
        result.append(ch);
    }
    return result;
}

QChar contextMenuMnemonic(const QString &text)
{
    const int tab = text.indexOf(QLatin1Char('\t'));
    const QString label = tab >= 0 ? text.left(tab) : text;
    for (int i = 0; i + 1 < label.size(); ++i) {
        if (label.at(i) != QLatin1Char('&')) {
            continue;
        }
        // "&&" 是字面量，跳过后继续查找
        if (label.at(i + 1) == QLatin1Char('&')) {
            ++i;
            continue;
        }
        return label.at(i + 1).toLower();
    }
    return {};
}

QString contextMenuShortcutText(const QAction *action)
{
    if (!action) {
        return {};
    }
    // 1. 兼容 QMenu 的 "文案\t快捷键" 写法
    const QString text = action->text();
    const int tab = text.indexOf(QLatin1Char('\t'));
    if (tab >= 0) {
        return text.mid(tab + 1).trimmed();
    }
    // 2. 其次读取 QAction 自身的快捷键与上下文菜单可见性
    if (action->shortcut().isEmpty() || !action->isShortcutVisibleInContextMenu()) {
        return {};
    }
    return action->shortcut().toString(QKeySequence::NativeText);
}

QList<QAction *> contextMenuVisibleActions(const QList<QAction *> &actions)
{
    QList<QAction *> visible;
    visible.reserve(actions.size());
    for (QAction *action : actions) {
        if (!action || !action->isVisible()) {
            continue;
        }
        // 1. 跳过开头与连续的分隔线
        if (action->isSeparator() && (visible.isEmpty() || visible.constLast()->isSeparator())) {
            continue;
        }
        visible.append(action);
    }
    // 2. 去掉结尾的分隔线
    while (!visible.isEmpty() && visible.constLast()->isSeparator()) {
        visible.removeLast();
    }
    return visible;
}

ContextMenuEntry contextMenuEntryFromAction(const QAction *action, bool hasSubmenu)
{
    ContextMenuEntry entry;
    if (!action) {
        entry.separator = true;
        return entry;
    }
    entry.separator = action->isSeparator();
    entry.enabled = action->isEnabled();
    entry.checkable = action->isCheckable();
    entry.checked = action->isChecked();
    entry.exclusive = action->actionGroup()
        && action->actionGroup()->exclusionPolicy() != QActionGroup::ExclusionPolicy::None;
    entry.danger = action->property(kContextMenuDangerProperty).toBool();
    entry.hasSubmenu = hasSubmenu;
    if (action->isIconVisibleInMenu()) {
        entry.icon = action->icon();
    }
    entry.text = contextMenuDisplayText(action->text());
    entry.shortcut = hasSubmenu ? QString() : contextMenuShortcutText(action);
    return entry;
}

bool contextMenuEntrySelectable(const ContextMenuEntry &entry)
{
    return !entry.separator && entry.enabled;
}

ContextMenuLayout layoutContextMenu(const QVector<ContextMenuEntry> &entries,
                                    const ContextMenuStyle &style,
                                    const QFontMetrics &metrics)
{
    // 1. 统计各列宽度，只要有一行需要图标或勾选标记就为全部行保留图标列
    bool iconColumn = false;
    bool arrowColumn = false;
    int textWidth = 0;
    int shortcutWidth = 0;
    for (const ContextMenuEntry &entry : entries) {
        if (entry.separator) {
            continue;
        }
        iconColumn = iconColumn || entry.checkable || !entry.icon.isNull();
        arrowColumn = arrowColumn || entry.hasSubmenu;
        textWidth = std::max(textWidth, metrics.horizontalAdvance(entry.text));
        if (!entry.shortcut.isEmpty()) {
            shortcutWidth = std::max(shortcutWidth, metrics.horizontalAdvance(entry.shortcut));
        }
    }
    const int iconPart = iconColumn ? style.iconSize + style.iconTextGap : 0;
    const int arrowPart = arrowColumn ? style.arrowWidth + style.iconTextGap / 2 : 0;
    const int shortcutPart = shortcutWidth > 0 ? style.shortcutGap + shortcutWidth : 0;
    const int contentWidth = style.itemHorizontalPadding * 2 + iconPart + textWidth + shortcutPart + arrowPart;
    const int panelWidth = std::clamp(contentWidth + style.panelPadding * 2,
                                      style.minimumWidth,
                                      std::max(style.minimumWidth, style.maximumWidth));

    // 2. 行高随字体放大，避免大字号时文字被裁切
    const int itemHeight = std::max(style.itemHeight, metrics.height() + 10);

    // 3. 自上而下排列菜单行，四周预留阴影边距
    ContextMenuLayout layout;
    const int panelLeft = style.shadowMargin;
    const int panelTop = style.shadowMargin;
    const int itemLeft = panelLeft + style.panelPadding;
    const int itemWidth = panelWidth - style.panelPadding * 2;
    int y = panelTop + style.panelPadding;
    layout.itemRects.reserve(entries.size());
    for (const ContextMenuEntry &entry : entries) {
        const int height = entry.separator ? style.separatorHeight : itemHeight;
        layout.itemRects.append(QRect(itemLeft, y, itemWidth, height));
        y += height;
    }
    const int panelHeight = y + style.panelPadding - panelTop;
    layout.panelRect = QRect(panelLeft, panelTop, panelWidth, panelHeight);
    layout.widgetSize = QSize(panelWidth + style.shadowMargin * 2, panelHeight + style.shadowMargin * 2);
    layout.textX = itemLeft + style.itemHorizontalPadding + iconPart;
    layout.shortcutRight = itemLeft + itemWidth - style.itemHorizontalPadding - arrowPart;
    return layout;
}

int contextMenuIndexAt(const ContextMenuLayout &layout, const QPoint &localPoint)
{
    for (int i = 0; i < layout.itemRects.size(); ++i) {
        if (layout.itemRects.at(i).contains(localPoint)) {
            return i;
        }
    }
    return -1;
}

}  // namespace markshot::ui
