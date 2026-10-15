#include "config_value.h"
#include "json_bridge.h"

#include <algorithm>

namespace markshot::config {

QJsonObject objectValue(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isObject() ? value.toObject() : QJsonObject();
}

QJsonObject firstObjectValue(const QJsonObject &object, const QString &key)
{
    return objectValue(object, key);
}

QJsonObject firstObjectValue(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (value.isObject()) {
            return value.toObject();
        }
    }
    return {};
}

QJsonObject firstNonEmptyObjectValue(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonObject child = objectValue(object, key);
        if (!child.isEmpty()) {
            return child;
        }
    }
    return {};
}

QJsonValue valueForKeys(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (!value.isUndefined()) {
            return value;
        }
    }
    return {};
}

/** 归一化 key 的分隔符与大小写，返回 Rust 核心处理后的键名。 */
QString normalizedKey(QString key)
{
    ShotBytes *output = nullptr;
    if (!rust::configSucceeded(shot_config_normalize_key(rust::byteView(key.toUtf8()), &output))) {
        return {};
    }
    return QString::fromUtf8(rust::takeBytes(output).value_or(QByteArray{}));
}

/** 从 value 读取兼容布尔值，返回解析结果或空值，不在 Qt 重复执行校验规则。 */
std::optional<bool> boolValue(const QJsonValue &value)
{
    ShotConfigScalar result{};
    return rust::configSucceeded(shot_config_scalar(rust::byteView(rust::jsonValue(value)), &result))
            && result.boolean_valid
        ? std::optional<bool>(result.boolean_value != 0) : std::nullopt;
}

std::optional<bool> boolValue(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (value.isBool()) {
            return value.toBool();
        }
    }
    return std::nullopt;
}

/** 从 value 读取兼容整数，返回解析结果或空值，沿用历史 JSON 数字规则。 */
std::optional<int> intValue(const QJsonValue &value)
{
    ShotConfigScalar result{};
    return rust::configSucceeded(shot_config_scalar(rust::byteView(rust::jsonValue(value)), &result))
            && result.integer_valid
        ? std::optional<int>(result.integer_value) : std::nullopt;
}

std::optional<int> clampedIntValue(const QJsonValue &value, int minimum, int maximum)
{
    if (const std::optional<int> number = intValue(value)) {
        return std::clamp(*number, minimum, maximum);
    }
    return std::nullopt;
}

std::optional<QString> environmentStringValue(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 15);
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("1") : QStringLiteral("0");
    }
    return std::nullopt;
}

std::optional<QKeySequence> keySequenceValue(const QJsonValue &value)
{
    if (!value.isString()) {
        return std::nullopt;
    }

    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return std::nullopt;
    }

    QKeySequence sequence(text, QKeySequence::PortableText);
    if (sequence.isEmpty()) {
        sequence = QKeySequence(text, QKeySequence::NativeText);
    }
    return sequence.isEmpty() ? std::nullopt : std::optional<QKeySequence>(sequence);
}

}  // namespace markshot::config
