#include "kwin_script_probe.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QtTest>

#include <stdexcept>

KWinScriptProbe::KWinScriptProbe()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral("org.markshot.KdeWindowCheck"))
        || !bus.registerVirtualObject(QStringLiteral("/Probe"), this)) {
        throw std::runtime_error("Cannot register isolated KWin probe");
    }
}

QString KWinScriptProbe::introspect(const QString &) const
{
    return QStringLiteral("<interface name=\"org.markshot.KdeWindowCheck\"><method name=\"Report\">"
                          "<arg name=\"windows\" type=\"s\" direction=\"in\"/></method></interface>");
}

bool KWinScriptProbe::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    if (message.member() != QStringLiteral("Report") || message.arguments().size() != 1) {
        return false;
    }
    m_windows = QJsonDocument::fromJson(message.arguments().first().toString().toUtf8()).array();
    m_received = true;
    connection.send(message.createReply());
    return true;
}

QJsonArray KWinScriptProbe::snapshot(const QString &action)
{
    // 1. 【KDE测试】【窗口观察】仅在当前隔离会话加载观察脚本
    m_received = false;
    const QString plugin = QStringLiteral("mark-shot-window-probe-%1").arg(++m_sequence);
    QTemporaryFile file;
    if (!file.open()) {
        throw std::runtime_error("Cannot create KWin probe script");
    }
    const QString source = action + QStringLiteral(R"JS(
var windows = workspace.stackingOrder || workspace.windowList();
var report = [];
for (var i = 0; i < windows.length; ++i) {
    var w = windows[i];
    var g = w.frameGeometry;
    report.push({id: String(w.internalId), caption: w.caption,
        resourceClass: String(w.resourceClass), resourceName: String(w.resourceName),
        pid: w.pid, keepAbove: w.keepAbove, active: w.active,
        x: g.x, y: g.y, width: g.width, height: g.height,
        movable: w.moveable, resizable: w.resizeable});
}
callDBus('org.markshot.KdeWindowCheck', '/Probe', 'org.markshot.KdeWindowCheck',
         'Report', JSON.stringify(report));
)JS");
    file.write(source.toUtf8());
    file.flush();
    QDBusInterface scripting(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                             QStringLiteral("org.kde.kwin.Scripting"), QDBusConnection::sessionBus());
    const QDBusReply<int> id = scripting.call(QStringLiteral("loadScript"), file.fileName(), plugin);
    if (!id.isValid() || id.value() < 0) {
        throw std::runtime_error("Cannot load KWin probe script");
    }
    QDBusInterface script(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting/Script%1").arg(id.value()),
                          QStringLiteral("org.kde.kwin.Script"), QDBusConnection::sessionBus());
    const QDBusMessage reply = script.call(QStringLiteral("run"));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        throw std::runtime_error(reply.errorMessage().toStdString());
    }
    // 2. 【KDE测试】【状态回传】检查合成器报告，避免把脚本加载成功当作置顶成功
    QElapsedTimer timer;
    timer.start();
    while (!m_received && timer.elapsed() < 3000) {
        QTest::qWait(10);
    }
    scripting.call(QStringLiteral("unloadScript"), plugin);
    if (!m_received) {
        throw std::runtime_error("KWin did not report window state");
    }
    return m_windows;
}

int KWinScriptProbe::scriptCount() const
{
    QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                              QStringLiteral("org.freedesktop.DBus.Introspectable"), QDBusConnection::sessionBus());
    const QDBusReply<QString> reply = interface.call(QStringLiteral("Introspect"));
    if (!reply.isValid()) {
        throw std::runtime_error("Cannot inspect KWin script lifetime");
    }
    QXmlStreamReader xml(reply.value());
    int count = 0;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QStringLiteral("node")
            && xml.attributes().value(QStringLiteral("name")).startsWith(QStringLiteral("Script"))) {
            ++count;
        }
    }
    return count;
}
