#pragma once

#include "providers/provider_plugin_info.h"

#include <QStringList>
#include <QVector>

class QObject;
class QPluginLoader;

namespace markshot::plugin {
class OcrProviderPlugin;
class TranslateProviderPlugin;
class CodeScanProviderPlugin;
}

namespace markshot::providers {

/**
 * 插件目录中发现的候选动态库。
 *
 * 发现阶段只读取元数据，实际加载推迟到首次请求对应能力时。
 */
struct ProviderPluginCandidate {
    QString path;
    QVector<ProviderPluginCapability> declaredCapabilities;
    QPluginLoader *loader = nullptr;
    QObject *instance = nullptr;
    bool attempted = false;
    QVector<ProviderPluginInfo> infos;
};

/**
 * provider 插件注册器。
 *
 * 首次访问时扫描插件搜索目录并读取元数据，只加载声明了所需能力的插件，
 * 避免翻译等轻量能力连带加载 ONNX Runtime 等大型依赖。
 * 加载结果进程内缓存，仅主线程访问。
 */
class ProviderPluginRegistry final {
public:
    /**
     * 获取进程级注册器单例。
     * @return 注册器引用。
     */
    static ProviderPluginRegistry &instance();

    /**
     * 枚举已加载的 OCR provider 插件。
     * @return 插件指针列表，生命周期由注册器持有。
     */
    QVector<markshot::plugin::OcrProviderPlugin *> ocrProviders();

    /**
     * 枚举已加载的翻译 provider 插件。
     * @return 插件指针列表。
     */
    QVector<markshot::plugin::TranslateProviderPlugin *> translateProviders();

    /**
     * 枚举已加载的扫码 provider 插件。
     * @return 插件指针列表。
     */
    QVector<markshot::plugin::CodeScanProviderPlugin *> codeScanProviders();

    /**
     * 枚举插件加载诊断信息，会加载全部候选插件。
     * @return 插件诊断条目，按目录发现顺序排列。
     */
    QVector<ProviderPluginInfo> pluginInfos();

    /**
     * 读取插件搜索目录列表。
     * @return 目录绝对路径列表，供设置页展示。
     */
    static QStringList pluginSearchDirs();

private:
    ProviderPluginRegistry() = default;

    /**
     * 应用待更新插件并扫描插件目录元数据，只执行一次。
     * @return 无返回值。
     */
    void discoverOnce();

    /**
     * 加载声明了指定能力或未声明任何已知能力的候选插件。
     * @param capability 需要的插件能力。
     * @return 无返回值。
     */
    void loadCapability(ProviderPluginCapability capability);

    /**
     * 加载全部候选插件。
     * @return 无返回值。
     */
    void loadAll();

    /**
     * 加载单个候选插件并记录诊断信息，重复调用直接返回。
     * @param candidate 候选插件。
     * @return 无返回值。
     */
    void loadCandidate(ProviderPluginCandidate *candidate);

    bool m_discovered = false;
    QVector<ProviderPluginCandidate> m_candidates;
};

}  // namespace markshot::providers
