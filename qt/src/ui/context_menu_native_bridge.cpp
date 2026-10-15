#include "ui/context_menu_native_bridge.h"

#include "ui/context_menu.h"

#include <QMenu>
#include <QPointer>

namespace markshot::ui {

QMenu *createNativeMenuMirror(ContextMenu *source, QWidget *parent)
{
    auto *mirror = new QMenu(parent);
    if (!source) {
        return mirror;
    }
    mirror->setTitle(source->title());
    const QPointer<ContextMenu> sourceGuard(source);

    // 1. 原生菜单打开时先让源菜单刷新动作状态，触发后同步发出源菜单的 triggered
    QObject::connect(mirror, &QMenu::aboutToShow, source, [sourceGuard] {
        if (sourceGuard) {
            emit sourceGuard->aboutToShow();
        }
    });
    QObject::connect(mirror, &QMenu::aboutToHide, source, [sourceGuard] {
        if (sourceGuard) {
            emit sourceGuard->aboutToHide();
        }
    });
    QObject::connect(mirror, &QMenu::triggered, source, [sourceGuard](QAction *action) {
        if (sourceGuard) {
            emit sourceGuard->triggered(action);
        }
    });

    // 2. 普通动作直接共享，子菜单递归生成镜像并沿用菜单行动作的文案与图标
    for (QAction *action : source->actions()) {
        ContextMenu *submenu = source->menuForAction(action);
        if (!submenu) {
            mirror->addAction(action);
            continue;
        }
        QMenu *child = createNativeMenuMirror(submenu, mirror);
        child->setIcon(action->icon());
        child->menuAction()->setEnabled(action->isEnabled());
        child->menuAction()->setVisible(action->isVisible());
        QObject::connect(action, &QAction::changed, child, [child, action] {
            child->setTitle(action->text());
            child->setIcon(action->icon());
            child->menuAction()->setEnabled(action->isEnabled());
            child->menuAction()->setVisible(action->isVisible());
        });
        mirror->addMenu(child);
    }
    return mirror;
}

}  // namespace markshot::ui
