#pragma once

#ifdef MARK_SHOT_WITH_DBUS
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QMap>
#include <QTemporaryFile>
#include <QTimer>

namespace markshot::shot {

/// @brief 用一个常驻 KWin 脚本接收应用内各窗口的最新状态
class KdeKeepAboveBridge final : public QDBusVirtualObject {
public:
    /// @brief 注册状态接口和合成器恢复处理
    /// @param parent 当前图形应用
    explicit KdeKeepAboveBridge(QObject *parent);
    /// @brief 注销接口并卸载当前应用的脚本
    ~KdeKeepAboveBridge() override;
    /// @brief 更新一个窗口的状态，合并快速连续切换
    /// @param title 带编号的窗口标题
    /// @param alwaysOnTop 是否保持置顶
    /// @return 脚本通道是否已启动
    bool setWindowState(const QString &title, bool alwaysOnTop);
    /// @brief 删除已销毁窗口的状态
    /// @param title 带编号的窗口标题
    /// @return 无返回值
    void removeWindow(const QString &title);
    /// @brief 返回状态订阅接口的描述
    /// @param path 对象路径
    /// @return D-Bus 接口 XML
    QString introspect(const QString &path) const override;
    /// @brief 为 KWin 提供最新状态或等待下一次变更
    /// @param message KWin 的异步订阅请求
    /// @param connection 当前会话连接
    /// @return 是否处理该请求
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override;

private:
    /// @brief 加载并执行当前应用唯一的 KWin 脚本
    /// @return 加载与执行请求成功时返回 true
    bool startScript();
    /// @brief 停止订阅心跳并异步卸载应用脚本
    /// @return 无返回值
    void stopScript();
    /// @brief 回复最新状态，心跳避免空闲订阅超过 D-Bus 超时
    /// @return 无返回值
    void replyState();

    QMap<QString, bool> m_windows;
    qint64 m_revision = 0;
    QDBusMessage m_waitingCall;
    QTemporaryFile m_file;
    QTimer m_heartbeat;
    QString m_plugin;
    bool m_registered = false;
    bool m_started = false;
};

}  // namespace markshot::shot
#endif
