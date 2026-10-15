#include "marketplace/plugin_installer.h"
#include "marketplace/plugin_updates.h"
#include "providers/provider_plugin_paths.h"

#include <QCryptographicHash>
#include <QFile>
#include <QLibrary>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class PluginInstallerTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 【插件测试】【占用更新】运行中的库保持可用，下载的新库留待重启替换
     * @return 无返回值
     */
    void stagesUpdateWhileLibraryIsLoaded()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString destination = dir.filePath(pluginFileName());
        QVERIFY(QFile::copy(QString::fromUtf8(MARK_SHOT_TEST_OLD_PLUGIN), destination));
        QLibrary loaded(destination);
        auto revision = reinterpret_cast<int (*)()>(loaded.resolve("pluginRevision"));
        QVERIFY2(revision, qPrintable(loaded.errorString()));
        QCOMPARE(revision(), 1);
        QFile oldFile(destination);
        QVERIFY(oldFile.open(QIODevice::ReadOnly));
        const QByteArray oldBytes = oldFile.readAll();
        oldFile.close();

        const auto result = markshot::marketplace::installPluginAsset(
            {QString::fromUtf8(MARK_SHOT_TEST_NEW_PLUGIN), pluginFileName(), dir.path(), {}});
        QVERIFY2(result.success, qPrintable(result.error));
        QVERIFY(result.pendingRestart);
        QFile current(destination);
        QVERIFY(current.open(QIODevice::ReadOnly));
        QCOMPARE(current.readAll(), oldBytes);
        current.close();
        QCOMPARE(revision(), 1);
        QVERIFY(loaded.unload());
        // 1. 【插件测试】【重启生效】解除占用后应用更新，并从原路径加载新版本
        const auto errors = markshot::marketplace::applyPendingPluginUpdates(dir.path());
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join(QLatin1Char('\n'))));
        QLibrary updated(destination);
        auto updatedRevision = reinterpret_cast<int (*)()>(updated.resolve("pluginRevision"));
        QVERIFY2(updatedRevision, qPrintable(updated.errorString()));
        QCOMPARE(updatedRevision(), 2);
        QVERIFY(updated.unload());
        QVERIFY(markshot::marketplace::applyPendingPluginUpdates(dir.path()).isEmpty());
    }

    /**
     * 【插件测试】【锁定重试】其他进程占用目标时保留旧文件及更新包
     * @return 无返回值
     */
    void retainsUpdateWhenDestinationIsLocked()
    {
#ifndef Q_OS_WIN
        QSKIP("Windows file sharing semantics are required");
#else
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString destination = dir.filePath(pluginFileName());
        QVERIFY(QFile::copy(QString::fromUtf8(MARK_SHOT_TEST_OLD_PLUGIN), destination));
        const auto installed = markshot::marketplace::installPluginAsset(
            {QString::fromUtf8(MARK_SHOT_TEST_NEW_PLUGIN), pluginFileName(), dir.path(), {}});
        QVERIFY(installed.success && installed.pendingRestart);
        // 1. 【插件测试】【锁定重试】模拟另一个进程持有不允许替换的 DLL 句柄
        const HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(destination.utf16()),
            GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        auto close = qScopeGuard([handle] { CloseHandle(handle); });
        QVERIFY(!markshot::marketplace::applyPendingPluginUpdates(dir.path()).isEmpty());
        QFile current(destination);
        QFile original(QString::fromUtf8(MARK_SHOT_TEST_OLD_PLUGIN));
        QVERIFY(current.open(QIODevice::ReadOnly));
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(current.readAll(), original.readAll());
        current.close();
        original.close();
        CloseHandle(handle);
        close.dismiss();
        QVERIFY(markshot::marketplace::applyPendingPluginUpdates(dir.path()).isEmpty());
        QLibrary updated(destination);
        auto revision = reinterpret_cast<int (*)()>(updated.resolve("pluginRevision"));
        QVERIFY2(revision, qPrintable(updated.errorString()));
        QCOMPARE(revision(), 2);
        QVERIFY(updated.unload());
#endif
    }

    /// @brief 校验失败不能覆盖先前已下载的更新，无参数和返回值
    void rejectedDownloadPreservesPendingUpdate()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString destination = dir.filePath(pluginFileName());
        QVERIFY(QFile::copy(QString::fromUtf8(MARK_SHOT_TEST_OLD_PLUGIN), destination));
        const auto staged = markshot::marketplace::installPluginAsset(
            {QString::fromUtf8(MARK_SHOT_TEST_NEW_PLUGIN), pluginFileName(), dir.path(), {}});
        QVERIFY(staged.success && staged.pendingRestart);
        const auto rejected = markshot::marketplace::installPluginAsset(
            {QString::fromUtf8(MARK_SHOT_TEST_OLD_PLUGIN), pluginFileName(), dir.path(), QString(64, QLatin1Char('0'))});
        QVERIFY(!rejected.success);
        QVERIFY(markshot::marketplace::applyPendingPluginUpdates(dir.path()).isEmpty());
        QLibrary updated(destination);
        auto revision = reinterpret_cast<int (*)()>(updated.resolve("pluginRevision"));
        QVERIFY2(revision, qPrintable(updated.errorString()));
        QCOMPARE(revision(), 2);
        QVERIFY(updated.unload());
    }

    /// @brief 市场更新的用户插件优先于程序附带插件，无参数和返回值
    void prefersUserPluginDirectory()
    {
        QCOMPARE(markshot::providers::pluginSearchDirs().first(), markshot::providers::userPluginDirectory());
    }

    /**
     * 验证动态库资产可以安装到指定插件目录。
     * @return 无返回值。
     */
    void installsLibraryAsset()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString sourcePath = dir.filePath(pluginFileName());
        const QByteArray content = QByteArrayLiteral("plugin-binary");
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(content), qint64(content.size()));
        source.close();

        const QString sha256 = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
        const markshot::marketplace::PluginInstallResult result =
            markshot::marketplace::installPluginAsset({sourcePath,
                                                       pluginFileName(),
                                                       dir.filePath(QStringLiteral("plugins")),
                                                       sha256});

        QVERIFY2(result.success, qPrintable(result.error));
        QFile installed(result.installedPath);
        QVERIFY(installed.open(QIODevice::ReadOnly));
        QCOMPARE(installed.readAll(), content);
    }

    /**
     * 验证安装器拒绝压缩包资产。
     * @return 无返回值。
     */
    void rejectsArchiveAsset()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString sourcePath = dir.filePath(QStringLiteral("plugin.zip"));
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write(QByteArrayLiteral("zip"));
        source.close();

        const markshot::marketplace::PluginInstallResult result =
            markshot::marketplace::installPluginAsset({sourcePath,
                                                       QStringLiteral("plugin.zip"),
                                                       dir.filePath(QStringLiteral("plugins")),
                                                       QString()});

        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("dynamic library")));
    }

    /**
     * 验证安装器拒绝带路径分隔符的目标文件名。
     * @return 无返回值。
     */
    void rejectsPathTraversal()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString sourcePath = dir.filePath(pluginFileName());
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write(QByteArrayLiteral("plugin-binary"));
        source.close();

        const markshot::marketplace::PluginInstallResult result =
            markshot::marketplace::installPluginAsset({sourcePath,
                                                       QStringLiteral("../") + pluginFileName(),
                                                       dir.filePath(QStringLiteral("plugins")),
                                                       QString()});

        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("path separators")));
    }

private:
    /**
     * 读取当前平台测试用插件文件名。
     * @return 插件文件名。
     */
    QString pluginFileName() const
    {
#ifdef Q_OS_WIN
        return QStringLiteral("mark-shot-sample.dll");
#elif defined(Q_OS_MACOS)
        return QStringLiteral("libmark-shot-sample.dylib");
#else
        return QStringLiteral("libmark-shot-sample.so");
#endif
    }
};

QTEST_GUILESS_MAIN(PluginInstallerTest)

#include "plugin_installer_test.moc"
