#pragma once

#include "pinned_window/pinned_resize_controller.h"

#include <QObject>

#include <functional>

class QWidget;

namespace markshot::shot {

/// @brief 由窗口管理器处理缩放锚点，同时保持钉图宽高比
class PinnedNativeResize final : public QObject {
public:
    /// @brief 准备可缩放约束并安装窗口尺寸监听
    /// @param window 需要原生缩放的窗口，同时负责销毁此对象
    /// @param onResize 应用最终尺寸后调用，用于同步图片绘制与缩放比例
    PinnedNativeResize(QWidget *window, std::function<void(QSize)> onResize);

    /// @brief 在真实鼠标按下事件中请求系统缩放
    /// @param direction 按下时命中的边缘或角落
    /// @return 窗口系统接受缩放请求时返回 true
    bool start(PinnedResizeDirection direction);

    /// @brief 判断当前手势是否交给窗口管理器处理
    /// @return 原生缩放尚未结束时返回 true
    bool isActive() const;

    /// @brief 结束当前缩放手势并清除方向
    /// @return 无返回值
    void finish();

protected:
    /// @brief 根据原生尺寸事件保持图片比例，避免递归调整
    /// @param object 事件所属窗口
    /// @param event 窗口事件
    /// @return 始终返回 false，允许窗口继续处理事件
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QWidget *m_window;
    std::function<void(QSize)> m_onResize;
    PinnedResizeDragState m_drag;
    bool m_adjusting = false;
};

}  // namespace markshot::shot
