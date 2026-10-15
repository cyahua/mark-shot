#pragma once

#include <QFrame>
#include <QImage>
#include <QPointF>

class QContextMenuEvent;
class QMouseEvent;
class QWheelEvent;

namespace markshot::shot {

/// @brief 在独立原图页面按可用空间显示完整截图，并支持简单缩放、拖动和右键操作
class OcrSourcePreview final : public QFrame {
public:
    /// @brief 创建保留原始图像质量的原图页面
    /// @param image 本次 OCR 实际使用的截图
    /// @param parent 所属窗口
    explicit OcrSourcePreview(QImage image, QWidget *parent = nullptr);

protected:
    /// @brief 按当前缩放和偏移绘制原图
    /// @param event 绘制事件
    /// @return 无返回值
    void paintEvent(QPaintEvent *event) override;

    /// @brief 滚轮以指针为中心缩放，最小回到适配视区
    /// @param event 滚轮事件
    /// @return 无返回值
    void wheelEvent(QWheelEvent *event) override;

    /// @brief 左键开始拖动已放大的图片
    /// @param event 鼠标按下事件
    /// @return 无返回值
    void mousePressEvent(QMouseEvent *event) override;

    /// @brief 拖动图片并限制它不完全离开视区
    /// @param event 鼠标移动事件
    /// @return 无返回值
    void mouseMoveEvent(QMouseEvent *event) override;

    /// @brief 结束拖动
    /// @param event 鼠标释放事件
    /// @return 无返回值
    void mouseReleaseEvent(QMouseEvent *event) override;

    /// @brief 双击恢复适配视区的大小和位置
    /// @param event 双击事件
    /// @return 无返回值
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    /// @brief 提供编辑和置顶图片两个入口
    /// @param event 右键菜单事件
    /// @return 无返回值
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    /// @brief 按当前缩放计算绘制尺寸，不含平移
    /// @return 绘制尺寸
    QSize fittedSize() const;

    /// @brief 返回当前图片在视区中的矩形
    /// @return 绘制矩形
    QRect imageRect() const;

    /// @brief 限制平移，使图片至少有一段留在视区内
    /// @return 无返回值
    void clampPan();

    QImage m_image;
    qreal m_zoom = 1.0;
    QPointF m_pan;
    bool m_dragging = false;
    QPointF m_dragAnchor;
    QPointF m_panAnchor;
};

}
