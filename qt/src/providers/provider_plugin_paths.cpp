#include "providers/provider_plugin_paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace markshot::providers {
namespace {

/**
 * 追加去重后的插件搜索目录。
 * @param dirs 目录列表。
 * @param path 待追加路径。
 * @return 无返回值。
 */
void addSearchDir(QStringList *dirs, const QString &path)
{
    const QString absolute = QDir(path).absolutePath();
    if (!absolute.isEmpty() && !dirs->contains(absolute)) {
        dirs->append(absolute);
    }
}

/**
 * 读取跨平台应用数据目录。
 * @return 应用数据目录。
 */
QString appDataDirectory()
{
#ifdef Q_OS_WIN
    const QString appLocalData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!appLocalData.isEmpty()) {
        return appLocalData;
    }
    return QDir::home().filePath(QStringLiteral("AppData/Local/mark-shot"));
#else
    const QString dataHome = qEnvironmentVariable("XDG_DATA_HOME").trimmed();
    const QString userBase = dataHome.isEmpty()
        ? QDir::home().filePath(QStringLiteral(".local/share"))
        : dataHome;
    return QDir(userBase).filePath(QStringLiteral("mark-shot"));
#endif
}

}  // namespace

QString userPluginDirectory()
{
    return QDir(appDataDirectory()).filePath(QStringLiteral("plugins"));
}

QStringList pluginSearchDirs()
{
    QStringList dirs;
    const QString appDir = QCoreApplication::applicationDirPath();
    // 1. 【插件】【用户更新】市场安装的新版本优先于安装包附带的旧版本
    addSearchDir(&dirs, userPluginDirectory());
    // 2. 系统级与应用相邻目录，支持安装包和免安装目录
    addSearchDir(&dirs, QDir(appDir).filePath(QStringLiteral("plugins")));
    addSearchDir(&dirs, QDir(appDir).filePath(QStringLiteral("../lib/mark-shot/plugins")));
    addSearchDir(&dirs, QDir(appDir).filePath(QStringLiteral("../lib64/mark-shot/plugins")));
    for (const QString &path : QCoreApplication::libraryPaths()) {
        addSearchDir(&dirs, QDir(path).filePath(QStringLiteral("mark-shot/plugins")));
    }
    return dirs;
}

}  // namespace markshot::providers
