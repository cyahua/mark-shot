#pragma once

#include <QRect>
#include <QPointer>
#include <QWidget>

#include <functional>

class QPainter;

namespace markshot::shot {

/// @brief 在保持原窗口鼠标抓取时显示不接收输入的 layer-shell 拖动预览
class PinnedLayerShellDragPreview final : public QWidget {
public:
    using PaintImage = std::function<void(QPainter &, QRectF)>;

    /// @brief 创建由 owner 管理的预览窗口，通过 paintImage 复用贴图绘制
    /// @param owner 接收鼠标输入的原始贴图窗口
    /// @param paintImage 根据当前完整图片与可见区域绘制内容的回调
    PinnedLayerShellDragPreview(QWidget *owner, PaintImage paintImage);

    /// @brief 更新预览的全局逻辑几何，并在必要时切换所属屏幕
    /// @param geometry 包含屏幕外部分的完整图片几何
    /// @return layer-shell 配置成功时返回 true
    bool setLogicalGeometry(QRect geometry);

    /// @brief 判断预览是否已开始绘制，可由原窗口据此隐藏重复图像
    /// @return 首次绘制后返回 true
    bool isReady() const;

protected:
    /// @brief 使用原窗口的绘制回调显示图片、边框及文字覆盖层
    /// @param event Qt 绘制事件
    /// @return 无返回值
    void paintEvent(QPaintEvent *event) override;

private:
    QPointer<QWidget> m_owner;
    PaintImage m_paintImage;
    bool m_configured = false;
    bool m_ready = false;
};

}  // namespace markshot::shot
