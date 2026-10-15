#pragma once

#include "shot_window.h"
#include "ui/icons.h"

#include <QString>

namespace markshot::ui {

/**
 * 返回工具栏动作对应的统一 SVG 图标名称
 * @param action 工具栏动作
 * @return 图标资源名称，不含扩展名
 */
QString toolIconName(ShotWindow::Action action);

/**
 * 返回属性面板控件对应的统一 SVG 图标名称
 * @param icon 属性面板图标类型
 * @return 图标资源名称，不含扩展名
 */
QString propertyIconName(PropertyIcon icon);

}  // namespace markshot::ui
