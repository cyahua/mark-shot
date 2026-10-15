#pragma once

#include <QString>

namespace markshot::settings::texts {

// 设置字段新增文案，按当前界面语言返回，避免修改公共翻译表

/// @brief 快捷键胶囊未聚焦时的提示
/// @return 本地化文本
QString shortcutIdlePlaceholder();

/// @brief 快捷键胶囊录入中的提示
/// @return 本地化文本
QString shortcutRecordingPlaceholder();

/// @brief 清除快捷键图标的提示
/// @return 本地化文本
QString clearShortcut();

/// @brief 浏览按钮文本
/// @return 本地化文本
QString browse();

/// @brief 选择文件夹对话框标题
/// @return 本地化文本
QString selectFolder();

/// @brief 选择文件对话框标题
/// @return 本地化文本
QString selectFile();

/// @brief 选择命令程序对话框标题
/// @return 本地化文本
QString selectProgram();

/// @brief 撤销未保存修改按钮文本
/// @return 本地化文本
QString revertUnsaved();

}  // namespace markshot::settings::texts
