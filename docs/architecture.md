# 架构与迁移边界

## 依赖方向

```text
Qt Widgets / 路径与平台适配 / 插件
                 |
             qt/bridge
                 |
              shot-ffi
                 |
                 +-- shot-geometry
                 +-- shot-image
                 +-- shot-stitch --> shot-image
                 +-- shot-config
                 +-- shot-editor --> shot-config
```

除 `shot-ffi` 外，所有 crate 均禁止 unsafe，不依赖 Qt。
`shot-geometry`、`shot-image`、`shot-stitch` 不读取环境，不访问文件系统或窗口服务。
`shot-config` 只接受调用者明确传入的绝对文件路径；`shot-editor` 的样式存储委托给它。
`shot-ffi` 是唯一使用 unsafe 的 crate，也是唯一交给 Qt 链接的静态库，避免重复链接 Rust 运行时。
没有路径依赖指向参考仓库，也没有 C++ 算法运行时回退。

## 模块职责

| 模块 | 单一职责 |
| --- | --- |
| `shot-geometry/types.rs` | 半开整数矩形、浮点坐标、屏幕布局值类型 |
| `shot-geometry/capture.rs` | 逻辑/物理坐标换算和混合 DPR 显示器裁剪 |
| `shot-geometry/selection.rs` | 选区创建、比例手柄约束、精确尺寸 |
| `shot-geometry/line.rs` | 水平、垂直与 45 度线段投影 |
| `shot-image/buffer.rs` | 尺寸和步长检查、借用视图、不可变共享图像、分块转置 |
| `shot-image/filters.rs` | 直通色彩转换与精确预乘舍入 |
| `shot-image/blur.rs` | 在预乘空间中执行三次盒式模糊 |
| `shot-image/export/layout.rs` | 导出物理像素尺寸、边距、溢出及分配上限检查 |
| `shot-image/export/mask.rs` | 圆角抗锯齿与捕获窗口边缘内缩 |
| `shot-image/export/shadow.rs` | 16 位透明度模糊、阴影偏移与最终画布裁切 |
| `shot-image/export/pixels.rs` | 预乘通道缩放及一次 SourceOver 合成 |
| `shot-config/defaults.rs`、`export.rs` | 平台上下文默认值与导出配置兼容解析 |
| `shot-config/validation.rs` | 通用值转换、键名规范化与嵌套键更新 |
| `shot-config/atomic.rs`、`store.rs` | 显式路径读取、同目录文件锁与原子替换 |
| `shot-editor/annotation.rs` | 完整标注快照、协议版本和内存校验 |
| `shot-editor/history.rs` | 撤销重做、分支截断、容量限制及滚轮合并事务 |
| `shot-editor/colors.rs`、`style.rs` | Qt 颜色格式与旧版工具样式兼容 |
| `shot-editor/state.rs` | 工具样式的缺失回退与显式路径保存 |
| `shot-stitch/profile.rs` | 八个采样带的行签名及像素行比较 |
| `shot-stitch/comparison.rs` | 纹理判断、固定带排除、稳健重叠评分 |
| `shot-stitch/fixed_regions.rs` | 固定头部/底部检测与稳定估计 |
| `shot-stitch/motion_mask.rs` | 固定侧栏排除与运动带权重 |
| `shot-stitch/adjacent.rs` | 预测位移精搜、粗搜和歧义补查 |
| `shot-stitch/matching.rs` | 长图内部定位和边缘恢复 |
| `shot-stitch/line_matching.rs` | 像素连续行恢复和固定栏裁剪候选 |
| `shot-stitch/long_image.rs` | 预留空间、增量追加/前插及签名同步 |
| `shot-stitch/session.rs` | 会话生命周期、方向锁定和输出缓存 |
| `shot-stitch/algorithm.rs` | 单帧匹配、校验与提交的业务流程 |
| `shot-ffi/{geometry,image,stitch,fixed_regions,export}.rs` | 几何和图像 ABI 适配，不重新实现算法 |
| `shot-ffi/{config,editor_history,annotation_state}.rs` | 配置和编辑状态 ABI、输出前置校验与事务边界 |
| `shot-ffi/bytes.rs` | 同步输入字节视图与不可变输出句柄的生命周期 |
| `shot-ffi/boundary.rs` | 指针基础检查、错误码与 panic 隔离 |

