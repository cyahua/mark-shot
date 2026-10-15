#include "ui/context_menu.h"

#include "ui/context_menu_painter.h"
#include "ui/context_menu_placement.h"

#include <QActionEvent>
#include <QApplication>
#include <QCursor>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QStyleHints>
#include <QVariant>

namespace markshot::ui {
namespace {

// 弹出后短时间内的激活状态切换视为弹出过程本身引起，不触发外部关闭
constexpr int kDeactivateGraceMs = 300;
// 悬停切换子菜单的延迟
constexpr int kSubmenuDelayMs = 180;
// 控件下方弹出时面板与控件的间距
constexpr int kAnchorGap = 4;

/**
 * 判断当前是否运行在 Wayland 平台插件上。
 * @return Wayland 平台返回 true。
 */
bool isWaylandPlatform()
{
    return QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
}

}  // namespace

ContextMenu::ContextMenu(QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    initialize();
}

ContextMenu::ContextMenu(const QString &title, QWidget *parent)
    : ContextMenu(parent)
{
    setTitle(title);
}

ContextMenu::~ContextMenu()
{
    // 1. 结束阻塞会话，exec 中的守卫指针随后会发现菜单已销毁
    if (m_loop) {
        m_loop->quit();
    }
    QObject::disconnect(m_appStateConnection);
    // 2. 外部持有的子菜单不会随本对象销毁，需要显式关闭
    closeSubmenu();
    // 3. 从父菜单解除链接，避免父菜单持有悬空的打开状态
    if (m_parentMenu && m_parentMenu->m_openSubmenu == this) {
        m_parentMenu->m_openSubmenu = nullptr;
    }
}

void ContextMenu::initialize()
{
    // 1. 透明背景用于绘制圆角与阴影，弹出窗口类型使 Qt 负责指针与键盘抓取
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_X11NetWmWindowTypePopupMenu);
    setMouseTracking(true);
    setFont(QApplication::font("QMenu"));

    // 2. 菜单动作用于把本菜单挂入父菜单
    m_menuAction = new QAction(this);

    // 3. 悬停打开子菜单的延迟计时器
    m_submenuTimer.setSingleShot(true);
    m_submenuTimer.setInterval(kSubmenuDelayMs);
    connect(&m_submenuTimer, &QTimer::timeout, this, &ContextMenu::onSubmenuTimer);
}

QAction *ContextMenu::addSeparator()
{
    auto *action = new QAction(this);
    action->setSeparator(true);
    QWidget::addAction(action);
    return action;
}

ContextMenu *ContextMenu::addMenu(const QString &title)
{
    auto *menu = new ContextMenu(title, this);
    addMenu(menu);
    return menu;
}

ContextMenu *ContextMenu::addMenu(const QIcon &icon, const QString &title)
{
    ContextMenu *menu = addMenu(title);
    menu->menuAction()->setIcon(icon);
    return menu;
}

QAction *ContextMenu::addMenu(ContextMenu *menu)
{
    if (!menu || menu == this) {
        return nullptr;
    }
    // 先登记再添加，保证 ActionAdded 重建布局时已识别为子菜单行
    m_submenus.insert(menu->menuAction(), menu);
    QWidget::addAction(menu->menuAction());
    return menu->menuAction();
}

ContextMenu *ContextMenu::menuForAction(const QAction *action) const
{
    return action ? m_submenus.value(action).data() : nullptr;
}

QAction *ContextMenu::menuAction() const
{
    return m_menuAction;
}

void ContextMenu::setTitle(const QString &title)
{
    m_menuAction->setText(title);
    setWindowTitle(contextMenuDisplayText(title));
    setAccessibleName(contextMenuDisplayText(title));
}

QString ContextMenu::title() const
{
    return m_menuAction->text();
}

void ContextMenu::clear()
{
    closeSubmenu();
    const QList<QAction *> current = actions();
    for (QAction *action : current) {
        const QPointer<ContextMenu> submenu = m_submenus.value(action);
        removeAction(action);
        // 自有子菜单连同其菜单动作一起删除；外部子菜单与外部动作只解除挂载
        if (submenu && submenu->parent() == this) {
            submenu->deleteLater();
        } else if (!submenu && action->parent() == this) {
            action->deleteLater();
        }
    }
}

