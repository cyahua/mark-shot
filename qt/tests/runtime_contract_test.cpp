#include "annotation_state_store.h"
#include "providers/provider_plugin_paths.h"
#include "window_detection.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

class RuntimeContractTest final : public QObject
{
    Q_OBJECT
  private slots:
    /** 验证显式配置目录不会回退至 HOME 下已有配置，返回值为空。 */
    void explicitConfigNeverFallsBackToHome()
    {
        QTemporaryDir isolated;
        QVERIFY(isolated.isValid());
        const QByteArray oldOverride = qgetenv("MARK_SHOT_CONFIG_DIR");
        const QByteArray oldConfigHome = qgetenv("XDG_CONFIG_HOME");
        const QString legacyRoot = isolated.filePath(QStringLiteral("legacy"));
        QVERIFY(QDir().mkpath(QDir(legacyRoot).filePath(QStringLiteral("mark-shot"))));
        QFile legacy(QDir(legacyRoot).filePath(QStringLiteral("mark-shot/config.json")));
        QVERIFY(legacy.open(QIODevice::WriteOnly));
        const QByteArray sentinel = "{\"legacy\":true}";
        QCOMPARE(legacy.write(sentinel), sentinel.size());
        legacy.close();
        const QString requested = isolated.filePath(QStringLiteral("explicit"));
        qputenv("MARK_SHOT_CONFIG_DIR", requested.toUtf8());
        qputenv("XDG_CONFIG_HOME", legacyRoot.toUtf8());
        const auto restore = qScopeGuard(
            [oldOverride, oldConfigHome]
            {
                qputenv("MARK_SHOT_CONFIG_DIR", oldOverride);
                qputenv("XDG_CONFIG_HOME", oldConfigHome);
            });
        QCOMPARE(markshot::markShotConfigDir(), requested);
        QVERIFY(markshot::ensureAppConfigFile());
        QCOMPARE(markshot::appConfigPath(), QDir(requested).filePath(QStringLiteral("config.json")));
        QVERIFY(markshot::saveAnnotationState(markshot::AnnotationState{}));
        QVERIFY(QFileInfo::exists(QDir(requested).filePath(QStringLiteral("annotation-state.json"))));
        QVERIFY(legacy.open(QIODevice::ReadOnly));
        QCOMPARE(legacy.readAll(), sentinel);
    }

    /** 验证开发构建的插件位置与实际运行时搜索目录一致，返回值为空。 */
    void pluginIsDiscoverableAndLoadable()
    {
        const QString plugin = QStringLiteral(MARK_SHOT_OPENAI_PLUGIN);
        QVERIFY(QFileInfo::exists(plugin));
        const QString directory = QFileInfo(plugin).absolutePath();
        QVERIFY2(markshot::providers::pluginSearchDirs().contains(directory), qPrintable(directory));
        QPluginLoader loader(plugin);
        QVERIFY2(loader.instance() != nullptr, qPrintable(loader.errorString()));
        QVERIFY(loader.unload());
    }
};

QTEST_GUILESS_MAIN(RuntimeContractTest)
#include "runtime_contract_test.moc"
