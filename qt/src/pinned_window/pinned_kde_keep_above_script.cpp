#include "pinned_window/pinned_kde_keep_above_script.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>

namespace markshot::shot {

bool isKdePinnedKeepAboveTitle(const QString &title)
{
    static const QStringList titles = {
        QStringLiteral("Pinned Mark Shot"), QStringLiteral("钉住的截图"),
        QStringLiteral("OCR Result"), QStringLiteral("OCR 结果"),
    };
    return titles.contains(title.trimmed());
}

QString kdePinnedKeepAboveScriptSource(const QString &service, qint64 processId, const QString &pluginName)
{
    if (service.trimmed().isEmpty() || processId <= 0 || pluginName.isEmpty()) {
        return {};
    }
    QString identity = QString::fromUtf8(
        QJsonDocument(QJsonArray{service, processId, pluginName}).toJson(QJsonDocument::Compact));
    identity.replace(QChar(0x2028), QStringLiteral("\\u2028"));
    identity.replace(QChar(0x2029), QStringLiteral("\\u2029"));
    return QStringLiteral(R"JS(
(function() {
    const identity = %1;
    var states = {};

    /**
     * 获取当前桌面的窗口。
     * @return Plasma 5 或 Plasma 6 的窗口列表。
     */
    function windows() {
        return (workspace.windowList && workspace.windowList())
            || (workspace.clientList && workspace.clientList()) || workspace.stackingOrder || [];
    }

    /**
     * 根据进程和带编号标题设置一个窗口的层级。
     * @param window KWin 窗口对象。
     * @return 无返回值。
     */
    function apply(window) {
        if (!window || window.pid !== identity[1]) {
            return;
        }
        const caption = String(window.caption || '');
        const titles = Object.keys(states);
        for (var i = 0; i < titles.length; ++i) {
            const title = titles[i];
            if (caption === title || caption.indexOf(title + ' ') === 0) {
                window.keepAbove = states[title];
                return;
            }
        }
    }

    /**
     * 在窗口映射和标题就绪后恢复对应状态。
     * @param window 已有或新建立的窗口。
     * @return 无返回值。
     */
    function watch(window) {
        apply(window);
        if (window && window.pid === identity[1] && window.captionChanged) {
            window.captionChanged.connect(function() { apply(window); });
        }
    }

    /**
     * 异步等待下一份状态，避免重新加载脚本和覆盖其他窗口。
     * @param revision 已经收到的状态修订。
     * @return 无返回值。
     */
    function waitForState(revision) {
        callDBus(identity[0], '/org/markshot/KdePinnedWindows', 'org.markshot.KdePinnedWindows',
            'WaitForState', revision, function(payload) {
                if (typeof payload !== 'string' || !payload) {
                    callDBus('org.kde.KWin', '/Scripting', 'org.kde.kwin.Scripting', 'unloadScript', identity[2]);
                    return;
                }
                const state = JSON.parse(payload);
                states = state.windows;
                const current = windows();
                for (var i = 0; i < current.length; ++i) {
                    apply(current[i]);
                }
                waitForState(state.revision);
            });
    }

    // 1. 【KDE置顶】【窗口生命周期】只注册一次窗口和标题监听
    const current = windows();
    for (var i = 0; i < current.length; ++i) {
        watch(current[i]);
    }
    const added = workspace.windowAdded || workspace.clientAdded;
    if (added && typeof added.connect === 'function') {
        added.connect(watch);
    }
    // 2. 【KDE置顶】【状态订阅】初始请求立即取得完整状态，之后等待变更或心跳
    waitForState(-1);
})();
)JS").arg(identity);
}

}  // namespace markshot::shot
