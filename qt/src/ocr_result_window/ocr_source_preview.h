#pragma once

#include <QFrame>
#include <QImage>

namespace markshot::shot {

/// @brief 在独立原图页面按可用空间显示完整截图，保持原始比例
class OcrSourcePreview final : public QFrame {
public:
    /// @brief 创建保留原始图像质量的原图页面
    /// @param image 本次 OCR 实际使用的截图
    /// @param parent 所属窗口
    explicit OcrSourcePreview(QImage image, QWidget *parent = nullptr);

protected:
    /// @brief 按当前视图尺寸绘制原图，不与文本编辑区同时占位
    /// @param event 绘制事件
    /// @return 无返回值
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
};

}
