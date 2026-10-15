#pragma once

class QApplication;

namespace markshot::ui {

/// @brief 为通用控件安装光标规则，覆盖可用状态、文本输入与拖动生命周期
/// @param application 当前图形应用，重复调用不会重复安装
/// @return 无返回值
void installInteractionCursorPolicy(QApplication *application);

}
