#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace markshot::ui {

/// @brief 字体选择器最近使用列表开关。关闭时不读写、不插入分区。
inline constexpr bool FEATURE_RECENT_TEXT_FONTS_ENABLED = true;

/// @brief 最近使用字体的保留数量。字体弹层最多同时露出约 10 行，5 条最近字体仍能看到完整列表的开头。
inline constexpr int kRecentFontLimit = 5;

/// @brief 生成加入新字体后的最近使用列表。
/// @param history 原最近使用字体，靠前的更新。
/// @param fontFamily 新选中的字体系列。
/// @param limit 最大保留数量。
/// @return 去重、去空白并限制数量后的列表，最新字体在最前。
QStringList recentFontsWithRememberedFont(const QStringList &history,
                                          const QString &fontFamily,
                                          int limit = kRecentFontLimit);

/// @brief 从应用配置根对象读取最近使用字体。
/// @param root 应用配置根对象。
/// @param limit 最大保留数量。
/// @return 最近使用字体列表。
QStringList recentFontsFromConfigRoot(const QJsonObject &root, int limit = kRecentFontLimit);

/// @brief 只保留当前系统仍安装的最近使用字体。
/// @param history 已保存的最近使用字体。
/// @param availableFamilies 当前系统字体系列。
/// @param limit 最大保留数量。
/// @return 仍可选用的最近使用字体，顺序与 history 一致。
QStringList visibleRecentFonts(const QStringList &history,
                               const QStringList &availableFamilies,
                               int limit = kRecentFontLimit);

/// @brief 从应用配置文件读取最近使用字体。
/// @param limit 最大保留数量。
/// @return 最近使用字体列表；读取失败时返回空列表。
QStringList readRecentFonts(int limit = kRecentFontLimit);

/// @brief 将字体系列记录到最近使用列表。
/// @param fontFamily 字体系列名称。
/// @param error 失败时写入错误信息。
/// @return 空名称或写入成功时返回 true，否则返回 false。
bool rememberRecentFont(const QString &fontFamily, QString *error = nullptr);

}  // namespace markshot::ui
