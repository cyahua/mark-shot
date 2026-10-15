#pragma once

#include "marketplace/plugin_installer.h"

#include <QStringList>

namespace markshot::marketplace {

/**
 * 【插件】【安装存储】首次安装直接写入，已有插件暂存到独立更新目录
 * @param sourcePath 已校验的动态库路径
 * @param destinationPath 最终安装路径
 * @return 安装结果，暂存更新时 pendingRestart 为 true
 */
PluginInstallResult storePluginAsset(const QString &sourcePath, const QString &destinationPath);

/**
 * 【插件】【启动更新】加载插件前原子替换待更新文件，失败时保留旧库和更新包
 * @param pluginDirectory 用户插件目录
 * @return 未能应用的更新错误列表
 */
QStringList applyPendingPluginUpdates(const QString &pluginDirectory);

}
