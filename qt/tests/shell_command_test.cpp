#include "shell_command.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

namespace {

/// @brief 为测试中受控的文件路径添加当前命令解释器要求的引号
/// @param path 测试文件路径
/// @return 带引号的命令行参数
QString quotedPath(const QString &path)
{
#ifdef Q_OS_WIN
    return QLatin1Char('"') + QDir::toNativeSeparators(path) + QLatin1Char('"');
#else
    QString escaped = path;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\"'\"'"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
#endif
}

}

class ShellCommandTest final : public QObject {
    Q_OBJECT
private slots:
    /// @brief 验证命令解释器能启动带空格路径的程序，并保留被引用的特殊字符，无参数和返回值
    void runsQuotedProgramAndImagePath()
    {
        QTemporaryDir isolated;
        QVERIFY(isolated.isValid());
#ifdef Q_OS_WIN
        const QString program = isolated.filePath(QStringLiteral("OCR fixture.exe"));
#else
        const QString program = isolated.filePath(QStringLiteral("OCR fixture"));
#endif
        QVERIFY(QFile::copy(QString::fromUtf8(MARK_SHOT_TEST_OCR_FIXTURE_PATH), program));
        QFile input(isolated.filePath(QStringLiteral("image input & sample.png")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        input.close();
        const QString command = quotedPath(program) + QStringLiteral(" --ocr-fixture ")
            + quotedPath(input.fileName());
        QProcess process;
        markshot::setShellCommand(&process, command);
        process.start();
        QVERIFY2(process.waitForFinished(10000), qPrintable(process.errorString()));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        const QByteArray errors = process.readAllStandardError();
        QVERIFY2(process.exitCode() == 0, errors.constData());
        const auto tokens = QJsonDocument::fromJson(process.readAllStandardOutput())
                                .object().value(QStringLiteral("tokens")).toArray();
        QCOMPARE(tokens.size(), 2);
        QCOMPARE(tokens.first().toObject().value(QStringLiteral("text")).toString(), QStringLiteral("Hello"));

        // 1. 【命令测试】【重复使用】同一进程对象改用新命令时不应保留旧参数
        markshot::setShellCommand(&process, quotedPath(program) + QStringLiteral(" --ocr-empty ")
                                               + quotedPath(input.fileName()));
        process.start();
        QVERIFY2(process.waitForFinished(10000), qPrintable(process.errorString()));
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(process.readAllStandardOutput(), QByteArray("[]"));
    }
};

QTEST_GUILESS_MAIN(ShellCommandTest)
#include "shell_command_test.moc"
