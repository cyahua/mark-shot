#include "app_config_store.h"

#include "json_bridge.h"
#include "window_detection.h"

namespace markshot {

/** 读取应用配置，ok 接收读取结果，返回 JSON 对象；失败时返回空对象。 */
QJsonObject readAppConfigRoot(bool *ok)
{
    if (ok) {
        *ok = false;
    }
    if (!ensureAppConfigFile()) {
        return {};
    }
    const auto root = rust::readConfigObject(appConfigPath());
    if (ok) {
        *ok = root.has_value();
    }
    return root.value_or(QJsonObject{});
}

/** 把 root 原子写入应用配置，error 接收失败原因，返回保存结果。 */
bool writeAppConfigRoot(const QJsonObject &root, QString *error)
{
    return rust::configSucceeded(shot_config_write(
        rust::byteView(appConfigPath().toUtf8()),
        rust::byteView(QJsonDocument(root).toJson(QJsonDocument::Compact))), error);
}

/** 原子更新 path 对应的 value，error 接收失败原因，返回保存结果；undefined 表示删除。 */
bool writeAppConfigValue(const QStringList &path, const QJsonValue &value, QString *error)
{
    if (path.isEmpty() || !ensureAppConfigFile()) {
        if (error) {
            *error = QStringLiteral("Cannot initialize application config or key path is empty");
        }
        return false;
    }
    return rust::configSucceeded(shot_config_patch(
        rust::byteView(appConfigPath().toUtf8()), rust::byteView(rust::configChange(path, value))), error);
}

} // namespace markshot
