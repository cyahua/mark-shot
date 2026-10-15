#include "settings/settings_command_path.h"

#include <QDir>

namespace markshot::settings {

CommandLineParts splitCommandProgram(const QString &commandLine)
{
    CommandLineParts parts;
    // 1. 【设置】【命令解析】跳过前导空白，定位程序片段起点
    qsizetype index = 0;
    while (index < commandLine.size() && commandLine.at(index).isSpace()) {
        ++index;
    }
    // 2. 【设置】【命令解析】逐字符读取程序路径，引号内的空白属于路径
    QChar quote;
    for (; index < commandLine.size(); ++index) {
        const QChar ch = commandLine.at(index);
        if (!quote.isNull()) {
            if (ch == quote) {
                quote = QChar();
            } else {
                parts.program.append(ch);
            }
            continue;
        }
        if (ch == QLatin1Char('"') || ch == QLatin1Char('\'')) {
            quote = ch;
            continue;
        }
        if (ch.isSpace()) {
            break;
        }
        parts.program.append(ch);
    }
    // 3. 【设置】【命令解析】剩余文本原样保留，包括参数间的空白
    parts.remainder = commandLine.mid(index);
    return parts;
}

QString quoteCommandProgram(const QString &program)
{
#ifdef Q_OS_WIN
    // 反斜杠是 Windows 路径分隔符，不触发引号
    static const QString special = QStringLiteral(" \t'\"$`;&|<>()*?![]{}#");
#else
    static const QString special = QStringLiteral(" \t'\"$`\\;&|<>()*?![]{}#");
#endif
    bool needsQuote = program.isEmpty();
    for (const QChar ch : program) {
        if (special.contains(ch)) {
            needsQuote = true;
            break;
        }
    }
    if (!needsQuote) {
        return program;
    }
#ifdef Q_OS_WIN
    // 1. 【设置】【命令引号】Windows 命令解释器使用双引号
    return QLatin1Char('"') + program + QLatin1Char('"');
#else
    // 2. 【设置】【命令引号】POSIX shell 使用单引号，内部单引号转义为 '\''
    QString escaped = program;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
#endif
}

QString replaceCommandProgram(const QString &commandLine, const QString &program)
{
    const CommandLineParts parts = splitCommandProgram(commandLine);
    return quoteCommandProgram(program) + parts.remainder;
}

QString expandHomePath(const QString &path)
{
    if (path == QLatin1String("~")) {
        return QDir::homePath();
    }
    if (path.startsWith(QLatin1String("~/"))) {
        return QDir::homePath() + path.mid(1);
    }
    return path;
}

}  // namespace markshot::settings
