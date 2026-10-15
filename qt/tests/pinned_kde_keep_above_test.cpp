#include "pinned_window/pinned_kde_keep_above_script.h"

#include <QtTest/QtTest>

#ifdef MARK_SHOT_TEST_WITH_QML
#include <QJSEngine>

namespace {

/// @brief 提供 KWin 窗口和信号的最小行为模型
/// @return 可在 JavaScript 引擎执行的测试环境
QString workspaceFixture()
{
    return QStringLiteral(R"JS(
/** @return 可注册和触发回调的测试信号。 */
function signal() {
    var callbacks = [];
    return {connect: function(callback) { callbacks.push(callback); },
            emit: function(value) { callbacks.forEach(function(callback) { callback(value); }); }};
}
/**
 * @param pid 所属进程号。
 * @param caption 窗口标题。
 * @param above 初始置顶状态。
 * @return 测试窗口对象。
 */
function makeWindow(pid, caption, above) {
    return {pid: pid, caption: caption, keepAbove: above, captionChanged: signal()};
}
var target = makeWindow(123, 'OCR Result (1)', false);
var sibling = makeWindow(123, 'OCR Result (2)', false);
var foreign = makeWindow(456, 'OCR Result (1)', false);
var windows = [target, sibling, foreign];
var workspace = {windowList: function() { return windows; }, windowAdded: signal()};
var stateReceiver;
var requestedRevision = -1;
/**
 * 保存脚本的异步订阅请求，供测试发送状态。
 * @param service 服务名称。
 * @param path 对象路径。
 * @param iface 接口名称。
 * @param method 方法名称。
 * @param revision 已收到的修订。
 * @param callback 接收状态的回调。
 * @return 无返回值。
 */
function callDBus(service, path, iface, method, revision, callback) {
    stateReceiver = callback;
    requestedRevision = revision;
}
)JS");
}

}  // namespace
#endif

class PinnedKdeKeepAboveTest : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证只接受应用管理的中英文窗口标题
    /// @return 无返回值
    void matchesPinnedAndOcrTitles()
    {
        for (const QString &title : {QStringLiteral("Pinned Mark Shot"), QStringLiteral("钉住的截图"),
                                     QStringLiteral("OCR Result"), QStringLiteral("OCR 结果")}) {
            QVERIFY(markshot::shot::isKdePinnedKeepAboveTitle(title));
        }
        QVERIFY(!markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("Mark Shot")));
        QVERIFY(!markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("Settings")));
    }

    /// @brief 拒绝缺失窗口身份的脚本，避免误操作其他窗口
    /// @return 无返回值
    void rejectsIncompleteIdentity()
    {
        QVERIFY(markshot::shot::kdePinnedKeepAboveScriptSource({}, 123, QStringLiteral("test")).isEmpty());
        QVERIFY(markshot::shot::kdePinnedKeepAboveScriptSource(QStringLiteral(":1.42"), 0, QStringLiteral("test")).isEmpty());
    }

