#include "pinned_window/pinned_kde_keep_above.h"

#include "pinned_window/pinned_kde_keep_above_script.h"

#include <QGuiApplication>
#include <QPointer>
#include <QProcessEnvironment>
#include <QWidget>

#ifdef MARK_SHOT_WITH_DBUS
#include "pinned_window/pinned_kde_keep_above_bridge.h"
#endif

namespace markshot::shot {
namespace {

#ifdef MARK_SHOT_WITH_DBUS
constexpr const char *kControllerName = "markShotKdeKeepAbove";

/// @brief 获取当前应用共享的 KWin 状态通道
/// @return 随应用销毁的通道对象
KdeKeepAboveBridge *stateBridge()
{
    static QPointer<KdeKeepAboveBridge> bridge;
    if (!bridge) {
        bridge = new KdeKeepAboveBridge(qApp);
    }
    return bridge;
}

/// @brief 随窗口管理稳定标题身份和独立置顶状态
class KdeKeepAboveController final : public QObject {
public:
    /// @brief 为同类窗口添加编号，供用户和 KWin 区分
    /// @param window 需要管理的钉图或 OCR 窗口
    explicit KdeKeepAboveController(QWidget *window) : QObject(window), m_bridge(stateBridge())
    {
        static quint64 nextWindowNumber = 0;
        setObjectName(QString::fromLatin1(kControllerName));
        m_title = QStringLiteral("%1 (%2)").arg(window->windowTitle().trimmed()).arg(++nextWindowNumber);
        window->setWindowTitle(m_title);
    }

    /// @brief 关闭窗口时删除对应状态，保留其他窗口的请求
    ~KdeKeepAboveController() override
    {
        if (m_bridge) {
            m_bridge->removeWindow(m_title);
        }
    }

    /// @brief 更新当前窗口的目标状态
    /// @param alwaysOnTop 是否保持置顶
    /// @return 请求是否成功提交到状态通道
    bool apply(bool alwaysOnTop)
    {
        return m_bridge && m_bridge->setWindowState(m_title, alwaysOnTop);
    }

private:
    QString m_title;
    QPointer<KdeKeepAboveBridge> m_bridge;
};
#endif

}  // namespace

bool usesKdePinnedKeepAbove()
{
#ifndef MARK_SHOT_WITH_DBUS
    return false;
#else
    const QString platform = QGuiApplication::platformName().toLower();
    if (!platform.contains(QStringLiteral("wayland")) && platform != QStringLiteral("xcb")) {
        return false;
    }
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString desktop = (env.value(QStringLiteral("XDG_CURRENT_DESKTOP")) + QLatin1Char(':')
        + env.value(QStringLiteral("XDG_SESSION_DESKTOP")) + QLatin1Char(':')
        + env.value(QStringLiteral("DESKTOP_SESSION"))).toLower();
    return desktop.contains(QStringLiteral("kde")) || desktop.contains(QStringLiteral("plasma"));
#endif
}

bool applyKdePinnedWindowKeepAbove(QWidget *window, bool alwaysOnTop)
{
#ifndef MARK_SHOT_WITH_DBUS
    Q_UNUSED(window)
    Q_UNUSED(alwaysOnTop)
    return false;
#else
    if (!window || !usesKdePinnedKeepAbove()) {
        return false;
    }
    auto *controller = static_cast<KdeKeepAboveController *>(
        window->findChild<QObject *>(QString::fromLatin1(kControllerName), Qt::FindDirectChildrenOnly));
    if (!controller) {
        if (!isKdePinnedKeepAboveTitle(window->windowTitle())) {
            return false;
        }
        controller = new KdeKeepAboveController(window);
    }
    return controller->apply(alwaysOnTop);
#endif
}

}  // namespace markshot::shot
