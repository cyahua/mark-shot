#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>

namespace markshot::ui {

/**
 * 将矩形平移到可用区域内；矩形大于可用区域时贴齐左上角。
 * @param rect 待调整矩形。
 * @param bounds 可用区域。
 * @return 平移后的矩形。
 */
QRect clampContextMenuRect(QRect rect, const QRect &bounds);

/**
 * 计算根菜单面板位置：默认位于锚点右下方，越界时翻转到左侧或上方。
 * @param anchor 鼠标或调用方给出的全局锚点。
 * @param panelSize 面板尺寸（不含阴影边距）。
 * @param bounds 屏幕可用区域。
 * @return 面板全局矩形。
 */
QRect placeContextMenuAtPoint(const QPoint &anchor, const QSize &panelSize, const QRect &bounds);

/**
 * 计算挂在控件下方的菜单面板位置，下方空间不足时翻转到控件上方。
 * @param anchorRect 触发控件的全局矩形。
 * @param panelSize 面板尺寸（不含阴影边距）。
 * @param bounds 屏幕可用区域。
 * @param gap 面板与控件之间的间距。
 * @return 面板全局矩形。
 */
QRect placeContextMenuBelow(const QRect &anchorRect, const QSize &panelSize, const QRect &bounds, int gap);

/**
 * 计算子菜单面板位置：默认位于父面板右侧并与父菜单行顶部对齐，越界时翻转到左侧。
 * @param parentPanel 父菜单面板全局矩形。
 * @param parentItem 打开子菜单的父菜单行全局矩形。
 * @param panelSize 子菜单面板尺寸（不含阴影边距）。
 * @param bounds 屏幕可用区域。
 * @param overlap 子面板与父面板重叠的像素数。
 * @param itemInset 子菜单首行相对其面板顶部的偏移，用于首行与父行对齐。
 * @return 子菜单面板全局矩形。
 */
QRect placeContextSubmenu(const QRect &parentPanel,
                          const QRect &parentItem,
                          const QSize &panelSize,
                          const QRect &bounds,
                          int overlap,
                          int itemInset);

}  // namespace markshot::ui
