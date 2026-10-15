#pragma once

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace markshot::test {

/** 将当前测试进程隔离到 home，返回设置结果；必须在创建 QApplication 前调用。 */
inline bool isolateConfig(const QTemporaryDir &home)
{
    if (!home.isValid()) {
        return false;
    }
    for (const char *name : {"HOME", "USERPROFILE"}) {
        qputenv(name, home.path().toUtf8());
    }
    for (const char *name : {"XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME", "XDG_RUNTIME_DIR",
                             "APPDATA", "LOCALAPPDATA", "MARK_SHOT_CONFIG_DIR"}) {
        const QString path = home.filePath(QString::fromLatin1(name));
        if (!QDir().mkpath(path)) {
            return false;
        }
        qputenv(name, path.toUtf8());
    }
    QFile::setPermissions(qEnvironmentVariable("XDG_RUNTIME_DIR"),
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    return true;
}

} // namespace markshot::test
