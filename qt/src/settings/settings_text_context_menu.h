#pragma once

class QLineEdit;
class QPlainTextEdit;

namespace markshot::settings {

/// @brief 为单行输入框安装本地化右键编辑菜单
/// @param edit 目标输入框
/// @return 无返回值
void installLineEditContextMenu(QLineEdit *edit);

/// @brief 为多行输入框安装本地化右键编辑菜单
/// @param edit 目标多行输入框
/// @return 无返回值
void installPlainTextEditContextMenu(QPlainTextEdit *edit);

}  // namespace markshot::settings
