#pragma once

#include <QSizeGrip>
#include <QPointer>

#include <functional>

namespace markshot::ui {

/// @brief 为不支持系统尺寸调整的窗口提供应用内拖动入口
class WindowResizeGrip final : public QSizeGrip {
public:
    /// @brief 创建沿用当前主题的右下角尺寸控件
    /// @param parent 所属窗口或窗口内的容器
    /// @param clientResizeRequired 返回 true 时由应用处理尺寸变化，否则交给 Qt
    explicit WindowResizeGrip(QWidget *parent, std::function<bool()> clientResizeRequired);

protected:
    /// @brief 根据窗口协议选择 Qt 行为或开始应用内尺寸调整
    /// @param event 鼠标按下事件
    /// @return 无返回值
    void mousePressEvent(QMouseEvent *event) override;

    /// @brief 按指针位移调整窗口尺寸并遵循布局约束
    /// @param event 鼠标移动事件
    /// @return 无返回值
    void mouseMoveEvent(QMouseEvent *event) override;

    /// @brief 释放左键后终止应用内尺寸调整
    /// @param event 鼠标释放事件
    /// @return 无返回值
    void mouseReleaseEvent(QMouseEvent *event) override;

    /// @brief 隐藏控件时取消拖动并归还鼠标捕获
    /// @param event 控件隐藏事件
    /// @return 无返回值
    void hideEvent(QHideEvent *event) override;

private:
    /// @brief 清理当前尺寸调整状态，仅释放本控件持有的鼠标捕获
    /// @return 无返回值
    void stopResize();

    std::function<bool()> m_clientResizeRequired;
    QPointer<QWidget> m_resizeWindow;
    QPoint m_dragStartPosition;
    QSize m_dragStartSize;
};

}
