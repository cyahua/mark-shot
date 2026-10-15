#include <QtCore/qglobal.h>

/**
 * 【插件测试】【版本探测】提供可加载的最小动态库版本标识
 * @return 当前测试库的版本编号
 */
extern "C" Q_DECL_EXPORT int pluginRevision()
{
    return MARK_SHOT_TEST_PLUGIN_REVISION;
}
