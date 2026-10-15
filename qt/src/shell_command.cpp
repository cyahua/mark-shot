#include "shell_command.h"

#include <QProcess>
#include <QProcessEnvironment>

namespace markshot {

QString commandShellProgram()
{
#if defined(Q_OS_WIN)
    const QString comspec = QProcessEnvironment::systemEnvironment().value(QStringLiteral("COMSPEC"));
    return comspec.isEmpty() ? QStringLiteral("cmd.exe") : comspec;
#else
    QString shell = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SHELL"),
                                                                   QStringLiteral("/bin/sh"));
    return shell.isEmpty() ? QStringLiteral("/bin/sh") : shell;
#endif
}

QStringList commandShellArguments(const QString &commandLine)
{
#if defined(Q_OS_WIN)
    return {QStringLiteral("/D"), QStringLiteral("/V:OFF"), QStringLiteral("/S"), QStringLiteral("/C"), commandLine};
#else
    return {QStringLiteral("-c"), commandLine};
#endif
}

void setShellCommand(QProcess *process, const QString &commandLine)
{
    if (!process) {
        return;
    }
    process->setProgram(commandShellProgram());
#if defined(Q_OS_WIN)
    // 1. 【命令执行】【Windows 引号】cmd.exe 不接受普通 argv 转义，保留用户命令中的引号
    process->setArguments({});
    process->setNativeArguments(QStringLiteral("/D /V:OFF /S /C \"") + commandLine + QLatin1Char('"'));
#else
    process->setArguments(commandShellArguments(commandLine));
#endif
}

}  // namespace markshot
