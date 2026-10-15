#pragma once

#include <QString>

namespace markshot::ui {

/**
 * 生成图片编辑画布的细窄圆角滚动条样式
 * @param extent 滚动条占用宽度，与画布定位使用的尺寸一致
 * @return 可应用于 QScrollBar 的统一样式表
 */
QString canvasScrollBarStyleSheet(int extent);

}  // namespace markshot::ui
