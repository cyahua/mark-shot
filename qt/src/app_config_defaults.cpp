#include "app_config_defaults.h"

#include "json_bridge.h"
#include "ui/theme.h"

namespace markshot {

/** 根据 windowDetectionCommand 与当前平台生成默认配置，返回 Qt JSON 对象。 */
QJsonObject defaultAppConfigRoot(const QString &windowDetectionCommand)
{
    const QJsonObject context{
        {QStringLiteral("windowDetectionCommand"), windowDetectionCommand},
        {QStringLiteral("pinnedBorderColor"), theme::kAccent.name(QColor::HexRgb).toUpper()},
#if defined(Q_OS_WIN)
        {QStringLiteral("windows"), true},
#else
        {QStringLiteral("windows"), false},
#endif
    };
    ShotBytes *output = nullptr;
    if (!rust::configSucceeded(shot_config_defaults(
            rust::byteView(QJsonDocument(context).toJson(QJsonDocument::Compact)), &output))) {
        return {};
    }
    return rust::takeObject(output).value_or(QJsonObject{});
}

} // namespace markshot
