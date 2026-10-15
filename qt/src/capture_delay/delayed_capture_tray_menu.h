#pragma once

#include <functional>

namespace markshot::ui {
class ContextMenu;
}

namespace markshot::capture_delay {

/**
 * 托盘「延时截图」子菜单需要的回调。
 */
struct DelayedCaptureMenuCallbacks {
    // 按毫秒数计划一次延时截图
    std::function<void(int delayMs)> schedule;
    // 取消尚未触发的延时截图
    std::function<void()> cancel;
    // 查询是否存在待执行的延时截图
    std::function<bool()> pending;
};

/**
 * 创建托盘「延时截图」子菜单，包含 3/5/10 秒预设与取消项。
 * @param callbacks 调度回调。
 * @param parent 父菜单，子菜单随其销毁并继承其主题。
 * @return 子菜单实例，由调用方通过 ContextMenu::addMenu 挂到托盘菜单上。
 */
ui::ContextMenu *createDelayedCaptureMenu(DelayedCaptureMenuCallbacks callbacks, ui::ContextMenu *parent);

}  // namespace markshot::capture_delay
