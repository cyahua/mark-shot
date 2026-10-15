#include "ui/context_menu.h"

#include "ui/context_menu_placement.h"

#include <QKeyEvent>
#include <QMouseEvent>

#include <algorithm>

namespace markshot::ui {
namespace {

// 弹出后指针移动超过该距离才认为用户在菜单内进行了新的操作
constexpr int kReleaseDragThreshold = 4;
// 弹出后超过该时长的释放视为有效点击
constexpr int kReleaseDelayMs = 250;

}  // namespace

ContextMenu *ContextMenu::deepestMenu()
{
    ContextMenu *menu = rootMenu();
    while (menu->m_openSubmenu && menu->m_openSubmenu->isVisible()) {
        menu = menu->m_openSubmenu;
    }
    return menu;
}

ContextMenu *ContextMenu::menuAt(const QPoint &globalPoint)
{
    // 自最深的子菜单向根菜单查找，重叠区域优先命中上层菜单
    for (ContextMenu *menu = deepestMenu(); menu; menu = menu->m_parentMenu) {
        if (menu->isVisible() && menu->panelGlobalRect().contains(globalPoint)) {
            return menu;
        }
    }
    return nullptr;
}

void ContextMenu::mouseMoveEvent(QMouseEvent *event)
{
    // 弹出窗口抓取指针后，移动事件可能投递给任一层菜单，统一按全局坐标分派
    hoverAt(event->globalPosition().toPoint());
    event->accept();
}

void ContextMenu::hoverAt(const QPoint &globalPoint)
{
    ContextMenu *target = menuAt(globalPoint);
    if (!target) {
        // 指针离开整棵菜单：没有打开子菜单的最深层取消高亮
        ContextMenu *deepest = deepestMenu();
        if (!deepest->m_openSubmenu) {
            deepest->setActiveIndex(-1, true);
        }
        return;
    }
    // 1. 指针进入子菜单时，父菜单保持指向该子菜单的行并取消待执行的切换
    if (ContextMenu *parentMenu = target->m_parentMenu) {
        parentMenu->m_submenuTimer.stop();
        parentMenu->m_pendingSubmenuIndex = -1;
        const int ownerIndex = parentMenu->m_visibleActions.indexOf(target->menuAction());
        if (ownerIndex >= 0 && parentMenu->m_activeIndex != ownerIndex) {
            parentMenu->m_activeIndex = ownerIndex;
            parentMenu->update();
        }
    }
    // 2. 命中菜单内按行高亮，分隔线与禁用行不高亮
    const int index = contextMenuIndexAt(target->m_layout, target->mapFromGlobal(globalPoint));
    target->setActiveIndex(target->selectable(index) ? index : -1, true);
}

void ContextMenu::setActiveIndex(int index, bool fromHover)
{
    // 1. 悬停到其他行时延迟关闭已打开的子菜单，便于斜向移动进入子菜单
    if (fromHover) {
        const int openIndex = m_openSubmenu ? m_visibleActions.indexOf(m_openSubmenu->menuAction()) : -1;
        const bool wantsSubmenu = index >= 0 && menuForAction(m_visibleActions.at(index));
        if (index != openIndex && (m_openSubmenu || wantsSubmenu)) {
            if (m_pendingSubmenuIndex != index || !m_submenuTimer.isActive()) {
                m_pendingSubmenuIndex = index;
                m_submenuTimer.start();
            }
        } else if (index == openIndex) {
            m_submenuTimer.stop();
            m_pendingSubmenuIndex = -1;
        }
        // 子菜单仍打开时，离开到空白处保持父行高亮，直到计时器切换
        if (index < 0 && m_openSubmenu) {
            return;
        }
    }
    if (index == m_activeIndex) {
        return;
    }
    // 2. 更新高亮并发出 hovered，槽函数可能修改或删除动作
    m_activeIndex = index;
    update();
    QAction *action = activeAction();
    if (!action) {
        return;
    }
    const QPointer<ContextMenu> guard(this);
    const QPointer<QAction> guardAction(action);
    action->activate(QAction::Hover);
    if (guard && guardAction) {
        emit hovered(guardAction.data());
    }
}

void ContextMenu::onSubmenuTimer()
{
    const int index = m_pendingSubmenuIndex;
    m_pendingSubmenuIndex = -1;
    if (!isVisible()) {
        return;
    }
    // 计时到期后按最终停留的行切换子菜单
    const bool wantsSubmenu = selectable(index) && menuForAction(m_visibleActions.at(index));
    if (wantsSubmenu) {
        openSubmenu(index, false);
        return;
    }
    closeSubmenu();
    if (index != m_activeIndex) {
        setActiveIndex(index, false);
    }
}

bool ContextMenu::selectable(int index) const
{
    return index >= 0 && index < m_entries.size() && contextMenuEntrySelectable(m_entries.at(index));
}

int ContextMenu::nextSelectable(int from, int step) const
{
    const int count = m_entries.size();
    if (count == 0 || step == 0) {
        return -1;
    }
    // 起点无效时从首尾开始，循环查找下一个可用行
    int index = from;
    if (index < 0 || index >= count) {
        index = step > 0 ? -1 : count;
    }
    for (int i = 0; i < count; ++i) {
        index = ((index + step) % count + count) % count;
        if (selectable(index)) {
            return index;
        }
    }
    return -1;
}

void ContextMenu::activateIndex(int index, bool fromKeyboard)
{
    if (!selectable(index)) {
        return;
    }
    // 子菜单行只负责展开，普通行关闭菜单后交付动作
    if (menuForAction(m_visibleActions.at(index))) {
        openSubmenu(index, fromKeyboard);
        return;
    }
    triggerAction(m_visibleActions.at(index));
}

void ContextMenu::openSubmenu(int index, bool selectFirst)
{
    m_submenuTimer.stop();
    m_pendingSubmenuIndex = -1;
    ContextMenu *submenu = selectable(index) ? menuForAction(m_visibleActions.at(index)) : nullptr;
    if (!submenu) {
        return;
    }
    // 1. 同一子菜单已打开时只同步高亮
    if (m_openSubmenu == submenu && submenu->isVisible()) {
        if (selectFirst) {
            submenu->setActiveIndex(submenu->nextSelectable(-1, 1), false);
        }
        return;
    }
    closeSubmenu();
    if (m_activeIndex != index) {
        setActiveIndex(index, false);
    }

    // 2. 先建立父子链接，子菜单才能继承主题并在关闭时回到父菜单
    const QPointer<ContextMenu> guard(this);
    const QPointer<ContextMenu> child(submenu);
    child->m_parentMenu = this;
    m_openSubmenu = child;
    if (!child->preparePopup()) {
        if (child) {
            child->m_parentMenu = nullptr;
        }
        if (guard) {
            m_openSubmenu = nullptr;
        }
        return;
    }
    if (!guard || !child || !isVisible()) {
        return;
    }

    // 3. 子菜单首行与父菜单行对齐，右侧越界时翻转到左侧
    const int inset = child->m_layout.itemRects.isEmpty()
        ? 0 : child->m_layout.itemRects.first().top() - child->m_layout.panelRect.top();
    const QRect panel = placeContextSubmenu(panelGlobalRect(), itemGlobalRect(index),
                                            child->m_layout.panelRect.size(),
                                            child->screenBounds(itemGlobalRect(index).center()),
                                            m_style.submenuOverlap, inset);
    child->showPanel(panel);
    if (child && selectFirst) {
        child->setActiveIndex(child->nextSelectable(-1, 1), false);
    }
}

void ContextMenu::closeSubmenu()
{
    const QPointer<ContextMenu> child = m_openSubmenu;
    m_openSubmenu = nullptr;
    if (!child) {
        return;
    }
    // 未显示的子菜单不会收到 hideEvent，需要手动解除链接
    if (child->isVisible()) {
        child->hide();
    }
    if (child && child->m_parentMenu == this) {
        child->m_parentMenu = nullptr;
    }
}

void ContextMenu::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    ContextMenu *root = rootMenu();
    ContextMenu *target = menuAt(event->globalPosition().toPoint());
    // 1. 点击整棵菜单之外时关闭会话
    if (!target) {
        root->hide();
        return;
    }
    // 2. 只有左键参与选择；按下子菜单行立即展开
    root->m_pressInside = event->button() == Qt::LeftButton;
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const int index = contextMenuIndexAt(target->m_layout, target->mapFromGlobal(event->globalPosition().toPoint()));
    if (target->selectable(index) && target->menuForAction(target->m_visibleActions.at(index))) {
        target->openSubmenu(index, false);
    }
}

