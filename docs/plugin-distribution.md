# 插件分发规范

本文定义第三方 Mark Shot Provider 插件的发布格式。该规范面向 GitHub Release、系统包、用户手工安装三种分发方式。

## 包命名

推荐格式：

```text
mark-shot-plugin-<capability>-<provider>-<version>-<platform>.<ext>
```

示例：

```text
mark-shot-plugin-ocr-paddle-0.1.0-linux-x86_64.tar.gz
mark-shot-plugin-translate-deepl-0.2.1-windows-x86_64.zip
```

## 包目录结构

```text
mark-shot-plugin-example/
├── README.md
├── metadata.json
├── LICENSE
└── libmark-shot-example.so
```

Linux 安装目标目录为 `~/.local/share/mark-shot/plugins`。系统包可以安装到 `${libdir}/mark-shot/plugins`。

## metadata.json

必填字段：

```json
{
    "name": "mark-shot-example",
    "version": "0.1.0",
    "vendor": "example",
    "markShotMinVersion": "0.1.38",
    "capabilities": [
        {
            "type": "ocr",
            "providerId": "example-ocr",
            "displayName": "Example OCR"
        }
    ]
}
```

字段约定：

| 字段 | 必填 | 说明 |
|---|---:|---|
| `name` | 是 | 插件包名，建议与动态库 target 对齐 |
| `version` | 是 | 插件版本，建议 SemVer |
| `vendor` | 是 | 发布方标识 |
| `markShotMinVersion` | 是 | 最低兼容 Mark Shot 版本 |
| `capabilities` | 是 | 能力列表 |
| `capabilities[].type` | 是 | `ocr`、`translation`、`code-scan` |
| `capabilities[].providerId` | 是 | 与 C++ `providerId()` 完全一致 |
| `capabilities[].displayName` | 是 | 与 C++ `displayName()` 建议一致 |
| `homepage` | 否 | 项目主页 |
| `license` | 否 | 许可证标识 |
| `dependencies` | 否 | 运行时依赖说明 |

## 兼容策略

- Provider 接口通过 IID 版本区分，例如 `dev.mark-shot.OcrProviderPlugin/1.0`。
- 如果接口发生不兼容变更，Mark Shot 会提升 IID 版本号。
- 插件必须声明 `markShotMinVersion`，避免用户安装到过旧版本后无法诊断。
- `providerId` 发布后应保持稳定，不要把品牌名、模型版本或地区写成会频繁变化的值。

## 发布检查

发布前至少执行：

```bash
cmake -S . -B build -DMARK_SHOT_PLUGIN_SDK_DIR=/path/to/plugin-sdk
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

如果插件依赖外部模型或服务，`README.md` 必须说明：

- 依赖安装方式
- 环境变量
- 模型文件目录
- API Key 配置方式
- 常见不可用原因

如果插件要进入 GitHub 插件市场，还需要把 Release 动态库资产写入市场索引。
索引格式见 `docs/plugin-index-schema.md`，示例见 `examples/plugin-index.example.json`。

## PP-OCR Linux 发布检查

OCR 插件只使用 ONNX Runtime 的公开 API，不应直接链接 Protobuf、Abseil 或
UTF-8 支持库。直接链接这些内部依赖会把构建环境的精确库版本写入插件，导致
系统升级后出现 `libprotobuf.so.<旧版本>` 等加载错误。

仓库测试通过市场安装器安装真实动态库，再使用 `QPluginLoader` 加载。没有模型时
仍检查插件加载；模型齐全时继续验证中文和英文识别。Linux 还检查 ELF 直接依赖：

```bash
cmake --build build --target mark-shot-ocr-rapid-plugin-test --parallel
QT_QPA_PLATFORM=offscreen ctest --test-dir build -R '^ocr-rapid-plugin' --output-on-failure --no-tests=error
```

发布资产应取自 `cmake --install` 的安装目录。下载发布资产后，可以直接验证安装和识别：

```bash
QT_QPA_PLATFORM=offscreen \
MARK_SHOT_TEST_OCR_PLUGIN_PATH=/path/to/downloaded-plugin.so \
build/mark-shot-ocr-rapid-plugin-test
```

`v0.1.52` 的 Linux `r1` 修订资产使用 Qt 6.11 和 ONNX Runtime 1.29 构建，
需要对应或更新的兼容运行时，以及 PP-OCR 模型。它移除了无用的内部依赖绑定；
用户仍需安装 ONNX Runtime。修订资产使用独立文件名，市场索引同步更新下载地址、
文件大小和 SHA-256，避免继续分发旧库。

## Windows 插件更新与模型目录

Windows 会锁定已经加载的 DLL。更新已有插件时，安装器校验下载文件后将新库保存到
用户插件目录下的 `.pending-updates`，当前识别任务继续使用旧库。退出所有 Mark Shot
进程并重新启动后，程序在加载插件前应用更新。如果其他进程仍占用 DLL，程序保留旧库
和待更新文件，下次启动时重试。用户目录中的插件优先于安装包附带的插件。

OCR 下载器和插件共用模型目录规则：Windows 默认使用
`%LOCALAPPDATA%/mark-shot/models`，Linux 默认使用
`${XDG_DATA_HOME:-~/.local/share}/mark-shot/models`。`MARK_SHOT_OCR_MODEL_DIR`
同时覆盖下载和查找目录。旧版 `~/.local/share/mark-shot/models` 仍作为兼容搜索位置。
下载完模型后，设置页会重新检查插件状态；模型缺失导致的识别失败不会永久阻止后续重试。
