#include "markshot/translate_provider_plugin.h"
#include "providers/provider_plugin_registry.h"

#include <QStandardPaths>
#include <QtTest>

#include <algorithm>

class PluginRegistryCapabilityTest final : public QObject {
    Q_OBJECT
private slots:
    /// @brief 只在请求声明能力时加载插件，诊断列表加载全部候选，无参数和返回值
    void loadsOnlyDeclaredCapability()
    {
        qunsetenv("MARK_SHOT_TEST_CAPABILITY_FIXTURE_LOADED");
        auto &registry = markshot::providers::ProviderPluginRegistry::instance();

        // 1. 【插件测试】【按需加载】请求其他能力时不映射只声明翻译能力的插件
        QVERIFY(registry.ocrProviders().isEmpty());
        QVERIFY(registry.codeScanProviders().isEmpty());
        QVERIFY(!qEnvironmentVariableIsSet("MARK_SHOT_TEST_CAPABILITY_FIXTURE_LOADED"));

        // 2. 【插件测试】【按需加载】请求翻译能力时加载并返回插件
        const auto translators = registry.translateProviders();
        QVERIFY(qEnvironmentVariableIsSet("MARK_SHOT_TEST_CAPABILITY_FIXTURE_LOADED"));
        const auto it = std::find_if(translators.cbegin(), translators.cend(), [](const auto *plugin) {
            return plugin->providerId() == QStringLiteral("capability-fixture");
        });
        QVERIFY(it != translators.cend());

        // 3. 【插件测试】【诊断列表】诊断条目包含已加载插件且可用
        const auto infos = registry.pluginInfos();
        const auto info = std::find_if(infos.cbegin(), infos.cend(), [](const auto &entry) {
            return entry.providerId == QStringLiteral("capability-fixture");
        });
        QVERIFY(info != infos.cend());
        QVERIFY(info->matched);
        QVERIFY(info->available);
        QCOMPARE(info->capability, markshot::providers::ProviderPluginCapability::Translation);
    }
};

/**
 * 【插件测试】【入口】启用测试路径后执行用例
 * @param argc 参数数量
 * @param argv 参数列表
 * @return 测试退出码
 */
int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication app(argc, argv);
    PluginRegistryCapabilityTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "plugin_registry_capability_test.moc"
