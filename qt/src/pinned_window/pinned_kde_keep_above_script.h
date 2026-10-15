#pragma once

#include <QString>

namespace markshot::shot {

/// @brief 判断初始标题是否属于应用管理的钉图或 OCR 窗口
/// @param title 尚未添加窗口编号的标题
/// @return 支持的窗口标题返回 true
bool isKdePinnedKeepAboveTitle(const QString &title);

/// @brief 生成订阅应用状态并逐窗口应用置顶的 KWin 脚本
/// @param service 当前应用的 D-Bus 唯一服务名
/// @param processId 创建窗口的进程号
/// @param pluginName 应用脚本的唯一名称
/// @return 脚本源码，身份不完整时返回空字符串
QString kdePinnedKeepAboveScriptSource(const QString &service, qint64 processId, const QString &pluginName);

}  // namespace markshot::shot
