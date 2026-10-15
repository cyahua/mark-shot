# 内存与发行体积

## 对比口径

Snow Shot 的内存管理会在空闲条件满足后调用 Windows `EmptyWorkingSet`。这能减少工作集，
但不等于释放全部私有提交内存；再次使用相关页面时还会产生缺页。
不能把一次任务管理器读数当作应用完整内存成本，也不能直接比较 Windows 工作集与 Linux RSS。

本次只参考 `snow-apps` 的构建和内存策略，没有复制其 GPL 应用实现。对比入口包括：

- `snow_shot/src/platform/windows/memoryoptimization.cpp`：工作集回收和采样
- `snow_shot/src/runtime/memoryoptimizationcontroller.cpp`：空闲与任务门控
- `cmake/ProjectWarnings.cmake`：冷路径体积优化与热路径速度优化
- `cmake/SnowShotPackageCompression.cmake`：LZMA 固实压缩
- `cmake/vcpkg-overlay-ports/ffmpeg/`：定制媒体依赖

一张 3840 x 2160 的 32 位图像本身需要约 31.6 MiB。截图编辑状态还需要窗口绘制缓冲、
标注和临时图像，因此不能把托盘空闲时的 20 MB 作为所有工作场景的上限。

## 已实施的优化

- FFmpeg 写出器独立为 `recording/` 中的按需后端；截图和托盘启动不加载媒体编解码库
- 录制对象持有动态库句柄，先销毁编码器和音频线程，再释放库引用；错误文本不引用已卸载模块
- 后端使用带版本检查的内部接口，仅支持同一版本、Qt 和工具链构建，不作为第三方插件协议
- 不透明的预乘截图直接用于区域分析，避免额外复制一份全分辨率图像；透明图像继续解除预乘
- 空闲回收避开录制和 Qt 线程池任务，短延迟不会舍入为零而重复触发
- 常用软件录制预设按帧尺寸限制并发，BGRA 池延迟释放持续闲置槽位；高质量档保留自动并发，详见[录制内存测量](recording-memory.md)
- GUI 保留共享不透明原图、按块生成锐化视口、提前释放失效缓存，设置页和区域识别按需启动，详见[GUI 内存测量](gui-memory.md)
- Release 默认启用链接期优化和未引用段删除，配置界面等冷路径使用体积优化；像素、录制和 Rust 核心保留速度策略
- 发行链接删除调试段，安装时进一步剥离符号；Debug 构建和 Rust FFI 的 panic 隔离策略不变
- Windows 部署裁剪 Qt 可选组件，检查全部依赖，包括按需后端；缺失 DLL 或 FFmpeg ABI 不匹配会阻止打包
- 只在 Schannel 可用时排除 OpenSSL，保留 SVG 图标支持；归档与剥离分开，避免破坏已签名文件

工作区原有的 provider 按能力加载、部分设置页延迟创建、ONNX 分配策略和悬停增量重绘继续保留。

## 本机测量

2026-10-15，Linux x86_64、Qt 6.12.0，Release、offscreen、全新隔离配置、启动托盘后两秒采样。
基线是开始本次优化时已有的 Release 产物，包含当时已编译的工作区改动。

| 指标 | 优化前 | 优化后 |
| --- | ---: | ---: |
| RSS | 75.9 MiB | 50.8 MiB |
| PSS | 41.4 MiB | 28.2 MiB |
| 私有驻留内存 | 31.8 MiB | 14.5 MiB |
| 映射文件数 | 225 | 130 |
| 启动时映射的 FFmpeg 库 | 6 | 0 |
| 构建目录内主程序大小 | 10.24 MiB | 5.43 MiB |

新录制后端另占约 0.14 MiB。主程序大小变化包含删除调试段，不代表代码段同比缩小。
PSS 和私有干净页会随系统共享情况波动，不能据此承诺固定常驻值。

本机裁剪版 FFmpeg 8.0.1 的六个运行库合计 5,837,584 字节，约 5.57 MiB，
不包含 Qt、x264、硬件驱动运行库、ONNX 和应用资源，也不是完整安装包体积。
已验证软件视频、AAC、GIF、调色板和缩放；硬件编码的真实设备路径未在 Windows 上验证。

