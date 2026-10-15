#include "ui/canvas_scrollbar_style.h"

#include <QCoreApplication>
#include <QEvent>
#include <QScrollBar>
#include <QStyle>

namespace markshot::ui {
namespace {

constexpr char kHoverProperty[] = "canvasHover";

/**
 * 切换滚动条悬停属性并重新套用样式表
 * @param bar 画布滚动条
 * @param hovered 是否处于悬停或拖动状态
 * @return 无返回值
 */
void setCanvasScrollBarHovered(QScrollBar *bar, bool hovered)
{
    if (bar->property(kHoverProperty).toBool() == hovered) {
        return;
    }
    bar->setProperty(kHoverProperty, hovered);
    bar->style()->unpolish(bar);
    bar->style()->polish(bar);
    bar->update();
}

/// @brief 跟踪画布滚动条的进入、离开与拖动结束，维护悬停外观
class CanvasScrollBarHover final : public QObject {
public:
    /// @brief 创建随滚动条销毁的悬停跟踪器
    /// @param bar 被跟踪的滚动条
    explicit CanvasScrollBarHover(QScrollBar *bar) : QObject(bar) {}

protected:
    /// @brief 根据指针进出更新外观，进入时让画布结束自身悬停状态，不拦截原事件
    /// @param watched 画布滚动条
    /// @param event 控件事件
    /// @return 始终返回 false
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        auto *bar = static_cast<QScrollBar *>(watched);
        switch (event->type()) {
        case QEvent::Enter:
            setCanvasScrollBarHovered(bar, true);
            // 1. 【画布滚动条】【悬停】指针从画布移入子控件时画布收不到离开事件，主动结束笔刷预览与软件指针
            if (QWidget *canvas = bar->parentWidget()) {
                QEvent leave(QEvent::Leave);
                QCoreApplication::sendEvent(canvas, &leave);
            }
            break;
        case QEvent::Leave:
            // 2. 【画布滚动条】【拖动】拖动滑块时保持加宽，松开后再按指针位置恢复
            if (!bar->isSliderDown()) {
                setCanvasScrollBarHovered(bar, false);
            }
            break;
        case QEvent::MouseButtonRelease:
            if (!bar->underMouse()) {
                setCanvasScrollBarHovered(bar, false);
            }
            break;
        case QEvent::Hide:
            setCanvasScrollBarHovered(bar, false);
            break;
        default:
            break;
        }
        return false;
    }
};

}  // namespace

QString canvasScrollBarStyleSheet(int extent)
{
    // 1. 空闲时为带深色描边的细滑块，浅色和深色图片上都可辨认；轨道与箭头保持透明
    // 2. 悬停时显示圆角轨道并加宽滑块，直接悬停滑块再提亮，拖动时使用强调色
    return QStringLiteral(
        "QScrollBar { background: transparent; border: none; }"
        "QScrollBar:horizontal { height: %1px; margin: 4px 3px; }"
        "QScrollBar:vertical { width: %1px; margin: 3px 4px; }"
        "QScrollBar::handle { background: rgba(203, 213, 225, 150);"
        " border: 1px solid rgba(15, 23, 42, 90); border-radius: 3px; }"
        "QScrollBar::handle:horizontal { min-width: 36px; }"
        "QScrollBar::handle:vertical { min-height: 36px; }"
        "QScrollBar[canvasHover=\"true\"] { background: rgba(15, 23, 42, 110); border-radius: 5px; }"
        "QScrollBar[canvasHover=\"true\"]:horizontal { margin: 2px 2px; }"
        "QScrollBar[canvasHover=\"true\"]:vertical { margin: 2px 2px; }"
        "QScrollBar[canvasHover=\"true\"]::handle { background: rgba(226, 232, 240, 215);"
        " border-color: rgba(15, 23, 42, 120); border-radius: 4px; }"
        "QScrollBar::handle:hover { background: rgba(241, 245, 249, 240); }"
        "QScrollBar::handle:pressed { background: rgba(94, 234, 212, 230);"
        " border-color: rgba(15, 118, 110, 160); }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; background: transparent; border: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }")
        .arg(extent);
}

void setupCanvasScrollBar(QScrollBar *bar, int extent)
{
    if (!bar) {
        return;
    }
    // 1. 滚动条不继承画布的十字、笔刷或隐藏光标
    bar->setCursor(Qt::ArrowCursor);
    bar->setAttribute(Qt::WA_Hover, true);
    bar->setProperty(kHoverProperty, false);
    bar->setStyleSheet(canvasScrollBarStyleSheet(extent));
    bar->installEventFilter(new CanvasScrollBarHover(bar));
}

}  // namespace markshot::ui
