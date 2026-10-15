#include "ui/window_resize_grip.h"

#include <QHideEvent>
#include <QLayout>
#include <QMouseEvent>

#include <utility>

namespace markshot::ui {

WindowResizeGrip::WindowResizeGrip(QWidget *parent, std::function<bool()> clientResizeRequired)
    : QSizeGrip(parent), m_clientResizeRequired(std::move(clientResizeRequired))
{
}

void WindowResizeGrip::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_clientResizeRequired
        || !m_clientResizeRequired()) {
        QSizeGrip::mousePressEvent(event);
        return;
    }

    // 1. 【窗口】【尺寸调整】显式捕获浮层拖动，避免指针离开控件后丢失移动事件
    m_resizeWindow = window();
    m_dragStartPosition = event->globalPosition().toPoint();
    m_dragStartSize = m_resizeWindow->size();
    grabMouse();
    event->accept();
}

void WindowResizeGrip::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_resizeWindow) {
        QSizeGrip::mouseMoveEvent(event);
        return;
    }
    if (!(event->buttons() & Qt::LeftButton)) {
        stopResize();
        event->accept();
        return;
    }

    // 1. 【窗口】【尺寸调整】使用按下时的尺寸计算位移，交给布局限制最小和最大尺寸
    const QPoint delta = event->globalPosition().toPoint() - m_dragStartPosition;
    const QSize requested = m_dragStartSize + QSize(delta.x(), delta.y());
    m_resizeWindow->resize(QLayout::closestAcceptableSize(m_resizeWindow, requested));
    event->accept();
}

void WindowResizeGrip::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_resizeWindow && event->button() == Qt::LeftButton) {
        stopResize();
        event->accept();
        return;
    }
    QSizeGrip::mouseReleaseEvent(event);
}

void WindowResizeGrip::hideEvent(QHideEvent *event)
{
    stopResize();
    QSizeGrip::hideEvent(event);
}

void WindowResizeGrip::stopResize()
{
    m_resizeWindow.clear();
    if (QWidget::mouseGrabber() == this) {
        releaseMouse();
    }
}

}
