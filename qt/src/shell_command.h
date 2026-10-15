#pragma once

#include <QString>
#include <QStringList>

class QProcess;

namespace markshot {

QString commandShellProgram();
QStringList commandShellArguments(const QString &commandLine);
/// @brief 按平台命令解释器的引号规则配置进程
/// @param process 待配置的进程，空指针时不执行操作
/// @param commandLine 保留命令解释器语法的完整命令行
/// @return 无返回值
void setShellCommand(QProcess *process, const QString &commandLine);

}  // namespace markshot
