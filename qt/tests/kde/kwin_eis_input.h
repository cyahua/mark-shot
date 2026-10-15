#pragma once

#include <QPointF>

struct ei;
struct ei_device;

/// @brief 只向本轮隔离 KWin 会话发送真实鼠标事件
class KWinEisInput final {
public:
    /// @brief 连接隔离会话的 EIS 接口并等待鼠标设备就绪
    KWinEisInput();
    /// @brief 断开鼠标设备和 EIS 连接
    ~KWinEisInput();
    KWinEisInput(const KWinEisInput &) = delete;
    KWinEisInput &operator=(const KWinEisInput &) = delete;

    /// @brief 执行按下、移动、松开的完整左键拖拽
    /// @param from 起始输出逻辑坐标
    /// @param to 结束输出逻辑坐标
    /// @return 无返回值
    void drag(QPointF from, QPointF to);

private:
    /// @brief 处理设备握手和 Qt 事件
    /// @param milliseconds 最短处理时长，单位为毫秒
    /// @return 无返回值
    void pump(int milliseconds);
    /// @brief 移动鼠标并等待窗口收到事件
    /// @param position 输出逻辑坐标
    /// @return 无返回值
    void move(QPointF position);
    /// @brief 更新左键状态并等待窗口处理
    /// @param down 是否按下左键
    /// @return 无返回值
    void left(bool down);
    /// @brief 释放已经取得的设备和连接
    /// @return 无返回值
    void release();

    ei *m_context = nullptr;
    ei_device *m_pointer = nullptr;
    ei_device *m_buttons = nullptr;
};
