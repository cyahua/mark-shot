#include "pinned_window/pinned_kde_keep_above_bridge.h"

#ifdef MARK_SHOT_WITH_DBUS
#include "debug_log.h"
#include "pinned_window/pinned_kde_keep_above_script.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace markshot::shot {
namespace {

constexpr const char *kStatePath = "/org/markshot/KdePinnedWindows";
constexpr const char *kStateInterface = "org.markshot.KdePinnedWindows";

/// @brief 创建不需要接口自省的 KWin 脚本管理调用
/// @param method 管理方法名称
/// @param arguments 方法参数
/// @return 发往当前会话 KWin 的调用消息
QDBusMessage scriptingCall(const QString &method, const QVariantList &arguments)
{
    auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
        QStringLiteral("/Scripting"), QStringLiteral("org.kde.kwin.Scripting"), method);
    message.setArguments(arguments);
    return message;
}

}  // namespace

KdeKeepAboveBridge::KdeKeepAboveBridge(QObject *parent) : QDBusVirtualObject(parent)
{
    auto bus = QDBusConnection::sessionBus();
    m_registered = bus.isConnected() && bus.registerVirtualObject(QString::fromLatin1(kStatePath), this);
    m_plugin = QStringLiteral("mark-shot-pinned-%1").arg(QCoreApplication::applicationPid());
    const QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!cache.isEmpty() && QDir().mkpath(cache)) {
        m_file.setFileTemplate(QDir(cache).filePath(m_plugin + QStringLiteral("-XXXXXX.js")));
    }

    m_heartbeat.setInterval(10000);
    m_heartbeat.setSingleShot(true);
    connect(&m_heartbeat, &QTimer::timeout, this, [this] { replyState(); });
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { stopScript(); });
    auto *watcher = new QDBusServiceWatcher(QStringLiteral("org.kde.KWin"), bus,
        QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &owner) {
        m_started = false;
        m_waitingCall = {};
        m_heartbeat.stop();
        if (!owner.isEmpty() && !m_windows.isEmpty()) {
            startScript();
        }
    });
}

KdeKeepAboveBridge::~KdeKeepAboveBridge()
{
    stopScript();
    if (m_registered) {
        QDBusConnection::sessionBus().unregisterObject(QString::fromLatin1(kStatePath));
    }
}

bool KdeKeepAboveBridge::setWindowState(const QString &title, bool alwaysOnTop)
{
    m_windows.insert(title, alwaysOnTop);
    ++m_revision;
    replyState();
    return m_started || startScript();
}

void KdeKeepAboveBridge::removeWindow(const QString &title)
{
    if (m_windows.remove(title) > 0) {
        ++m_revision;
        replyState();
    }
}

QString KdeKeepAboveBridge::introspect(const QString &) const
{
    return QStringLiteral("<interface name=\"org.markshot.KdePinnedWindows\">"
        "<method name=\"WaitForState\"><arg name=\"revision\" type=\"x\" direction=\"in\"/>"
        "<arg name=\"state\" type=\"s\" direction=\"out\"/></method></interface>");
}

bool KdeKeepAboveBridge::handleMessage(const QDBusMessage &message, const QDBusConnection &)
{
    if (message.interface() != QString::fromLatin1(kStateInterface)
        || message.member() != QStringLiteral("WaitForState") || message.arguments().size() != 1) {
        return false;
    }
    // 1. 【钉图】【KDE订阅】合并等待者并保留最新修订，过期请求立即获取当前状态
    replyState();
    m_waitingCall = message;
    message.setDelayedReply(true);
    if (message.arguments().first().toLongLong() != m_revision) {
        replyState();
    } else {
        m_heartbeat.start();
    }
    return true;
}

void KdeKeepAboveBridge::replyState()
{
    if (m_waitingCall.type() != QDBusMessage::MethodCallMessage) {
        return;
    }
    QJsonObject windows;
    for (auto it = m_windows.cbegin(); it != m_windows.cend(); ++it) {
        windows.insert(it.key(), it.value());
    }
    const QString payload = QString::fromUtf8(QJsonDocument(
        QJsonObject{{QStringLiteral("revision"), m_revision}, {QStringLiteral("windows"), windows}})
        .toJson(QJsonDocument::Compact));
    const QDBusMessage reply = m_waitingCall.createReply(QVariantList{payload});
    m_waitingCall = {};
    m_heartbeat.stop();
    QDBusConnection::sessionBus().send(reply);
}

bool KdeKeepAboveBridge::startScript()
{
    if (!m_registered) {
        return false;
    }
    // 1. 【钉图】【KDE脚本】每个应用只加载一次，避免 KWin 按脚本数量复用对象编号
    auto bus = QDBusConnection::sessionBus();
    const QByteArray source = kdePinnedKeepAboveScriptSource(
        bus.baseService(), QCoreApplication::applicationPid(), m_plugin).toUtf8();
    if ((!m_file.isOpen() && !m_file.open()) || !m_file.resize(0) || !m_file.seek(0)
        || m_file.write(source) != source.size() || !m_file.flush()) {
        markshot::debugLog("pinned-window", "【钉图】【KDE置顶】无法写入应用脚本");
        return false;
    }
    const auto loaded = bus.call(scriptingCall(QStringLiteral("loadScript"),
        {m_file.fileName(), m_plugin}), QDBus::Block, 1000);
    if (loaded.type() == QDBusMessage::ErrorMessage || loaded.arguments().isEmpty()
        || loaded.arguments().first().toInt() < 0) {
        markshot::debugLog("pinned-window", "【钉图】【KDE置顶】加载应用脚本失败: %s",
                          loaded.errorMessage().toUtf8().constData());
        return false;
    }

    // 2. 【钉图】【KDE脚本】兼容 Plasma 6 和 Plasma 5 的脚本对象路径
    const int id = loaded.arguments().first().toInt();
    QDBusMessage reply;
    for (const QString &path : {QStringLiteral("/Scripting/Script%1").arg(id),
                                QStringLiteral("/%1").arg(id)}) {
        const auto run = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"), path,
            QStringLiteral("org.kde.kwin.Script"), QStringLiteral("run"));
        reply = bus.call(run, QDBus::Block, 1000);
        if (reply.type() != QDBusMessage::ErrorMessage) {
            m_started = true;
            return true;
        }
    }
    markshot::debugLog("pinned-window", "【钉图】【KDE置顶】运行应用脚本失败: %s",
                      reply.errorMessage().toUtf8().constData());
    bus.asyncCall(scriptingCall(QStringLiteral("unloadScript"), {m_plugin}));
    return false;
}

void KdeKeepAboveBridge::stopScript()
{
    m_heartbeat.stop();
    m_waitingCall = {};
    if (m_started) {
        QDBusConnection::sessionBus().asyncCall(scriptingCall(QStringLiteral("unloadScript"), {m_plugin}));
        m_started = false;
    }
}

}  // namespace markshot::shot
#endif
