#pragma once

#include "ui/context_menu_model.h"
#include "ui/context_menu_style.h"

#include <QAction>
#include <QElapsedTimer>
#include <QHash>
#include <QPointer>
#include <QTimer>
#include <QWidget>

class QEventLoop;

namespace markshot::ui {

/**
 * QWidget 自绘上下文菜单，以 QAction 承载动作状态，接口与 QMenu 常用部分保持一致，
 * 便于从 QMenu 迁移。addAction 系列重载直接继承自 QWidget（Qt 6.3+）。
 * 根菜单与其子菜单构成一次弹出会话：外部点击、Esc 或失去激活时整棵菜单关闭，
 * 选中动作在整棵菜单隐藏后才触发，避免回调中销毁菜单所属窗口导致悬空访问。
 */
class ContextMenu final : public QWidget {
    Q_OBJECT
public:
    // 明暗主题：Inherit 依次继承父菜单、所属控件调色板与系统色彩方案
    enum class ColorScheme { Inherit, Light, Dark };
    Q_ENUM(ColorScheme)

    /**
     * 创建上下文菜单。
     * @param parent 所属控件，菜单作为其弹出窗口并随其销毁。
     */
    explicit ContextMenu(QWidget *parent = nullptr);

    /**
     * 创建带标题的菜单，标题作为挂到父菜单时的菜单行文案。
     * @param title 菜单标题。
     * @param parent 所属控件。
     */
    explicit ContextMenu(const QString &title, QWidget *parent = nullptr);

    /**
     * 结束尚未完成的弹出会话，并关闭仍可见的子菜单。
     */
    ~ContextMenu() override;

    /**
     * 追加分隔线。
     * @return 菜单持有的分隔动作。
     */
    QAction *addSeparator();

    /**
     * 新建并追加子菜单，子菜单由本菜单持有。
     * @param title 子菜单标题。
     * @return 新建的子菜单。
     */
    ContextMenu *addMenu(const QString &title);

    /**
     * 新建并追加带图标的子菜单。
     * @param icon 菜单行图标。
     * @param title 子菜单标题。
     * @return 新建的子菜单。
     */
    ContextMenu *addMenu(const QIcon &icon, const QString &title);

    /**
     * 追加已存在的子菜单，所有权保持不变。
     * @param menu 子菜单。
     * @return 子菜单在本菜单中的菜单行动作。
     */
    QAction *addMenu(ContextMenu *menu);

    /**
     * 返回挂在某个菜单行上的子菜单。
     * @param action 菜单行动作。
     * @return 子菜单；未挂载时返回空指针。
     */
    ContextMenu *menuForAction(const QAction *action) const;

    /**
     * 返回代表本菜单的动作，用于挂入父菜单。
     * @return 菜单动作。
     */
    QAction *menuAction() const;

    /**
     * 设置菜单标题。
     * @param title 菜单标题。
     * @return 无返回值。
     */
    void setTitle(const QString &title);

    /**
     * 返回菜单标题。
     * @return 菜单标题。
     */
    QString title() const;

    /**
     * 移除全部动作，并删除由本菜单创建的动作与子菜单，与 QMenu::clear 一致。
     * @return 无返回值。
     */
    void clear();

    /**
     * 标记危险动作，危险动作以危险色绘制文案与悬停背景。
     * @param action 菜单动作。
     * @param danger 是否为危险动作。
     * @return 无返回值。
     */
    void setActionDanger(QAction *action, bool danger = true);

    /**
     * 查询动作是否为危险动作。
     * @param action 菜单动作。
     * @return 危险动作返回 true。
     */
    static bool actionDanger(const QAction *action);

    /**
     * 设置明暗主题。
     * @param scheme 主题选择。
     * @return 无返回值。
     */
    void setColorScheme(ColorScheme scheme);

    /**
     * 返回明暗主题设置。
     * @return 主题选择。
     */
    ColorScheme colorScheme() const;

    /**
     * 覆盖菜单令牌，覆盖后不再随主题变化。
     * @param style 完整令牌。
     * @return 无返回值。
     */
    void setComponentTokens(const ContextMenuStyle &style);

    /**
     * 清除令牌覆盖，恢复按主题生成令牌。
     * @return 无返回值。
     */
    void resetComponentTokens();

