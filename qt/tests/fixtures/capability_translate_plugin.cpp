#include "markshot/translate_provider_plugin.h"

#include <QObject>

namespace {

/**
 * 动态库被映射时写入进程环境变量，供测试判断插件是否被加载。
 */
struct LoadMarker {
    LoadMarker() { qputenv("MARK_SHOT_TEST_CAPABILITY_FIXTURE_LOADED", "1"); }
};

const LoadMarker loadMarker;

}  // namespace

class CapabilityTranslateFixture final : public QObject, public markshot::plugin::TranslateProviderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MARK_SHOT_TRANSLATE_PROVIDER_PLUGIN_IID FILE "capability_translate_plugin.json")
    Q_INTERFACES(markshot::plugin::TranslateProviderPlugin)
public:
    /// @brief 返回测试 provider 标识，无参数
    /// @return 测试 provider 标识
    QString providerId() const override { return QStringLiteral("capability-fixture"); }

    /// @brief 返回测试展示名称，无参数
    /// @return 展示名称
    QString displayName() const override { return QStringLiteral("Capability Fixture"); }

    /// @brief 测试插件始终可用
    /// @param error 不可用原因
    /// @return true
    bool isAvailable(QString *error) const override
    {
        if (error) error->clear();
        return true;
    }

    /// @brief 测试库不提供实际翻译
    /// @param segments 输入分段
    /// @param targetLanguage 目标语言
    /// @param translations 输出译文
    /// @param error 输出错误
    /// @return false
    bool translate(const QVector<markshot::plugin::TranslateSegment> &segments,
                   const QString &targetLanguage,
                   QVector<markshot::plugin::TranslateSegment> *translations,
                   QString *error) override
    {
        Q_UNUSED(segments);
        Q_UNUSED(targetLanguage);
        translations->clear();
        if (error) *error = QStringLiteral("Fixture only");
        return false;
    }
};

#include "capability_translate_plugin.moc"
