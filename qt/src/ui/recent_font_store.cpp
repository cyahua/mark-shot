#include "ui/recent_font_store.h"

#include "app_config_store.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QSet>

namespace markshot::ui {
namespace {

/// @brief 去掉空白并按出现顺序去重。
/// @param history 原字体列表。
/// @param limit 最大保留数量。
/// @return 归一化后的字体列表。
QStringList normalizedRecentFonts(const QStringList &history, int limit)
{
    QStringList result;
    if (limit <= 0) {
        return result;
    }
    for (const QString &item : history) {
        const QString family = item.trimmed();
        if (family.isEmpty() || result.contains(family)) {
            continue;
        }
        result.push_back(family);
        if (result.size() >= limit) {
            break;
        }
    }
    return result;
}

/// @brief 将最近使用字体转换成 JSON 数组。
/// @param history 最近使用字体列表。
/// @return JSON 数组。
QJsonArray recentFontsToJsonArray(const QStringList &history)
{
    QJsonArray array;
    for (const QString &family : history) {
        if (!family.isEmpty()) {
            array.append(family);
        }
    }
    return array;
}

}  // namespace

QStringList recentFontsWithRememberedFont(const QStringList &history, const QString &fontFamily, int limit)
{
    if (limit <= 0) {
        return {};
    }
    const QString family = fontFamily.trimmed();
    if (family.isEmpty()) {
        return normalizedRecentFonts(history, limit);
    }

    QStringList result;
    result.push_back(family);
    for (const QString &item : history) {
        const QString existing = item.trimmed();
        if (existing.isEmpty() || result.contains(existing)) {
            continue;
        }
        result.push_back(existing);
        if (result.size() >= limit) {
            break;
        }
    }
    return result;
}

QStringList recentFontsFromConfigRoot(const QJsonObject &root, int limit)
{
    const QJsonValue textValue = root.value(QStringLiteral("text"));
    const QJsonArray fonts = textValue.isObject()
        ? textValue.toObject().value(QStringLiteral("recentFonts")).toArray()
        : QJsonArray();
    QStringList parsedFonts;
    for (const QJsonValue &item : fonts) {
        if (item.isString()) {
            parsedFonts.push_back(item.toString());
        }
    }
    return normalizedRecentFonts(parsedFonts, limit);
}

QStringList visibleRecentFonts(const QStringList &history, const QStringList &availableFamilies, int limit)
{
    const QSet<QString> available(availableFamilies.cbegin(), availableFamilies.cend());
    QStringList installed;
    for (const QString &family : normalizedRecentFonts(history, limit)) {
        if (available.contains(family)) {
            installed.push_back(family);
        }
    }
    return installed;
}

QStringList readRecentFonts(int limit)
{
    bool ok = false;
    const QJsonObject root = readAppConfigRoot(&ok);
    return ok ? recentFontsFromConfigRoot(root, limit) : QStringList();
}

bool rememberRecentFont(const QString &fontFamily, QString *error)
{
    if (fontFamily.trimmed().isEmpty()) {
        return true;
    }
    const QStringList history = recentFontsWithRememberedFont(readRecentFonts(), fontFamily);
    return writeAppConfigValue({QStringLiteral("text"), QStringLiteral("recentFonts")},
                               recentFontsToJsonArray(history),
                               error);
}

}  // namespace markshot::ui