void ContextMenu::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    if (event->button() != Qt::LeftButton) {
        return;
    }
    ContextMenu *root = rootMenu();
    ContextMenu *target = menuAt(event->globalPosition().toPoint());
    if (!target) {
        return;
    }
    // 1. 过滤“按下即弹出”时紧随其后的释放，避免误触发弹出位置下的菜单行
    const bool moved = (QCursor::pos() - root->m_popupCursor).manhattanLength() > kReleaseDragThreshold;
    const bool late = root->m_shownTimer.isValid() && root->m_shownTimer.elapsed() > kReleaseDelayMs;
    if (!root->m_pressInside && !moved && !late) {
        return;
    }
    root->m_pressInside = false;

    // 2. 子菜单行在按下时已展开，释放只交付普通动作
    const int index = contextMenuIndexAt(target->m_layout, target->mapFromGlobal(event->globalPosition().toPoint()));
    if (target->selectable(index) && !target->menuForAction(target->m_visibleActions.at(index))) {
        target->activateIndex(index, false);
    }
}

void ContextMenu::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    // 键盘焦点始终交给最深层可见菜单，保证子菜单打开后方向键作用于子菜单
    deepestMenu()->handleKey(event);
}

void ContextMenu::handleKey(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Backtab:
        setActiveIndex(nextSelectable(m_activeIndex, -1), false);
        return;
    case Qt::Key_Down:
    case Qt::Key_Tab:
        setActiveIndex(nextSelectable(m_activeIndex, 1), false);
        return;
    case Qt::Key_Home:
    case Qt::Key_PageUp:
        setActiveIndex(nextSelectable(-1, 1), false);
        return;
    case Qt::Key_End:
    case Qt::Key_PageDown:
        setActiveIndex(nextSelectable(-1, -1), false);
        return;
    case Qt::Key_Right:
        if (selectable(m_activeIndex) && menuForAction(m_visibleActions.at(m_activeIndex))) {
            openSubmenu(m_activeIndex, true);
        }
        return;
    case Qt::Key_Left:
        // 根菜单按左键不关闭，避免误操作丢失菜单
        if (m_parentMenu) {
            m_parentMenu->closeSubmenu();
        }
        return;
    case Qt::Key_Escape:
        if (m_parentMenu) {
            m_parentMenu->closeSubmenu();
        } else {
            hide();
        }
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
    case Qt::Key_Select:
        activateIndex(m_activeIndex, true);
        return;
    default:
        break;
    }
    // 修饰键组合不参与助记符匹配
    if (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier)) {
        return;
    }
    handleMnemonic(event->text());
}