## Qt 保留部分

Qt 窗口、工具栏、主题、翻译资源、标注绘制与手势交互保留原实现。
屏幕捕获的 Wayland/PipeWire/Portal/X11/Windows 后端、剪贴板、窗口管理、录制编码和网络插件保持原生实现。
当前画布上的 Qt 绘制数据、文本排版、绘制路径、手势和平台相关配置映射仍在 C++；不能将整个应用称为纯 Rust 应用。
历史栈与待合并事务不再保存在 Qt 中。Qt 只建立当前快照、转换 Rust 返回值并刷新画布。

对应算法源文件已改成薄适配器：

| Qt 入口 | Rust 实现 |
| --- | --- |
| `src/capture_geometry.cpp` | `shot-geometry/capture.rs` |
| `src/selection_aspect/selection_aspect_ratio.cpp` | `shot-geometry/selection.rs` |
| `src/shot_window_line_constraint.cpp` | `shot-geometry/line.rs` |
| `src/annotation_filters/image_filter_effects.cpp` | `shot-image/filters.rs`、`blur.rs` |
| `src/scroll/stitcher.cpp` | `shot-stitch` 完整管线 |
| `src/scroll/stitcher_fixed_regions.cpp` | `shot-stitch/fixed_regions.rs` |
| `src/app_config_defaults.cpp`、`app_config_store.cpp`、`config_value.cpp` | `shot-config` 默认值、通用校验和存储 |
| `src/annotation_state_store.cpp` | `shot-editor/state.rs`、`style.rs` |
| `bridge/editor_history.cpp` | `shot-editor/history.rs` |
| `src/export_image_effect.cpp` | `shot-config/export.rs`、`shot-image/export` |

原 C++ 拼接器的 matching、algorithm、frame_profile、motion_mask、long_image 和 internal 实现已经删除。
固定带检测器的 Qt 类只为兼容接口和回归测试而保留，实际检测也调用 Rust。

窗口控件已组合为 `ShotWindowControls`，不通过继承扩大窗口类；动作名称独立为 `ui/action_names.cpp`。
GNOME 扩展的 D-Bus 声明和窗口遮挡计算分别拆到 `dbus-interface.js`、`window-occlusion.js`。
所有维护的源文件须少于 750 行，检查脚本会阻止继续扩大文件。

## 像素与内存约定

- C ABI 使用 `0xAARRGGBB` 的原生整数像素，不把 RGBA 字节顺序假定为主机字节顺序。
- `ShotImageView.len` 和 `stride` 的单位为 `uint32_t` 像素；视图可包含行填充。
- 模糊、拼接使用预乘 ARGB；灰度、反色和提亮使用直通输入并输出预乘结果。
- 输入像素只在同步调用期间借用。拼接器复制需要保留的图像，绝不保留 Qt 内存指针。
- 图像输出写入 Qt 分配的缓冲区。JSON 和 UTF-8 输出通过不可变 `ShotBytes` 句柄返回，Qt 复制后立即释放。
- Rust 分配的句柄只交还对应 destroy 函数，C++ 通过 `unique_ptr` 自定义删除器管理；字节视图在所属句柄释放后失效。
- 单个像素缓冲区最多 128 Mi 像素；行签名也计入拼接内存预算。维度乘法、步长、容量和可恢复分配失败均显式检查。
- 同一句柄只能由一个线程独占访问；不同句柄可以用于不同线程。Qt 适配器沿用原会话线程模型，不承诺内部并发。
- 长图按 1.5 倍空间预留；余量足够时只复制新增像素，扩容时才搬移旧缓冲。Qt 预览按修订号和滚动轴缓存。

FFI 无法判断一个非空地址是否已释放或是否真的拥有声明长度。宿主仍须满足头文件中的有效内存和生命周期契约。
Rust 的 panic 使用 unwind 并在边界转换为状态码；这不捕获进程终止、分配器 abort 或宿主未定义行为。
C 头文件按领域显式维护，当前 ABI 版本为 2；改变布局或协议含义时必须更新版本，并同步运行 Rust 与 Qt 契约测试。
Qt 启动入口和契约测试共用 `SHOT_ABI_VERSION` 常量；`runtime-startup` 通过真实应用进程验证入口，避免对象复用测试漏掉启动检查。

## 配置与编辑事务