void ContextMenu::setActionDanger(QAction *action, bool danger)
{
    if (!action) {
        return;
    }
    action->setProperty(kContextMenuDangerProperty, danger);
    m_dirty = true;
    if (isVisible()) {
        rebuild();
        update();
    }
}

bool ContextMenu::actionDanger(const QAction *action)
{
    return action && action->property(kContextMenuDangerProperty).toBool();
}

void ContextMenu::setColorScheme(ColorScheme scheme)
{
    m_colorScheme = scheme;
    m_dirty = true;
    if (isVisible()) {
        rebuild();
        update();
    }
}

ContextMenu::ColorScheme ContextMenu::colorScheme() const
{
    return m_colorScheme;
}

void ContextMenu::setComponentTokens(const ContextMenuStyle &style)
{
    m_customStyle = style;
    m_customTokens = true;
    m_dirty = true;
    if (isVisible()) {
        rebuild();
        resize(m_layout.widgetSize);
        update();
    }
}

void ContextMenu::resetComponentTokens()
{
    m_customTokens = false;
    m_dirty = true;
    if (isVisible()) {
        rebuild();
        resize(m_layout.widgetSize);
        update();
    }
}

QSize ContextMenu::sizeHint() const
{
    ensureLayout();
    return m_layout.widgetSize;
}

void ContextMenu::popup(const QPoint &position, QAction *atAction)
{
    // 1. 动作交付期间的重新弹出请求延后到交付结束，避免打断当前会话
    if (rootMenu()->m_finishing) {
        const QPointer<QAction> at(atAction);
        QTimer::singleShot(0, this, [this, position, at] { popup(position, at.data()); });
        return;
    }
    // 2. 首次弹出发出 aboutToShow 并重建；已显示时只重新定位
    if (isVisible()) {
        ensureLayout();
    } else if (!preparePopup()) {
        return;
    }

    // 3. 面板默认位于锚点右下方；指定初始动作时让该行与锚点对齐
    const QRect bounds = screenBounds(position);
    QRect panel = placeContextMenuAtPoint(position, m_layout.panelRect.size(), bounds);
    const int atIndex = atAction ? m_visibleActions.indexOf(atAction) : -1;
    if (atIndex >= 0) {
        panel.translate(0, m_layout.panelRect.top() - m_layout.itemRects.at(atIndex).top());
        panel = clampContextMenuRect(panel, bounds);
    }
    showPanel(panel);
    if (atIndex >= 0) {
        setActiveIndex(atIndex, false);
    }
}

void ContextMenu::popupBelow(QWidget *anchor)
{
    const QPointer<QWidget> anchorGuard(anchor);
    if (!anchor || rootMenu()->m_finishing) {
        return;
    }
    if (isVisible()) {
        ensureLayout();
    } else if (!preparePopup()) {
        return;
    }
    if (!anchorGuard) {
        return;
    }
    const QRect anchorRect(anchor->mapToGlobal(QPoint(0, 0)), anchor->size());
    const QRect bounds = screenBounds(anchorRect.center());
    showPanel(placeContextMenuBelow(anchorRect, m_layout.panelRect.size(), bounds, kAnchorGap));
}

QAction *ContextMenu::exec(const QPoint &position, QAction *atAction)
{
    // 1. 防重入：执行中、交付中或已显示时拒绝新的阻塞会话
    if (m_executing || rootMenu()->m_finishing || isVisible()) {
        return nullptr;
    }
    const QPointer<ContextMenu> guard(this);
    m_executing = true;
    m_result = nullptr;
    popup(position, atAction);
    if (!guard) {
        return nullptr;
    }
    if (!isVisible()) {
        m_executing = false;
        return nullptr;
    }

    // 2. 局部事件循环运行到根菜单隐藏；菜单在循环中被销毁时由析构函数退出循环
    QEventLoop loop;
    m_loop = &loop;
    loop.exec(QEventLoop::DialogExec);
    if (!guard) {
        return nullptr;
    }
    m_loop = nullptr;
    m_executing = false;
    QAction *result = m_result.data();
    m_result = nullptr;
    return result;
}

void ContextMenu::dismiss()
{
    ContextMenu *root = rootMenu();
    if (root->isVisible()) {
        root->hide();
    }
}

void ContextMenu::setActiveAction(QAction *action)
{
    ensureLayout();
    const int index = action ? m_visibleActions.indexOf(action) : -1;
    setActiveIndex(selectable(index) ? index : -1, false);
}