bool ContextMenu::handleMnemonic(const QString &text)
{
    if (text.isEmpty() || !text.at(0).isPrint()) {
        return false;
    }
    const QChar key = text.at(0).toLower();

    // 1. 先匹配显式助记符，没有时退回到首字母匹配
    QVector<int> matches;
    for (int i = 0; i < m_visibleActions.size(); ++i) {
        if (selectable(i) && contextMenuMnemonic(m_visibleActions.at(i)->text()) == key) {
            matches.append(i);
        }
    }
    const bool explicitMnemonic = !matches.isEmpty();
    if (!explicitMnemonic) {
        for (int i = 0; i < m_entries.size(); ++i) {
            if (selectable(i) && !m_entries.at(i).text.isEmpty() && m_entries.at(i).text.at(0).toLower() == key) {
                matches.append(i);
            }
        }
    }
    if (matches.isEmpty()) {
        return false;
    }
    // 2. 唯一的显式助记符直接激活，其余情况在匹配项之间循环高亮
    if (explicitMnemonic && matches.size() == 1) {
        activateIndex(matches.first(), true);
        return true;
    }
    const auto next = std::upper_bound(matches.cbegin(), matches.cend(), m_activeIndex);
    setActiveIndex(next == matches.cend() ? matches.first() : *next, false);
    return true;
}

}  // namespace markshot::ui
