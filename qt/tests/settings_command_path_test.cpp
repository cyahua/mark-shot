#include "settings/settings_command_path.h"

#include <QDir>
#include <QtTest/QtTest>

using namespace markshot::settings;

/// @brief 检查命令字段浏览时只替换程序路径、保留参数与占位符
class SettingsCommandPathTest final : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证普通与带引号的程序片段拆分
    /// @return 无返回值
    void splitsProgramAndRemainder()
    {
        // 1. 无引号命令按首个空白拆分，剩余文本保留前导空白
        CommandLineParts plain = splitCommandProgram(QStringLiteral("mark-shot-ocr {image} --lang zh"));
        QCOMPARE(plain.program, QStringLiteral("mark-shot-ocr"));
        QCOMPARE(plain.remainder, QStringLiteral(" {image} --lang zh"));
        // 2. 引号内的空白属于程序路径
        CommandLineParts quoted = splitCommandProgram(QStringLiteral("'/opt/my tools/ocr' {image}"));
        QCOMPARE(quoted.program, QStringLiteral("/opt/my tools/ocr"));
        QCOMPARE(quoted.remainder, QStringLiteral(" {image}"));
        CommandLineParts doubleQuoted = splitCommandProgram(QStringLiteral("  \"/a b/c\""));
        QCOMPARE(doubleQuoted.program, QStringLiteral("/a b/c"));
        QVERIFY(doubleQuoted.remainder.isEmpty());
    }

    /// @brief 验证替换程序后参数与占位符原样保留
    /// @return 无返回值
    void replacesOnlyProgram()
    {
        QCOMPARE(replaceCommandProgram(QStringLiteral("mark-shot-upload {image} -H \"Auth: $TOKEN\""),
                                       QStringLiteral("/usr/local/bin/up")),
                 QStringLiteral("/usr/local/bin/up {image} -H \"Auth: $TOKEN\""));
        QCOMPARE(replaceCommandProgram(QString(), QStringLiteral("/usr/bin/tool")), QStringLiteral("/usr/bin/tool"));
#ifndef Q_OS_WIN
        // 1. 含空白或单引号的路径使用 POSIX 单引号转义
        QCOMPARE(replaceCommandProgram(QStringLiteral("old {input}"), QStringLiteral("/opt/my tools/tr")),
                 QStringLiteral("'/opt/my tools/tr' {input}"));
        QCOMPARE(quoteCommandProgram(QStringLiteral("/tmp/it's")), QStringLiteral("'/tmp/it'\\''s'"));
#endif
    }

    /// @brief 验证 ~ 前缀展开只用于对话框起点
    /// @return 无返回值
    void expandsHomePrefix()
    {
        QCOMPARE(expandHomePath(QStringLiteral("~")), QDir::homePath());
        QCOMPARE(expandHomePath(QStringLiteral("~/logs/a.log")), QDir::homePath() + QStringLiteral("/logs/a.log"));
        QCOMPARE(expandHomePath(QStringLiteral("/var/~x")), QStringLiteral("/var/~x"));
    }
};

QTEST_GUILESS_MAIN(SettingsCommandPathTest)

#include "settings_command_path_test.moc"
