#pragma once

#include <QString>

namespace markshot::settings {

/// @brief 命令行按程序与参数拆分的结果
struct CommandLineParts {
    /// @brief 去除引号后的程序路径
    QString program;
    /// @brief 程序之后的原始文本（含前导空白），原样保留占位符与参数
    QString remainder;
};

/// @brief 拆分命令行首个程序片段，支持单引号与双引号包裹的路径
/// @param commandLine 完整命令行
/// @return 程序路径与剩余参数文本
CommandLineParts splitCommandProgram(const QString &commandLine);

/// @brief 为程序路径补充引号，仅在包含空白或 shell 特殊字符时添加
/// @param program 程序路径
/// @return 可直接写入命令行的程序片段
QString quoteCommandProgram(const QString &program);

/// @brief 只替换命令行中的程序路径，保留后续参数与占位符
/// @param commandLine 原命令行
/// @param program 新的程序路径
/// @return 替换后的命令行
QString replaceCommandProgram(const QString &commandLine, const QString &program);

/// @brief 将 ~ 前缀展开为主目录，供文件对话框定位起始目录
/// @param path 原始路径
/// @return 展开后的路径，非 ~ 前缀时原样返回
QString expandHomePath(const QString &path);

}  // namespace markshot::settings
