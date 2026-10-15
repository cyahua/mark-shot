#include "pinned_window/pinned_native_resize.h"

#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QWidget>
#include <QWindow>

#include <utility>

namespace markshot::shot {
namespace {

constexpr int kMinimumExtent = 24;

/// @brief 将钉图边界方向转换成系统缩放边界
/// @param direction 鼠标命中的方向
/// @return Qt 原生缩放边界组合
Qt::Edges nativeEdges(PinnedResizeDirection direction)
{
    Qt::Edges edges;
    if (pinnedResizeDirectionIncludesLeft(direction)) {
        edges |= Qt::LeftEdge;
    }
    if (pinnedResizeDirectionIncludesRight(direction)) {
        edges |= Qt::RightEdge;
    }
    if (pinnedResizeDirectionIncludesTop(direction)) {
        edges |= Qt::TopEdge;
    }
    if (pinnedResizeDirectionIncludesBottom(direction)) {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

/// @brief 按系统请求尺寸还原拖拽位移，复用钉图比例和最小尺寸约束
/// @param drag 开始拖拽时的尺寸和方向
/// @param requested 窗口管理器请求的尺寸
/// @return 保持原始比例的最终尺寸
QSize constrainedSize(const PinnedResizeDragState &drag, QSize requested)
{
    QPoint delta(requested.width() - drag.startGeometry.width(),
                 requested.height() - drag.startGeometry.height());
    if (pinnedResizeDirectionIncludesLeft(drag.direction)) {
        delta.setX(-delta.x());
    }
    if (pinnedResizeDirectionIncludesTop(drag.direction)) {
        delta.setY(-delta.y());
    }
    return pinnedResizeGeometry(drag, drag.startGlobalPosition + delta,
                                QSize(kMinimumExtent, kMinimumExtent)).size();
}

}  // namespace

PinnedNativeResize::PinnedNativeResize(QWidget *window, std::function<void(QSize)> onResize)
    : QObject(window), m_window(window), m_onResize(std::move(onResize))
{
    // 1. 【钉图】【原生缩放】在首次显示前提交可缩放约束，避免 KWin 拒绝首个手势
    m_window->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    m_window->setMinimumSize(m_window->size().boundedTo(QSize(kMinimumExtent, kMinimumExtent)));
    m_window->installEventFilter(this);
}

bool PinnedNativeResize::start(PinnedResizeDirection direction)
{
    QWindow *window = m_window->windowHandle();
    const Qt::Edges edges = nativeEdges(direction);
    if (!window || !edges) {
        return false;
    }

    // 1. 【钉图】【原生缩放】使用合成器维护相对边缘锚点
    m_drag = beginPinnedResizeDrag(direction, QRect(QPoint(), m_window->size()), QPoint());
    if (!window->startSystemResize(edges)) {
        finish();
        return false;
    }
    return true;
}

bool PinnedNativeResize::isActive() const
{
    return isPinnedResizeDirection(m_drag.direction);
}

void PinnedNativeResize::finish()
{
    m_drag = {};
}

bool PinnedNativeResize::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_window || event->type() != QEvent::Resize || !isActive() || m_adjusting) {
        return false;
    }

    // 1. 【钉图】【原生缩放】窗口位置交给合成器，只约束客户端提交的图片尺寸
    const QSize target = constrainedSize(m_drag, static_cast<QResizeEvent *>(event)->size());
    const QScopedValueRollback<bool> adjusting(m_adjusting, true);
    if (m_window->size() != target) {
        m_window->resize(target);
    }
    if (m_onResize) {
        m_onResize(m_window->size());
    }
    return false;
}

}  // namespace markshot::shot
