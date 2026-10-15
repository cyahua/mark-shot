#include "recording/recording_config_dialog.h"

#include <QGuiApplication>
#include <QLayout>
#include <QScreen>
#include <QScrollArea>
#include <QShowEvent>
#include <QTimer>

#include <algorithm>

namespace markshot::recording {

void RecordingConfigDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    scheduleContentResize();
}

void RecordingConfigDialog::scheduleContentResize(QWidget *reveal)
{
    if (reveal) {
        m_pendingReveal = reveal;
    }
    if (m_contentResizePending) {
        return;
    }
    // 1. 【录制】【选项布局】等待本次显隐和表单换行完成，避免按旧高度调整窗口
    m_contentResizePending = true;
    QTimer::singleShot(0, this, [this] {
        m_contentResizePending = false;
        fitExpandedContent();
    });
}

void RecordingConfigDialog::fitExpandedContent()
{
    if (!isVisible() || !m_contentScroll || !m_contentScroll->widget() || !screen()) {
        return;
    }

    // 1. 【录制】【选项布局】测量内容与固定操作栏，按屏幕可用高度限制自动增高
    QWidget *content = m_contentScroll->widget();
    content->layout()->activate();
    layout()->activate();
    const QRect available = screen()->availableGeometry();
    const int frameHeight = std::max(0, frameGeometry().height() - height());
    const int maximumHeight = std::max(minimumHeight(), available.height() - frameHeight - 24);
    const int chromeHeight = height() - m_contentScroll->viewport()->height();
    const int targetHeight = std::clamp(content->sizeHint().height() + chromeHeight,
                                      minimumHeight(), maximumHeight);
    resize(width(), targetHeight);

    // 2. 【录制】【窗口位置】支持全局坐标的平台将窗口限制在当前屏幕内
    if (!QGuiApplication::platformName().contains(QStringLiteral("wayland"))) {
        const QRect frame = frameGeometry();
        const int top = std::clamp(frame.top(), available.top(),
                                  std::max(available.top(), available.bottom() - frame.height() + 1));
        move(pos() + QPoint(0, top - frame.top()));
    }

    // 3. 【录制】【展开定位】屏幕不足时仍保留操作栏，并将新出现的选项顶部滚动到视区
    QWidget *reveal = m_pendingReveal;
    m_pendingReveal = nullptr;
    if (reveal && reveal->isVisible()) {
        QTimer::singleShot(0, this, [this, reveal] {
            const QPoint point = reveal->mapTo(m_contentScroll->widget(), QPoint());
            m_contentScroll->ensureVisible(point.x(), point.y(), 0, 12);
        });
    }
}

}