QAction *ContextMenu::activeAction() const
{
    return m_activeIndex >= 0 && m_activeIndex < m_visibleActions.size()
        ? m_visibleActions.at(m_activeIndex)
        : nullptr;
}

QRect ContextMenu::actionGeometry(QAction *action) const
{
    ensureLayout();
    const int index = action ? m_visibleActions.indexOf(action) : -1;
    return index >= 0 ? m_layout.itemRects.at(index) : QRect();
}

bool ContextMenu::event(QEvent *event)
{
    // 菜单打开期间按键全部由菜单处理，避免触发宿主窗口的快捷键
    if (event->type() == QEvent::ShortcutOverride) {
        event->accept();
        return true;
    }
    return QWidget::event(event);
}

void ContextMenu::actionEvent(QActionEvent *event)
{
    QWidget::actionEvent(event);
    // 1. 移除子菜单行时解除登记，并关闭仍在显示的子菜单
    if (event->type() == QEvent::ActionRemoved) {
        const QPointer<ContextMenu> submenu = m_submenus.take(event->action());
        if (submenu && submenu == m_openSubmenu) {
            closeSubmenu();
        }
    }
    // 2. 显示期间动作变化立即重排，否则延迟到下次弹出
    m_dirty = true;
    if (isVisible()) {
        rebuild();
        resize(m_layout.widgetSize);
        update();
    }
}

void ContextMenu::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::StyleChange) {
        m_dirty = true;
        if (isVisible()) {
            rebuild();
            resize(m_layout.widgetSize);
            update();
        }
    }
}

void ContextMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    ensureLayout();
    QPainter painter(this);
    painter.setFont(font());
    paintContextMenu(painter, m_layout, m_entries, m_style, m_activeIndex);
}

void ContextMenu::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 根菜单在应用失去激活时关闭；Wayland 由合成器负责弹出窗口的外部关闭
    QObject::disconnect(m_appStateConnection);
    if (m_parentMenu || isWaylandPlatform()) {
        return;
    }
    m_appStateConnection = connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
                                   [this](Qt::ApplicationState state) {
                                       if (state != Qt::ApplicationActive && m_shownTimer.isValid()
                                           && m_shownTimer.elapsed() > kDeactivateGraceMs) {
                                           dismiss();
                                       }
                                   });
}

void ContextMenu::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_submenuTimer.stop();
    m_pendingSubmenuIndex = -1;
    QObject::disconnect(m_appStateConnection);

    // 1. 自深向浅关闭子菜单，再通知本菜单即将隐藏
    closeSubmenu();
    const QPointer<ContextMenu> guard(this);
    emit aboutToHide();
    if (!guard) {
        return;
    }
    m_activeIndex = -1;
    m_pressInside = false;
    m_shownTimer.invalidate();

    // 2. 子菜单只从父菜单解除链接，父菜单保持打开
    if (m_parentMenu) {
        if (m_parentMenu->m_openSubmenu == this) {
            m_parentMenu->m_openSubmenu = nullptr;
        }
        m_parentMenu = nullptr;
        return;
    }

    // 3. 根菜单结束会话；选中动作的场景由 triggerAction 在交付后发出 popupFinished
    if (m_loop) {
        m_loop->quit();
    }
    if (!m_finishing) {
        emit popupFinished();
    }
}

void ContextMenu::wheelEvent(QWheelEvent *event)
{
    // 菜单不滚动，吞掉滚轮避免穿透到弹出窗口抓取前的控件
    event->accept();
}

void ContextMenu::ensureLayout() const
{
    if (m_dirty) {
        const_cast<ContextMenu *>(this)->rebuild();
    }
}

void ContextMenu::rebuild()
{
    // 1. 以动作指针保持高亮，避免增删动作后高亮跳到其他行
    QAction *active = activeAction();
    m_dirty = false;
    m_style = effectiveStyle();
    m_visibleActions = contextMenuVisibleActions(actions());

    // 2. 从 QAction 派生展示快照并计算布局
    m_entries.clear();
    m_entries.reserve(m_visibleActions.size());
    for (const QAction *action : std::as_const(m_visibleActions)) {
        m_entries.append(contextMenuEntryFromAction(action, menuForAction(action) != nullptr));
    }
    m_layout = layoutContextMenu(m_entries, m_style, QFontMetrics(font()));
    const int index = active ? m_visibleActions.indexOf(active) : -1;
    m_activeIndex = selectable(index) ? index : -1;
}

