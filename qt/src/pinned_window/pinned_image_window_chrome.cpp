#include "pinned_window/pinned_image_window.h"

#include "pinned_window/pinned_non_window_chrome.h"

namespace markshot::shot {

void PinnedImageWindow::applyPinnedNonWindowChromeIfEnabled()
{
    if (!FEATURE_PINNED_NON_WINDOW_CHROME_ENABLED) {
        return;
    }
    // 1. 只避免激活。Qt::Tool 会变成截图覆盖层的附属窗口，覆盖层关闭时贴图一起消失
    setAttribute(Qt::WA_ShowWithoutActivating, true);
}

void PinnedImageWindow::updatePinnedNonWindowHoverIfEnabled(QEvent *event)
{
    Q_UNUSED(event);
}

bool PinnedImageWindow::paintPinnedNonWindowChromeIfEnabled(QPainter &painter, const QRectF &imageRect)
{
    Q_UNUSED(painter);
    Q_UNUSED(imageRect);
    // 边框只使用配置的颜色和粗细，不再叠加阴影或另一套描边
    return false;
}

}  // namespace markshot::shot
