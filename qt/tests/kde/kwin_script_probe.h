#pragma once

#include <QDBusVirtualObject>
#include <QJsonArray>

/// @brief 在隔离会话中读取真实 KWin 窗口状态
class KWinScriptProbe final : public QDBusVirtualObject {
public:
    /// @brief 注册测试专用 D-Bus 接口
    KWinScriptProbe();
    /// @brief 返回探针接口描述
    /// @param path D-Bus 对象路径
    /// @return 接口 XML
    QString introspect(const QString &path) const override;
    /// @brief 接收 KWin 脚本上报的窗口状态
    /// @param message 脚本调用
    /// @param connection 当前隔离会话总线
    /// @return 是否处理该调用
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override;
    /// @brief 执行脚本并读取窗口堆叠与置顶状态
    /// @param action 在读取状态前执行的测试动作
    /// @return 按堆叠顺序排列的窗口信息
    QJsonArray snapshot(const QString &action = {});
    /// @brief 读取当前 KWin 注册的脚本对象数量
    /// @return 可寻址的脚本对象数
    int scriptCount() const;

private:
    bool m_received = false;
    int m_sequence = 0;
    QJsonArray m_windows;
};