ContextMenuStyle ContextMenu::effectiveStyle() const
{
    return m_customTokens ? m_customStyle : contextMenuStyle(resolveDark());
}

bool ContextMenu::resolveDark() const
{
    // 1. 显式设置优先
    if (m_colorScheme == ColorScheme::Dark) {
        return true;
    }
    if (m_colorScheme == ColorScheme::Light) {
        return false;
    }
    // 2. 子菜单继承父菜单，根菜单继承所属控件调色板
    if (m_parentMenu) {
        return m_parentMenu->resolveDark();
    }
    if (const QWidget *owner = parentWidget()) {
        if (const auto *menu = qobject_cast<const ContextMenu *>(owner)) {
            return menu->resolveDark();
        }
        return owner->palette().color(QPalette::Window).lightness() < 128;
    }
    // 3. 无所属控件时使用系统色彩方案
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme != Qt::ColorScheme::Unknown) {
        return scheme == Qt::ColorScheme::Dark;
    }
#endif
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

bool ContextMenu::preparePopup()
{
    if (m_preparing) {
        return false;
    }
    // 1. aboutToShow 中的槽函数可能更新动作甚至销毁菜单
    const QPointer<ContextMenu> guard(this);
    m_preparing = true;
    emit aboutToShow();
    if (!guard) {
        return false;
    }
    m_preparing = false;
    // 2. 空菜单不显示
    rebuild();
    return !m_entries.isEmpty();
}

void ContextMenu::showPanel(const QRect &panelGlobal)
{
    // 1. 面板外扩阴影边距得到窗口几何
    resize(m_layout.widgetSize);
    move(panelGlobal.topLeft() - m_layout.panelRect.topLeft());
    if (isVisible()) {
        update();
        return;
    }
    // 2. 记录弹出时的指针位置与时间，用于过滤触发弹出的那次按键释放
    m_popupCursor = QCursor::pos();
    m_pressInside = false;
    m_activeIndex = -1;
    m_shownTimer.start();
    show();
    raise();
    // 3. 托盘等应用未激活的场景请求激活，使外部点击与失焦都能关闭菜单
    if (!m_parentMenu && !isWaylandPlatform()
        && QGuiApplication::applicationState() != Qt::ApplicationActive) {
        activateWindow();
    }
}

QRect ContextMenu::screenBounds(const QPoint &anchor) const
{
    QScreen *screen = QGuiApplication::screenAt(anchor);
    if (!screen && parentWidget()) {
        screen = parentWidget()->screen();
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    return screen ? screen->availableGeometry() : QRect();
}

QRect ContextMenu::panelGlobalRect() const
{
    return QRect(mapToGlobal(m_layout.panelRect.topLeft()), m_layout.panelRect.size());
}

QRect ContextMenu::itemGlobalRect(int index) const
{
    if (index < 0 || index >= m_layout.itemRects.size()) {
        return {};
    }
    const QRect local = m_layout.itemRects.at(index);
    return QRect(mapToGlobal(local.topLeft()), local.size());
}

ContextMenu *ContextMenu::rootMenu()
{
    ContextMenu *menu = this;
    while (menu->m_parentMenu) {
        menu = menu->m_parentMenu;
    }
    return menu;
}

void ContextMenu::triggerAction(QAction *action)
{
    ContextMenu *root = rootMenu();
    if (!action || root->m_finishing) {
        return;
    }
    // 1. 记录需要转发 triggered 的菜单链，并以弱引用保护动作与菜单的生命周期
    QList<QPointer<ContextMenu>> chain;
    for (ContextMenu *menu = this; menu; menu = menu->m_parentMenu) {
        chain.append(menu);
    }
    const QPointer<QAction> guardAction(action);
    const QPointer<ContextMenu> guardRoot(root);
    root->m_finishing = true;
    root->m_result = action;

    // 2. 先关闭整棵菜单，再交付动作，回调中销毁菜单或其所属窗口均安全
    root->hide();
    if (guardAction && guardAction->isEnabled()) {
        guardAction->activate(QAction::Trigger);
    }
    for (const QPointer<ContextMenu> &menu : std::as_const(chain)) {
        if (menu && guardAction) {
            emit menu->triggered(guardAction.data());
        }
    }

    // 3. 交付完成后结束会话
    if (guardRoot) {
        guardRoot->m_finishing = false;
        emit guardRoot->popupFinished();
    }
}

}  // namespace markshot::ui
