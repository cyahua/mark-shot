#pragma once

#include <QDir>
#include <QStandardPaths>
#include <QString>

namespace markshot::plugin {

/**
 * 【OCR】【模型目录】读取下载器和插件共用的默认模型目录
 * @return Windows 的 LocalAppData 或其他平台标准数据目录下的 mark-shot/models
 */
inline QString defaultOcrModelDirectory()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (base.isEmpty()) {
        base = QDir::home().filePath(QStringLiteral(".local/share"));
    }
    return QDir(base).filePath(QStringLiteral("mark-shot/models"));
}

/**
 * 【OCR】【模型目录】优先读取用户指定的模型目录
 * @return MARK_SHOT_OCR_MODEL_DIR 或默认模型目录
 */
inline QString ocrModelDirectory()
{
    const QString configured = qEnvironmentVariable("MARK_SHOT_OCR_MODEL_DIR").trimmed();
    return configured.isEmpty() ? defaultOcrModelDirectory() : configured;
}

}
