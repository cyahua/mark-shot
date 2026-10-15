#pragma once

#include <QObject>
#include <QTimer>

#include <functional>

namespace markshot::memory {

/**
 * 【内存】【空闲回收】应用失去前台一段时间后回收进程内存。
 *
 * 截图窗口关闭、应用退回托盘后，分配器空闲页和已映射的依赖库页面仍计入
 * 常驻内存；本类在应用持续非活动后执行一次回收，回到前台后重新计时。
 */
class IdleMemoryTrimmer final : public QObject {
    Q_OBJECT

public:
    struct Options {
        /// 非活动持续多久后回收，单位毫秒
        int idleDelayMs = 30000;
        /// 返回 true 时推迟回收，例如正在录制
        std::function<bool()> busy;
        /// 实际回收动作，为空时使用 trimProcessMemoryWithLog
        std::function<void()> trim;
    };

    /**
     * 创建空闲回收器并跟随应用前后台状态计时。
     * @param options 回收配置。
     * @param parent 父对象。
     */
    explicit IdleMemoryTrimmer(Options options, QObject *parent = nullptr);

    /**
     * 处理应用前后台状态变化。
     * @param state 新的应用状态。
     * @return 无返回值。
     */
    void handleApplicationStateChanged(Qt::ApplicationState state);

    /**
     * 是否正在等待回收。
     * @return 计时进行中时返回 true。
     */
    bool pending() const;

private:
    /**
     * 计时到期后检查忙碌状态并执行回收。
     * @return 无返回值。
     */
    void evaluate();

    Options m_options;
    QTimer m_timer;
    bool m_trimmedSinceActive = false;
};

}  // namespace markshot::memory
