#include "pinned_resize_checks.h"

#include "kwin_eis_input.h"
#include "kwin_script_probe.h"
#include "pinned_window/pinned_native_resize.h"
#include "pinned_window_top.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QWidget>
#include <QtTest>

#include <cstdio>
#include <stdexcept>

namespace {

using namespace markshot::shot;

/// @brief 通过正式原生缩放控制器处理实际鼠标事件
class ResizeWindow final : public QWidget {
public:
    /// @brief 创建与钉图相同的初始固定尺寸窗口
    ResizeWindow()
        : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint)
    {
        setWindowTitle(QStringLiteral("Pinned Mark Shot"));
        setFixedSize(310, 200);
        m_resize = new PinnedNativeResize(this, {});
        applyPinnedWindowTopState(this, true);
    }

    bool started = false;

protected:
    /// @brief 将鼠标命中的边缘交给生产缩放控制器
    /// @param event 实际鼠标按下事件
    /// @return 无返回值
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            started = m_resize->start(pinnedResizeDirectionAt(QRectF(rect()), event->position(), 8));
            event->accept();
        }
    }

private:
    PinnedNativeResize *m_resize;
};

/// @brief 查询合成器中的实际几何和窗口层级
/// @param probe 当前会话探针
/// @param window 被验证窗口
/// @return KWin 报告的窗口信息
QJsonObject snapshot(KWinScriptProbe &probe, const QWidget &window)
{
    QTest::qWait(100);
    for (const auto &value : probe.snapshot()) {
        const auto item = value.toObject();
        if (item.value(QStringLiteral("caption")).toString() == window.windowTitle()) {
            return item;
        }
    }
    throw std::runtime_error("Native resize window is absent from KWin");
}

/// @brief 读取 KWin 报告的窗口逻辑矩形
/// @param window 合成器状态
/// @return 实际逻辑矩形
QRectF geometry(const QJsonObject &window)
{
    return {window.value(QStringLiteral("x")).toDouble(), window.value(QStringLiteral("y")).toDouble(),
            window.value(QStringLiteral("width")).toDouble(), window.value(QStringLiteral("height")).toDouble()};
}

}  // namespace

int checkPinnedNativeResizes(KWinScriptProbe &probe)
{
    KWinEisInput input;
    int count = 0;
    for (const PinnedResizeDirection direction : {
             PinnedResizeDirection::Left, PinnedResizeDirection::Right,
             PinnedResizeDirection::Top, PinnedResizeDirection::Bottom,
             PinnedResizeDirection::TopLeft, PinnedResizeDirection::TopRight,
             PinnedResizeDirection::BottomLeft, PinnedResizeDirection::BottomRight}) {
        ResizeWindow window;
        window.show();
        const auto initial = snapshot(probe, window);
        const QRectF before = geometry(initial);
        QPointF from = before.center();
        QPointF delta;
        if (pinnedResizeDirectionIncludesLeft(direction)) {
            from.setX(before.left() + 2);
            delta.setX(-60);
        } else if (pinnedResizeDirectionIncludesRight(direction)) {
            from.setX(before.right() - 2);
            delta.setX(60);
        }
        if (pinnedResizeDirectionIncludesTop(direction)) {
            from.setY(before.top() + 2);
            delta.setY(-40);
        } else if (pinnedResizeDirectionIncludesBottom(direction)) {
            from.setY(before.bottom() - 2);
            delta.setY(40);
        }

        // 1. 【KDE测试】【原生缩放】真实输入必须改变尺寸，同时保留对侧边界与图片比例
        input.drag(from, from + delta);
        const auto resized = snapshot(probe, window);
        const QRectF after = geometry(resized);
        const bool anchorsPreserved =
            (!pinnedResizeDirectionIncludesLeft(direction) || qAbs(after.right() - before.right()) < 1.5)
            && (!pinnedResizeDirectionIncludesRight(direction) || qAbs(after.left() - before.left()) < 1.5)
            && (!pinnedResizeDirectionIncludesTop(direction) || qAbs(after.bottom() - before.bottom()) < 1.5)
            && (!pinnedResizeDirectionIncludesBottom(direction) || qAbs(after.top() - before.top()) < 1.5);
        if (!window.started || after.width() < before.width() + 25 || after.height() < before.height() + 15
            || !anchorsPreserved || qAbs(after.width() - after.height() * 1.55) > 1.6
            || !resized.value(QStringLiteral("keepAbove")).toBool()
            || initial.value(QStringLiteral("id")) != resized.value(QStringLiteral("id"))) {
            std::fprintf(stderr, "Observed: resize direction=%d before=%s after=%s\n", static_cast<int>(direction),
                QJsonDocument(initial).toJson(QJsonDocument::Compact).constData(),
                QJsonDocument(resized).toJson(QJsonDocument::Compact).constData());
            throw std::runtime_error("Native pinned resize must preserve opposite anchors, aspect ratio and top state");
        }
        ++count;
        std::fprintf(stdout, "PASS native pinned resize direction %d preserves anchors, ratio and top state\n",
                     static_cast<int>(direction));
        std::fflush(stdout);
    }
    return count;
}
