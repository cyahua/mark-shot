#include "memory/idle_memory_trimmer.h"

#include "memory/process_memory_trim.h"

#include <QGuiApplication>
#include <QThreadPool>

#include <algorithm>
#include <utility>

namespace markshot::memory {

IdleMemoryTrimmer::IdleMemoryTrimmer(Options options, QObject *parent)
    : QObject(parent)
    , m_options(std::move(options))
{
    if (!m_options.trim) {
        m_options.trim = trimProcessMemoryWithLog;
    }
    m_timer.setSingleShot(true);
    // 【内存】【空闲回收】秒以下的延迟不能使用会舍入成零的 VeryCoarseTimer
    m_options.idleDelayMs = std::max(1, m_options.idleDelayMs);
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, &IdleMemoryTrimmer::evaluate);

    // 1. 托盘启动时没有前台窗口，按当前状态立即开始计时
    if (auto *application = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        connect(application,
                &QGuiApplication::applicationStateChanged,
                this,
                &IdleMemoryTrimmer::handleApplicationStateChanged);
        handleApplicationStateChanged(QGuiApplication::applicationState());
    }
}

void IdleMemoryTrimmer::handleApplicationStateChanged(Qt::ApplicationState state)
{
    // 1. 回到前台说明会话重新开始，取消计时并允许下次空闲再回收
    if (state == Qt::ApplicationActive) {
        m_timer.stop();
        m_trimmedSinceActive = false;
        return;
    }
    // 2. 非活动状态之间切换不重置计时，同一空闲期只回收一次
    if (m_trimmedSinceActive || m_timer.isActive()) {
        return;
    }
    m_timer.start(std::max(0, m_options.idleDelayMs));
}

bool IdleMemoryTrimmer::pending() const
{
    return m_timer.isActive();
}

void IdleMemoryTrimmer::evaluate()
{
    // 1. 录制等后台任务进行中时推迟，避免换页影响实时编码
    if (QThreadPool::globalInstance()->activeThreadCount() > 0
        || (m_options.busy && m_options.busy())) {
        m_timer.start(std::max(0, m_options.idleDelayMs));
        return;
    }
    m_trimmedSinceActive = true;
    m_options.trim();
}

}  // namespace markshot::memory
