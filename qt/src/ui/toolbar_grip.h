#pragma once

#include <QWidget>

namespace markshot::ui {

/// @brief 【截图工具栏】【拖拽把手】绘制点阵握柄；拖拽定位由所属窗口处理
class ToolbarGrip final : public QWidget {
public:
    /// @brief 创建握柄
    /// @param parent 所属工具栏，负责控件生命周期
    explicit ToolbarGrip(QWidget *parent = nullptr);

    /// @brief 设置工具栏排列方向，并更新握柄尺寸
    /// @param orientation 工具栏横向或纵向排列
    /// @return 无返回值
    void setOrientation(Qt::Orientation orientation);

protected:
    /// @brief 根据悬停状态绘制点阵
    /// @param event 绘制事件
    /// @return 无返回值
    void paintEvent(QPaintEvent *event) override;
    /// @brief 在悬停或启用状态变化后刷新点阵
    /// @param event 控件事件
    /// @return 事件是否已处理
    bool event(QEvent *event) override;

private:
    Qt::Orientation m_orientation = Qt::Horizontal;
};

}
