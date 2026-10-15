#include "marketplace/plugin_updates.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QSaveFile>

namespace markshot::marketplace {
namespace {

constexpr auto kPendingDirectory = ".pending-updates";
constexpr auto kInstallLock = ".install.lock";

/**
 * 【插件】【原子写入】复制完整文件后原子替换目标，禁止直接覆盖已占用文件
 * @param sourcePath 源文件路径
 * @param destinationPath 目标文件路径
 * @param error 输出文件操作错误
 * @return 写入成功时返回 true
 */
bool copyAtomically(const QString &sourcePath, const QString &destinationPath, QString *error)
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        *error = source.errorString();
        return false;
    }
    QSaveFile destination(destinationPath);
    destination.setDirectWriteFallback(false);
    if (!destination.open(QIODevice::WriteOnly)) {
        *error = destination.errorString();
        return false;
    }
    while (!source.atEnd()) {
        const QByteArray data = source.read(1024 * 1024);
        if (source.error() != QFileDevice::NoError) {
            *error = source.errorString();
            return false;
        }
        if (destination.write(data) != data.size()) {
            *error = destination.errorString();
            return false;
        }
    }
    if (!destination.commit()) {
        *error = destination.errorString();
        return false;
    }
    return true;
}

}

PluginInstallResult storePluginAsset(const QString &sourcePath, const QString &destinationPath)
{
    PluginInstallResult result;
    const QFileInfo destination(destinationPath);
    QDir directory = destination.dir();
    QLockFile lock(directory.filePath(QLatin1String(kInstallLock)));
    if (!lock.tryLock(0)) {
        result.error = QStringLiteral("Another plugin installation is in progress");
        return result;
    }
    // 1. 【插件】【占用更新】不卸载运行中的插件，所有替换操作统一留待下次启动
    const QString pendingPath = directory.filePath(QLatin1String(kPendingDirectory) + QLatin1Char('/') + destination.fileName());
    const bool updating = destination.exists() || QFileInfo::exists(pendingPath);
    QString writePath = destinationPath;
    if (updating) {
        if (!directory.mkpath(QLatin1String(kPendingDirectory))) {
            result.error = QStringLiteral("Failed to create plugin update directory");
            return result;
        }
        writePath = pendingPath;
    }
    if (!copyAtomically(sourcePath, writePath, &result.error)) {
        return result;
    }
    result.success = true;
    result.installedPath = destinationPath;
    result.pendingRestart = updating;
    return result;
}

QStringList applyPendingPluginUpdates(const QString &pluginDirectory)
{
    QDir directory(pluginDirectory);
    QDir pending(directory.filePath(QLatin1String(kPendingDirectory)));
    if (!pending.exists()) {
        return {};
    }
    QLockFile lock(directory.filePath(QLatin1String(kInstallLock)));
    if (!lock.tryLock(0)) {
        return {QStringLiteral("Another plugin installation is in progress")};
    }
    QStringList errors;
    for (const QFileInfo &file : pending.entryInfoList(QDir::Files | QDir::NoSymLinks)) {
        if (!isSupportedPluginLibraryFile(file.fileName())) {
            continue;
        }
        // 1. 【插件】【启动更新】其他进程仍占用 DLL 时提交失败，保留两份文件供下次重试
        QString error;
        if (!copyAtomically(file.absoluteFilePath(), directory.filePath(file.fileName()), &error)) {
            errors.append(QStringLiteral("%1: %2").arg(file.fileName(), error));
            continue;
        }
        if (!QFile::remove(file.absoluteFilePath())) {
            errors.append(QStringLiteral("Failed to remove staged update: %1").arg(file.fileName()));
        }
    }
    directory.rmdir(QLatin1String(kPendingDirectory));
    return errors;
}

}