    /**
     * 返回菜单尺寸，包含四周阴影边距。
     * @return 菜单控件尺寸。
     */
    QSize sizeHint() const override;

    /**
     * 非阻塞弹出菜单，面板左上角位于锚点，越界时自动翻转。
     * 会话进行中重复调用只会移动面板，不会重复发出 aboutToShow。
     * @param position 全局锚点。
     * @param atAction 打开后高亮的动作，可为空。
     * @return 无返回值。
     */
    void popup(const QPoint &position, QAction *atAction = nullptr);

    /**
     * 在控件下方弹出菜单，下方空间不足时翻转到上方。
     * @param anchor 触发控件。
     * @return 无返回值。
     */
    void popupBelow(QWidget *anchor);

    /**
     * 阻塞弹出菜单，直到菜单关闭；已在显示或执行中时直接返回空指针，防止重入。
     * @param position 全局锚点。
     * @param atAction 打开后高亮的动作，可为空。
     * @return 被触发的动作；取消或菜单被销毁时返回空指针。
     */
    QAction *exec(const QPoint &position, QAction *atAction = nullptr);

    /**
     * 关闭本菜单所在的整棵菜单，不触发任何动作。
     * @return 无返回值。
     */
    void dismiss();

    /**
     * 设置高亮动作。
     * @param action 本菜单中的可见动作，空指针清除高亮。
     * @return 无返回值。
     */
    void setActiveAction(QAction *action);

    /**
     * 返回当前高亮动作。
     * @return 高亮动作；无高亮时返回空指针。
     */
    QAction *activeAction() const;

    /**
     * 返回动作所在菜单行的本地矩形。
     * @param action 菜单动作。
     * @return 本地矩形；动作不可见时返回空矩形。
     */
    QRect actionGeometry(QAction *action) const;

signals:
    void aboutToShow();
    void aboutToHide();
    void triggered(QAction *action);
    void hovered(QAction *action);
    // 整个弹出会话结束（含动作交付）后由根菜单发出
    void popupFinished();

protected:
    bool event(QEvent *event) override;
    void actionEvent(QActionEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // 生命周期与布局（context_menu.cpp）
    void initialize();
    void ensureLayout() const;
    void rebuild();
    ContextMenuStyle effectiveStyle() const;
    bool resolveDark() const;
    bool preparePopup();
    void showPanel(const QRect &panelGlobal);
    QRect screenBounds(const QPoint &anchor) const;
    QRect panelGlobalRect() const;
    QRect itemGlobalRect(int index) const;
    ContextMenu *rootMenu();
    void triggerAction(QAction *action);

    // 交互（context_menu_input.cpp）
    ContextMenu *menuAt(const QPoint &globalPoint);
    ContextMenu *deepestMenu();
    void hoverAt(const QPoint &globalPoint);
    void setActiveIndex(int index, bool fromHover);
    int nextSelectable(int from, int step) const;
    bool selectable(int index) const;
    void activateIndex(int index, bool fromKeyboard);
    void openSubmenu(int index, bool selectFirst);
    void closeSubmenu();
    void handleKey(QKeyEvent *event);
    bool handleMnemonic(const QString &text);
    void onSubmenuTimer();

    QAction *m_menuAction = nullptr;
    QHash<const QAction *, QPointer<ContextMenu>> m_submenus;
    QPointer<ContextMenu> m_parentMenu;
    QPointer<ContextMenu> m_openSubmenu;
    QList<QAction *> m_visibleActions;
    QVector<ContextMenuEntry> m_entries;
    ContextMenuLayout m_layout;
    ContextMenuStyle m_style;
    ContextMenuStyle m_customStyle;
    ColorScheme m_colorScheme = ColorScheme::Inherit;
    bool m_customTokens = false;
    bool m_dirty = true;
    bool m_preparing = false;
    bool m_finishing = false;
    bool m_executing = false;
    bool m_pressInside = false;
    int m_activeIndex = -1;
    int m_pendingSubmenuIndex = -1;
    QPoint m_popupCursor;
    QElapsedTimer m_shownTimer;
    QTimer m_submenuTimer;
    QPointer<QAction> m_result;
    QPointer<QEventLoop> m_loop;
    QMetaObject::Connection m_appStateConnection;
};

}  // namespace markshot::ui
