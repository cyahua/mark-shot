#pragma once

class KWinScriptProbe;

/// @brief 在真实 KWin 中验证钉图原生缩放与图片比例
/// @param probe 当前隔离会话的窗口状态探针
/// @return 通过的方向用例数量，失败时抛出异常
int checkPinnedNativeResizes(KWinScriptProbe &probe);
