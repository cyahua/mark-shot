# Mark Shot Rust Core

保留 Mark Shot 的 Qt 6 Widgets 界面，将截图几何、像素处理、双向滚动拼接、配置存储和标注编辑历史迁移到 Rust。
本目录可以独立构建，不依赖相邻的 `mark-shot` 或 `snow-apps` 源码目录。

## 模块

```text
crates/
  shot-geometry/  屏幕坐标、混合缩放、选区比例和线段约束
  shot-image/     像素布局、图像滤镜、转置、导出圆角与阴影合成
  shot-stitch/    采样、固定栏检测、双向匹配、增量长图和会话状态
  shot-config/    默认配置、兼容值校验、嵌套更新、文件锁和原子保存
  shot-editor/    完整标注快照、撤销重做、滚轮事务与工具样式持久化
  shot-ffi/       唯一的 C ABI 静态库，负责参数校验和 panic 隔离
qt/
  bridge/        C ABI 声明与 Qt 类型转换
  src/           原有 Qt 窗口、平台后端和核心适配器
  plugins/       原有 OCR、翻译和扫码插件
  tests/         原有 Qt 回归测试及新增边界测试
cmake/           Cargo 与 CMake 构建衔接
docs/            架构与迁移范围
scripts/         源文件尺寸检查
```

Qt 不再保留长截图拼接、编辑历史栈和导出图像合成的 C++ 实现；对应入口通过 C ABI 调用 Rust。
截图采集后端、录制编码、OCR/翻译插件、标注绘制、手势和文本排版仍保留原生实现，详见
[`docs/architecture.md`](docs/architecture.md)。这不是全量 C++ 替换。

## 构建

必需：Rust 1.85+、Cargo、CMake 3.20+、C++17 编译器、Qt 6 Core/Gui/Widgets/Concurrent/Network。
Linux 还需要 Qt DBus；启用测试时需要 Qt Test。Windows 原生采集使用 C++20。
推荐使用 Ninja。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel
./build/mark-shot
```

只构建应用可使用 `cmake --build build --target mark-shot --parallel`。
完整构建还会编译本机依赖允许启用的插件与测试。

FFmpeg、PipeWire、Wayland、LayerShellQt、ZXing、ONNX Runtime 和 PulseAudio 使用原项目的可选依赖探测。
缺少可选库时，对应能力不可用；CMake 会明确输出启用结果。开发环境可通过
`PATH="$PWD/qt/scripts:$PATH" ./build/mark-shot` 使用本仓库的辅助脚本。
构建产物中的 provider 插件统一位于可执行文件旁的 `plugins/` 目录。

录制编解码器独立放在 `recording/`，首次开始录制时才加载，不能在发行包中遗漏。
Release 默认启用链接期优化、未引用段删除和冷路径体积优化。
Windows 裁剪依赖、签名后归档、内存测量及已验证结果见
[`docs/performance.md`](docs/performance.md)。

可执行文件、桌面入口和应用数据目录沿用 `mark-shot`，可直接替换原发行包；单实例通道使用独立的
`mark-shot-rs` 名称，避免启动请求被旧 C++ 进程接管。
未指定配置目录时保留原 Mark Shot 配置兼容逻辑；需要隔离配置时，设置绝对路径
`MARK_SHOT_CONFIG_DIR=/path/to/config`，该模式不会回退到用户目录。

```sh
# 打开图片进行标注，不进行屏幕采集
./build/mark-shot /absolute/path/to/image.png

# 无窗口列出显示器，或采集到文件
./build/mark-shot --list-displays
./build/mark-shot --capture-to /tmp/shot.png
```

仅构建和测试 Rust 核心不需要 Qt：

```sh
cargo build --locked -p shot-ffi
cargo test --locked --workspace
cargo clippy --workspace --all-targets -- -D warnings
cargo fmt --all -- --check
```

几何、图像和拼接 crate 不依赖外部库；配置与编辑模块使用 `serde`、`serde_json`、`tempfile` 和 `fs2`。
依赖版本由 `Cargo.lock` 固定，工作区内依赖方向单一。
CMake 在当前构建目录的 `cargo/` 中生成静态库，不会修改全局 Cargo 配置。
交叉编译需要显式设置与 C++ 编译器匹配的 `MARK_SHOT_RUST_TARGET`，并安装该 Rust 目标。
无网络的发行构建先执行 `scripts/packaging/vendor-rust-crates.sh` 生成 `vendor/`，再传入
`-DMARK_SHOT_CARGO_VENDOR_DIR=vendor`；构建和 `rust-*` 测试都改为离线读取该目录。

## 发行打包

工作流、`debian/`、`flake.nix` 与发布说明位于仓库根目录，其余打包清单位于 `qt/packaging/`。
各渠道均在 CI 中安装 Rust 1.85 以上的工具链；Debian 源码包和 Flatpak 无法联网下载 crate，
使用 `scripts/packaging/` 生成只含 Linux 依赖的 vendor 组件，Nix 通过 `importCargoLock` 预取。

## 验证

```sh
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure \
  -R '^(rust-.*|.*config|config-value|app-config-defaults|startup-default-color|capture-geometry|line-constraint|selection-aspect-ratio|annotation-filters|stitcher.*|annotation-cursor|arrow-properties|settings-.*|runtime-contract|runtime-startup|source-module-limits|export-image-effect)$'
uv run --no-project python scripts/check_source_limits.py
```

Qt 测试经过真实的 C ABI 和 Rust 静态库，不使用模拟实现。
`rust-image-contract` 使用原 C++ 实现生成的固定摘要验证模糊输出，并逐个 alpha 值检查像素预乘语义。
`rust-state-contract` 使用原 Qt 默认配置和样式固定样本，并验证配置隔离、原子更新与未知字段保留。
`rust-editor-contract` 检查完整快照往返、滚轮合并和清空事务；`export-image-effect` 覆盖分数及整数 DPR、半透明内容和阴影分布。
`runtime-startup` 直接执行应用的 `--version` 和 `--help`，覆盖生产入口的 ABI 检查，而不是绕过 `main`。
GUI 测试采用 Qt offscreen 平台；这不等于对真实 Wayland、X11 或 Windows 桌面捕获的端到端验证。
CTest 为 Qt 测试设置独立的 HOME、XDG 和配置目录，测试文件写入 `build/test-homes/`。
新增文件写入测试即使直接执行，也会先创建临时配置目录；启动前的环境配置读取同样遵守 `MARK_SHOT_CONFIG_DIR`。

## 来源与许可证

- Qt 应用和迁移算法基于 `mark-shot` 提交 `a682fce199c731eb737e1b306c1f6dbe45987319`，版本 0.1.58。
- 架构参考 `snow-apps` 提交 `c6d3975d5d1bc2881a9c06170807cab9bddd581e` 的 Rust 工作区、单一静态库和 C ABI 边界；未复制其应用代码。
- 本项目沿用 Mark Shot 的 MIT 许可证与版权声明，见 `LICENSE`、`qt/LICENSE`。
