#pragma once

class QMenu;
class QWidget;

namespace markshot::ui {

class ContextMenu;

/**
 * 为必须交给平台渲染的场景（Linux StatusNotifierItem 托盘通过 DBusMenu 导出）
 * 生成共享同一组 QAction 的原生菜单镜像。镜像不复制动作，启用、勾选与文案状态天然同步；
 * aboutToShow 与 triggered 会转发回源菜单，源菜单中的刷新逻辑无需区分渲染方式。
 * 限制：镜像按创建时的结构生成，之后新增或移除的动作与子菜单不会同步。
 * @param source 自绘菜单。
 * @param parent 镜像菜单的父控件，可为空。
 * @return 新建的原生菜单，调用方负责其生命周期。
 */
QMenu *createNativeMenuMirror(ContextMenu *source, QWidget *parent = nullptr);

}  // namespace markshot::ui