配置读取接受 UTF-8 BOM，拒绝非对象根、非普通文件和超过 8 MiB 的 JSON，不自行创建备用文件。
写入使用同目录临时文件，保留已有权限，同步文件后原子替换；已有符号链接继续指向原目标。
同目录 `.lock` 文件协调 Rust 实例间的写入，局部更新从加锁到读取、修改、提交保持同一事务。
锁等待最多两秒，超时返回错误。默认配置只在目标缺失时创建，损坏文件不会被初始化流程覆盖。

标注快照包含全部绘制字段、多选状态、主选中编号以及下一个序号和标注编号。
JSON 解析启用 `float_roundtrip`，避免非整像素坐标在撤销重做后出现双精度舍入偏移。
单个快照限制为 32 MiB，历史总预算为 128 MiB，默认最多保留 100 个撤销目标；内存统计包含向量预留容量。
内存收缩优先移除较远记录，保留最近的撤销和重做候选，避免刚重做的操作无法撤销。
滚轮首次修改记录快照，同一上下文的后续事件无需再次序列化；上下文切换、计时器到期或撤销操作提交事务。
清空画布会同时丢弃挂起事务，迟到的计时器不能恢复旧文档。非法输入和输出编码失败不会消耗历史记录。
工具样式兼容旧笔宽字段、枚举边界、标准颜色名称和十六进制格式，保存继续使用 `#AARRGGBB`。

## 导出效果

导出按物理像素计算尺寸，DPR 只作为输出元数据保留；同时保留色彩空间与分辨率信息。
圆角继续剔除两像素捕获边缘，阴影先模糊后偏移和裁切，内容只进行一次预乘合成。
这修正了旧实现中高 DPI 内容错位、缩放，以及半透明内容被重复绘制的问题。

阴影采用三次分离盒式模糊近似高斯分布，不依赖 Qt 私有图形效果实现，也不保证与其逐像素相同。
原有阴影主方向、边缘软化、圆角和内容保留测试继续执行，新增测试覆盖整数及分数 DPR、透明输入和半透明内容。

## 运行与测试隔离

`mark-shot-rs` 使用独立的 IPC 服务名，不会把请求交给原 C++ 常驻进程。
应用默认保留旧配置兼容逻辑；`MARK_SHOT_CONFIG_DIR` 指定目录后禁止任何 HOME 回退。
测试配置为独立目录，CTest 同时隔离 HOME、XDG、Windows 应用数据路径和运行目录。
箭头属性测试直接执行时也自行设置配置目录覆盖，避免 Qt 的通用配置查找回到真实用户目录。
开发构建将 provider 插件放在应用旁的 `plugins/` 中；运行契约测试验证真实插件搜索与加载。

## 拼接行为

默认阈值沿用原项目：最小重叠 100、接受误差 9、最小追加 15、提前结束误差 1。
横向会话通过转置使用同一条纵向管线；只有首次方向性增长之后才锁定滚动轴。
回看已知内容只移动锚点，不追加重复像素。小于最小追加量的滚动保留原锚点，以便累计位移。
固定头尾与固定侧栏使用独立检测器；无纹理重叠不作为可靠位移证据。

迁移同时修正了预测偏移序列在靠近边缘时未覆盖完整搜索区间的问题，以及已知位置恢复后位移预测未同步的问题。
非法尺寸、非有限比例和无效内存布局会被拒绝，不继承原 C++ 未定义行为。

## 验证与平台范围

Rust 测试不加载 Qt，覆盖算法边界、长图像素、会话隔离、输出所有权和错误契约。
原 Qt 回归测试直接链接新静态库，因此也覆盖 Qt 与 Rust 的布局和数值转换。
`qt/tests/fixtures/image_filter_golden.json` 来自未修改的原项目滤镜实现；摘要按大端 ARGB 字节序计算，避免依赖主机端序。
`app_config_golden.json` 和 `annotation_state_golden.json` 来自迁移前的 Qt 配置实现，覆盖完整默认对象和样式兼容行为。
配置与状态契约测试在创建 QApplication 前隔离 HOME 和配置目录，也验证启动前的配置查找不会越过显式目录。

Linux 构建和 offscreen GUI 测试不代表真实桌面抓屏、屏幕共享授权或录制完整流程已通过验证。
Windows 后端保留源码和构建条件，但仍需要在 Windows 上执行原生构建与验证。
