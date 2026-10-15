#pragma once

#include <QString>

class QScrollBar;

namespace markshot::ui {

/**
 * 生成图片编辑画布的细窄圆角滚动条样式
 * @param extent 滚动条占用宽度，与画布定位使用的尺寸一致
 * @return 可应用于 QScrollBar 的统一样式表
 */
QString canvasScrollBarStyleSheet(int extent);

/**
 * 为画布滚动条应用样式与悬停行为：箭头光标、悬停时加宽加亮滑块，并让画布结束自身的悬停预览
 * @param bar 画布上方的滚动条，父控件为画布窗口
 * @param extent 滚动条占用宽度
 * @return 无返回值
 */
void setupCanvasScrollBar(QScrollBar *bar, int extent);

}  // namespace markshot::ui