Windows EXE、Windows 完整压缩包、原生桌面捕获和 Windows 内存尚未实测。
这里的 Linux 数字不等价于已经达到 Windows 20 MB 或 20 MB 压缩包。

## Windows 发行构建

在 MSYS2 UCRT64 中准备 C++、Rust Windows GNU 目标、CMake、Ninja、Qt 6、
pkg-config、make、nasm、x264，以及所需的可选硬件编码头文件。NASM 缺失时构建会失败，
不会为减小体积而关闭像素处理的汇编优化。

```sh
bash qt/scripts/build-minimal-ffmpeg.sh --prefix "$PWD/ffmpeg-min"

cmake -S . -B build-windows-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  -DMARK_SHOT_WITH_LAYER_SHELL=OFF \
  -DMARK_SHOT_RUST_TARGET=x86_64-pc-windows-gnu \
  -DMARK_SHOT_FFMPEG_ROOT="$PWD/ffmpeg-min" \
  -DMARK_SHOT_REQUIRE_FFMPEG=ON
cmake --build build-windows-release --parallel

MARK_SHOT_FFMPEG_PREFIX="$PWD/ffmpeg-min" \
  bash qt/scripts/windows-package.sh \
  --build-dir build-windows-release --stage-dir "$PWD/app-release"

# 如需签名，在此阶段签名 app-release 中的 EXE/DLL
bash qt/scripts/windows-package.sh --stage-dir "$PWD/app-release" \
  --archive "$PWD/mark-shot-windows-x86_64.7z" --archive-only
```

`MARK_SHOT_FFMPEG_ROOT` 隔离头文件和链接库，避免悄悄混入系统全功能版。
部署阶段的 `MARK_SHOT_FFMPEG_PREFIX` 必须指向同一前缀。后端位于
`bin/recording/mark-shot-recording-libav.dll`，不能只分发主 EXE。
保留录制、GIF、OCR 和翻译能力，不使用删除功能的精简包冒充优化结果。

`.7z` 使用 LZMA2 固实压缩，`.zip` 使用最高级别 Deflate，二者大小不能混作同一指标。
已有非空暂存目录必须带有脚本创建的归属标记，脚本不会清空任意用户目录。

裁剪构建启用了 x264 和 GPL，不能将这些二进制当作纯 MIT 依赖发布。
脚本保存 FFmpeg 许可证、组件配置、源码地址和下载摘要；正式发行仍须满足
FFmpeg、x264、Qt 及其他随包依赖的授权和对应源码提供要求。

## 测量与回归

```sh
# Linux：不主动回收工作集，不依赖用户配置，不操作真实桌面
uv run --no-project python scripts/performance/measure_idle_memory.py \
  build-release/mark-shot-rs --assert-no-media

# Windows 暂存目录和实际压缩包体积，JSON 同时列出最大的文件
python scripts/performance/report_package_size.py app-release \
  --archive mark-shot-windows-x86_64.7z

# 只执行与本次改动有关的回归
QT_QPA_PLATFORM=offscreen ctest --test-dir build-release --output-on-failure \
  -R '^(recording-backend|capture-memory|region-image-memory|region-detector|idle-memory-trimmer|settings-card-pages|selection-hover-repaint|libav-recording-process|libav-video-scaler|windows-packaging|runtime-idle-memory|runtime-startup)$'
```

Windows PowerShell 对两个应用采用相同场景、相同等待时间采样：

```powershell
.\scripts\performance\measure_windows_memory.ps1 -ProcessId 1234 -Seconds 60 -Scenario idle
```

该脚本只读取指定进程，不启动、终止或主动回收它，分别输出总工作集和私有提交量。
建议对比托盘空闲、4K 编辑、关闭截图后和停止录制后，明确区分工作集下降与私有提交下降。

`region-image-memory` 验证不透明输入共享像素、全部 alpha 值的转换正确性。
`capture-memory` 六次创建并关闭真实截图窗口，检查原始 4K 帧的最终释放回调。
`recording-backend` 覆盖缺库、版本拒绝、同时持有多个写出器、错误文本生命周期和生产异步写出入口，
Linux 下还检查最后一个写出器销毁后后端不再映射。
