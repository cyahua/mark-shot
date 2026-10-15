#include "marketplace/plugin_installer.h"
#include "markshot/ocr_provider_plugin.h"
#include "providers/provider_plugin_paths.h"
#include "providers/provider_plugin_registry.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include <algorithm>

class PluginRegistryUpdateTest final : public QObject {
    Q_OBJECT
private slots:
    /// @brief 注册器首次加载前应用待更新库，模型补齐后刷新诊断，无参数和返回值
    void appliesUpdateBeforeLoading()
    {
        QTemporaryDir isolated;
        QVERIFY(isolated.isValid());
        qputenv("MARK_SHOT_TEST_MODEL_MARKER", isolated.filePath(QStringLiteral("model-ready")).toUtf8());
        const QString directory = markshot::providers::userPluginDirectory();
        const QString library = QFileInfo(QString::fromUtf8(MARK_SHOT_TEST_OLD_PLUGIN)).fileName();
        const auto installed = markshot::marketplace::installPluginAsset(
            {QString::fromUtf8(MARK_SHOT_TEST_OLD_PLUGIN), library, directory, {}});
        QVERIFY2(installed.success, qPrintable(installed.error));
        const auto updated = markshot::marketplace::installPluginAsset(
            {QString::fromUtf8(MARK_SHOT_TEST_NEW_PLUGIN), library, directory, {}});
        QVERIFY(updated.success && updated.pendingRestart);
        // 1. 【插件测试】【启动加载】直接进入生产注册器，由注册器应用待更新文件
        auto &registry = markshot::providers::ProviderPluginRegistry::instance();
        const auto plugins = registry.ocrProviders();
        auto it = std::find_if(plugins.cbegin(), plugins.cend(), [](const auto *plugin) {
            return plugin->providerId() == QStringLiteral("update-fixture");
        });
        QVERIFY(it != plugins.cend());
        QCOMPARE((*it)->displayName(), QStringLiteral("2"));
        const auto before = registry.pluginInfos();
        const auto missing = std::find_if(before.cbegin(), before.cend(), [](const auto &info) {
            return info.providerId == QStringLiteral("update-fixture");
        });
        QVERIFY(missing != before.cend());
        QVERIFY(!missing->available);
        // 2. 【插件测试】【模型刷新】模拟模型下载完成，刷新后错误消失且插件可用
        QFile marker(qEnvironmentVariable("MARK_SHOT_TEST_MODEL_MARKER"));
        QVERIFY(marker.open(QIODevice::WriteOnly));
        marker.close();
        const auto after = registry.pluginInfos();
        const auto available = std::find_if(after.cbegin(), after.cend(), [](const auto &info) {
            return info.providerId == QStringLiteral("update-fixture");
        });
        QVERIFY(available != after.cend());
        QVERIFY(available->available);
        QVERIFY(available->error.isEmpty());
    }
};

/**
 * 【插件测试】【进程隔离】在子进程退出释放 DLL 后清理临时安装目录
 * @param argc 参数数量
 * @param argv 参数列表
 * @return 测试退出码
 */
int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication app(argc, argv);
    QString name = qEnvironmentVariable("MARK_SHOT_TEST_REGISTRY_NAME");
    const bool child = !name.isEmpty();
    if (!child) name = QStringLiteral("mark-shot-registry-") + QUuid::createUuid().toString(QUuid::Id128);
    app.setApplicationName(name);
    QTemporaryDir isolated;
    if (!isolated.isValid()) return 1;
    if (child) {
        PluginRegistryUpdateTest test;
        return QTest::qExec(&test, argc, argv);
    }
    qputenv("XDG_DATA_HOME", isolated.path().toUtf8());
    const QString directory = markshot::providers::userPluginDirectory();
    if (QDir(directory).exists()) return 1;
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("MARK_SHOT_TEST_REGISTRY_NAME"), name);
    process.setProcessEnvironment(environment);
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(app.applicationFilePath(), {QStringLiteral("-o"), QStringLiteral("-,txt")});
    if (!process.waitForFinished(20000)) return 1;
    const int result = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : 1;
    QDir(QFileInfo(directory).absolutePath()).removeRecursively();
    return result;
}

#include "plugin_registry_update_test.moc"
