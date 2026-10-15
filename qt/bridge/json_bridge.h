#pragma once

#include "shot_config_ffi.h"

#include <QByteArray>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <memory>
#include <optional>

namespace markshot::rust {

/** 将 bytes 转为同步借用视图，返回不拥有内存的字节范围。 */
inline ShotBytesView byteView(const QByteArray &bytes)
{
    return {reinterpret_cast<const uint8_t *>(bytes.constData()), static_cast<size_t>(bytes.size())};
}

/** 读取当前线程的接口错误，返回 UTF-8 错误文本，不发起其他 Rust 调用。 */
inline QString lastError()
{
    QByteArray buffer(static_cast<qsizetype>(shot_last_error(nullptr, 0)), '\0');
    shot_last_error(reinterpret_cast<uint8_t *>(buffer.data()), static_cast<size_t>(buffer.size()));
    return QString::fromUtf8(buffer.constData());
}

/** 检查 status 并在失败时填写 error，返回是否成功，不记录配置内容。 */
inline bool configSucceeded(int status, QString *error = nullptr)
{
    if (status == 0) {
        if (error) {
            error->clear();
        }
        return true;
    }
    if (error) {
        *error = lastError();
    }
    return false;
}

/** 接管 handle 并复制内容，返回独立字节；error 接收失败原因，句柄始终释放。 */
inline std::optional<QByteArray> takeBytes(ShotBytes *handle, QString *error = nullptr)
{
    const std::unique_ptr<ShotBytes, decltype(&shot_bytes_destroy)> owner(handle, shot_bytes_destroy);
    ShotBytesView view{};
    if (!configSucceeded(shot_bytes_view(owner.get(), &view), error)) {
        return std::nullopt;
    }
    return QByteArray(reinterpret_cast<const char *>(view.data), static_cast<qsizetype>(view.len));
}

/** 接管 handle 的 JSON 对象，返回 Qt 数据；error 接收解码错误。 */
inline std::optional<QJsonObject> takeObject(ShotBytes *handle, QString *error = nullptr)
{
    const auto bytes = takeBytes(handle, error);
    if (!bytes) {
        return std::nullopt;
    }
    const QJsonDocument document = QJsonDocument::fromJson(*bytes);
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("Rust core returned an invalid JSON object");
        }
        return std::nullopt;
    }
    return document.object();
}

/** 从明确的 path 读取配置对象，error 接收失败原因，返回对象或空值，不创建缺失文件。 */
inline std::optional<QJsonObject> readConfigObject(const QString &path, QString *error = nullptr)
{
    ShotBytes *output = nullptr;
    const QByteArray absolute = path.isEmpty() ? QByteArray{} : QFileInfo(path).absoluteFilePath().toUtf8();
    if (!configSucceeded(shot_config_read(byteView(absolute), &output), error)) {
        return std::nullopt;
    }
    return takeObject(output, error);
}

/** 编码 value 为独立 JSON 值，返回字节；undefined 用 null 表示，删除另行标记。 */
inline QByteArray jsonValue(const QJsonValue &value)
{
    const QByteArray array = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
    return array.mid(1, array.size() - 2);
}

/** 将 path 和 value 编码为局部更新请求，返回 JSON 字节；undefined 表示删除。 */
inline QByteArray configChange(const QStringList &path, const QJsonValue &value)
{
    return QJsonDocument(QJsonObject{
        {QStringLiteral("path"), QJsonArray::fromStringList(path)},
        {QStringLiteral("value"), value},
        {QStringLiteral("remove"), value.isUndefined()},
    }).toJson(QJsonDocument::Compact);
}

/** 在 object 中应用 path/value，返回成功标记；仅负责 Qt 与 Rust 类型转换。 */
inline bool updateObject(QJsonObject *object, const QStringList &path, const QJsonValue &value)
{
    if (!object) {
        return false;
    }
    ShotBytes *output = nullptr;
    if (!configSucceeded(shot_config_update(byteView(QJsonDocument(*object).toJson(QJsonDocument::Compact)),
                                            byteView(configChange(path, value)), &output))) {
        return false;
    }
    const auto root = takeObject(output);
    if (!root) {
        return false;
    }
    *object = *root;
    return true;
}

} // namespace markshot::rust