#ifdef MARK_SHOT_TEST_WITH_QML
    /// @brief 实际执行脚本，验证同类窗口和其他进程不会受到影响
    /// @return 无返回值
    void isolatesWindowAndProcess()
    {
        QJSEngine engine;
        QVERIFY(!engine.evaluate(workspaceFixture()).isError());
        QVERIFY(!engine.evaluate(markshot::shot::kdePinnedKeepAboveScriptSource(
            QStringLiteral(":1.42"), 123, QStringLiteral("test"))).isError());
        QVERIFY(!engine.evaluate(QStringLiteral(
            "stateReceiver(JSON.stringify({revision: 1, windows: {'OCR Result (1)': true}}));")).isError());
        QVERIFY(engine.evaluate(QStringLiteral("target.keepAbove")).toBool());
        QVERIFY(!engine.evaluate(QStringLiteral("sibling.keepAbove")).toBool());
        QVERIFY(!engine.evaluate(QStringLiteral("foreign.keepAbove")).toBool());
        QCOMPARE(engine.evaluate(QStringLiteral("requestedRevision")).toInt(), 1);
    }

    /// @brief 验证取消置顶仅影响目标，并兼容 Plasma 5 的窗口列表接口
    /// @return 无返回值
    void disablesOnlyTargetOnLegacyWorkspace()
    {
        QJSEngine engine;
        QVERIFY(!engine.evaluate(workspaceFixture() + QStringLiteral(
            "target.keepAbove = sibling.keepAbove = foreign.keepAbove = true;"
            "workspace.clientList = workspace.windowList; delete workspace.windowList;"
            "workspace.clientAdded = workspace.windowAdded; delete workspace.windowAdded;")).isError());
        QVERIFY(!engine.evaluate(markshot::shot::kdePinnedKeepAboveScriptSource(
            QStringLiteral(":1.42"), 123, QStringLiteral("test"))).isError());
        QVERIFY(!engine.evaluate(QStringLiteral(
            "stateReceiver(JSON.stringify({revision: 1, windows: {'OCR Result (1)': false}}));")).isError());
        QVERIFY(!engine.evaluate(QStringLiteral("target.keepAbove")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral("sibling.keepAbove && foreign.keepAbove")).toBool());
    }

    /// @brief 验证重新建立窗口以及晚于映射到达的标题仍能恢复置顶
    /// @return 无返回值
    void reappliesAfterMappingAndCaptionChanges()
    {
        QJSEngine engine;
        QVERIFY(!engine.evaluate(workspaceFixture()).isError());
        QVERIFY(!engine.evaluate(markshot::shot::kdePinnedKeepAboveScriptSource(
            QStringLiteral(":1.42"), 123, QStringLiteral("test"))).isError());
        QVERIFY(!engine.evaluate(QStringLiteral(
            "stateReceiver(JSON.stringify({revision: 1, windows: {'OCR Result (1)': true}}));")).isError());
        // 1. 【KDE测试】【重新映射】窗口建立时标题尚未就绪
        QVERIFY(!engine.evaluate(QStringLiteral(
            "var remapped = makeWindow(123, '', false); workspace.windowAdded.emit(remapped);")).isError());
        QVERIFY(!engine.evaluate(QStringLiteral("remapped.keepAbove")).toBool());
        // 2. 【KDE测试】【标题就绪】窗口管理器附加的快捷键后缀不影响身份
        QVERIFY(!engine.evaluate(QStringLiteral(
            "remapped.caption = 'OCR Result (1) {Alt+1}'; remapped.captionChanged.emit();")).isError());
        QVERIFY(engine.evaluate(QStringLiteral("remapped.keepAbove")).toBool());
    }

    /// @brief 验证引号、换行及 Unicode 行分隔符不会改变脚本含义
    /// @return 无返回值
    void preservesSpecialTitleCharacters()
    {
        QJSEngine engine;
        QVERIFY(!engine.evaluate(workspaceFixture()).isError());
        const QString title = QStringLiteral("OCR \"O'Clock\"\\\n") + QChar(0x2028) + QChar(0x2029);
        engine.globalObject().setProperty(QStringLiteral("expectedTitle"), title);
        QVERIFY(!engine.evaluate(QStringLiteral("target.caption = expectedTitle;")).isError());
        QVERIFY(!engine.evaluate(markshot::shot::kdePinnedKeepAboveScriptSource(
            QStringLiteral(":1.42"), 123, QStringLiteral("test"))).isError());
        QVERIFY(!engine.evaluate(QStringLiteral(
            "var states = {}; states[expectedTitle] = true;"
            "stateReceiver(JSON.stringify({revision: 1, windows: states}));")).isError());
        QVERIFY(engine.evaluate(QStringLiteral("target.keepAbove")).toBool());
        QVERIFY(!engine.evaluate(QStringLiteral("sibling.keepAbove || foreign.keepAbove")).toBool());
    }
#else
    /// @brief 未安装 JavaScript 引擎时明确报告行为测试缺失
    /// @return 无返回值
    void scriptBehaviorRequiresQml()
    {
        QSKIP("Qt Qml is required to execute KWin script behavior tests");
    }
#endif
};

QTEST_GUILESS_MAIN(PinnedKdeKeepAboveTest)

#include "pinned_kde_keep_above_test.moc"
