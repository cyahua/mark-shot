# 录制内存优化

## 适用范围

本轮主要减少软件录制的编码工作缓冲，以及采集帧缓冲池的闲置容量。
没有缩小画面、降低帧率、改变质量档位、关闭音频或缩减录制队列。
Windows 工作集、原生桌面采集、硬件编码和完整 Windows 安装包未在本机实测。

基线是拆出按需录制后端之后、调整编码线程之前的产物；与
[空闲内存及发行体积](performance.md) 中的启动基线不同。

## 实现策略

### 软件编码并发

原实现将 `AVCodecContext::thread_count` 设为 `0`。本机有 16 个逻辑处理器，
x264 实际选择了 24 个帧编码线程和 8 个前瞻线程。编码器需要为在途帧保留
像素、参考帧和运动分析缓冲；这部分内存远大于宿主的 BGRA 队列。

`recording_memory_policy.cpp` 将常用快速预设的并发与帧尺寸关联：

- `ultrafast`，以及低于 48 帧/秒的 `veryfast`，采用有界并发
- 普通帧率使用 128 MiB、高帧率使用 256 MiB 的 BGRA 等效并发预算
- 默认最多 8 个编码线程，同时不超过可用逻辑处理器数量
- 16 个逻辑处理器下，均衡档 1080p 为 8 线程，4K/30 为 4 线程，4K/60 为 8 线程
- 高质量档和其他复杂预设保留编码器自动并发；硬件编码保持驱动默认值

这里的预算只用于选择并发数，不是整个编码器或整个应用的内存上限。
预设、CRF、码率前瞻、GOP、B 帧和像素格式均保持原值。

实测发现，直接限制高质量档的并发虽然能减少内存，但会产生实时供帧积压。
因此没有把这一策略强加给高质量录制。`sync-lookahead=0` 的独立实验收益较小，
并伴随吞吐量下降，也没有保留。

### 队列与帧缓冲

输入队列沿用 96 MiB 默认预算及原背压阈值：1080p/30 最多 4 帧，1080p/60 和
4K 最多 3 帧。没有通过缩短队列或主动丢帧制造内存改善。

BGRA 缓冲池仍最多持有 4 个槽位，优先复用最近使用且无人共享的槽位。
一个槽位连续 64 次获取都处于闲置状态后才释放，避免短时负载波动造成频繁分配。
仍由编码器、心跳或其他下游持有的快照不会被覆写。所需容量降至原容量的一半以下时，
重新分配小缓冲，避免 `QByteArray::resize()` 长期保留大帧容量。

缓冲池释放容量不等于操作系统立即降低驻留量；测量程序没有调用工作集回收或分配器整理。

## 测量方法

环境：2026-10-15，Linux x86_64，Qt 6.12.0，16 个逻辑处理器，Release。

`mark-shot-recording-memory-benchmark` 使用实际的 `RecordingAsyncWriter`、动态录制后端、
x264 和 BGRA 缓冲池。它按固定公式生成相同输入，遵守生产背压，并等待写出线程真正结束。
软件编码由测量程序显式启用，不受机器上可用硬件编码器影响。

测量不包含真实桌面捕获、界面、音频、显存帧或合成器开销；不能直接当作完整应用内存，
也不能与 Snow Shot 托盘空闲时的工作集数值比较。

- 连续提交 120 帧用于比较峰值和吞吐量，每种配置单独启动进程，前后交替测量三次
- 表格采用三次中位数；峰值取全部内存快照中当前驻留量和系统峰值的最大值
- `--realtime` 按目标帧率供帧，另检查 240 帧的提交延迟和输出完整性
- Linux 的 `private_bytes` 是私有驻留量，Windows 的同名字段是私有提交量，不可混用
- `pool_retained_bytes` 只统计缓冲池当前引用的分配容量，不代表进程内存

### 均衡档结果

| 场景 | 优化前峰值 | 优化后峰值 | 减少 | 连续供帧吞吐量（前 / 后） |
| --- | ---: | ---: | ---: | ---: |
| 1080p / 30 | 750.89 MiB | 371.22 MiB | 50.6% | 272.1 / 225.6 帧/秒 |
| 4K / 30 | 2255.49 MiB | 801.87 MiB | 64.4% | 87.7 / 51.2 帧/秒 |
| 1080p / 60 | 411.63 MiB | 201.77 MiB | 51.0% | 515.0 / 512.8 帧/秒 |
| 4K / 60 | 1242.93 MiB | 563.77 MiB | 54.6% | 161.9 / 150.0 帧/秒 |

减少帧级并发会降低最大吞吐量，不是所有性能指标都提高。
这些固定样本仍有目标帧率所需的余量，但不能据此承诺低端 CPU、复杂运动画面和真实采集均不丢帧。
高质量档没有采用上述并发限制，不能宣称它也获得相同幅度的内存下降。

### 帧数、画质与实时供帧

最终版本的六组实时供帧样本（上述四组均衡档，加上 4K/30、4K/60 高质量档）
各提交 240 帧，`late_frames` 和最大提交延迟均为 0。
这些是合成输入测试，不包含真实采集端和系统负载变化。

已对三次前后对比和六组实时供帧共 42 个 MP4 核对解码帧数、尺寸、帧时间间隔及逐包 PTS/DTS。
120 帧测试均输出 120 帧，240 帧测试均输出 240 帧。

按基准程序相同的 BGRA 公式重建原始输入，经 `SWS_POINT` 转为 YUV420P 后，
与解码输出比较 SSIM：

| 均衡档场景 | 优化前 SSIM | 优化后 SSIM |
| --- | ---: | ---: |
| 1080p / 30 | 0.999787 | 0.999794 |
| 4K / 30 | 0.999792 | 0.999790 |
| 1080p / 60 | 0.999484 | 0.999654 |
| 4K / 60 | 0.999623 | 0.999638 |

4K/30、4K/60 的高质量输出与基线 MP4 逐字节一致。
均衡档的线程数变化会改变编码决策，输出不保证逐字节一致；上述画质结果仅适用于固定样本。

核对同时发现旧版已有的容器元数据偏差：120 帧、30 帧/秒的视频，逐包时间戳覆盖完整
4 秒，`stream.duration` 却为 3.966667 秒，`avg_frame_rate` 为 3600/119。
前后版本均如此，解码帧数和 `r_frame_rate=30/1` 正常；本轮没有修改封装时长逻辑，
也没有把这个偏差作为内存或性能收益。

本机原始记录保存在 `build-release/recording-memory/final/`：
`summary.json`、`quality-summary.json`、`verified-streams.json` 和逐次 JSON、日志、MP4。
构建目录不提交到版本库。

## 复现与回归

```sh
cmake --build build-release --target \
  mark-shot-recording-memory-benchmark \
  mark-shot-recording-memory-policy-test \
  mark-shot-recording-encoder-memory-test \
  mark-shot-recording-bgra-buffer-pool-test --parallel 6

# 连续供帧：比较内存峰值和吞吐量
./build-release/mark-shot-recording-memory-benchmark \
  --width 3840 --height 2160 --fps 30 --frames 120 \
  --quality balanced --output /tmp/recording-memory.mp4

# 实时供帧：检查延迟，不把排队等待误认为实时录制能力
./build-release/mark-shot-recording-memory-benchmark \
  --width 3840 --height 2160 --fps 60 --frames 240 \
  --quality balanced --realtime --output /tmp/recording-realtime.mp4

# 只恢复编码器自动并发；仍保留新版缓冲池，用于单变量对比
MARK_SHOT_RECORDING_ENCODER_THREADS=0 \
  ./build-release/mark-shot-recording-memory-benchmark \
  --width 3840 --height 2160 --fps 30 --frames 120 \
  --quality balanced --output /tmp/recording-auto-threads.mp4

# 检查实际解码帧数、画面尺寸、帧率和时长
ffprobe -v error -select_streams v:0 -count_frames \
  -show_entries stream=width,height,r_frame_rate,avg_frame_rate,nb_read_frames,duration \
  -of json /tmp/recording-realtime.mp4

QT_QPA_PLATFORM=offscreen ctest --test-dir build-release --output-on-failure \
  -R '^(recording-.*|libav-.*|idle-memory-trimmer|runtime-idle-memory|runtime-startup)$'
```

`--max-peak-mib` 可设置本机诊断预算，超过预算时程序返回非零。不同平台、编码器版本和
链接配置会改变峰值，不应把某台机器的绝对数值直接设为所有环境的 CI 门槛。
`late_frames` 统计提交晚于下一帧截止时间的样本数；它不是编码器输出丢帧数。

策略测试覆盖分辨率、帧率、CPU 数、异常尺寸和环境覆盖。实际编码器测试检查线程配置、
高质量档自动并发，以及全部质量档在 30/60 帧下的原有参数。
缓冲池测试覆盖长期闲置回收、短时复用、持有快照保护和分辨率降低后的容量收缩。

## 环境覆盖

| 环境变量 | 行为 |
| --- | --- |
| `MARK_SHOT_RECORDING_ENCODER_THREADS` 未设置 | 使用经过验证的默认策略 |
| `MARK_SHOT_RECORDING_ENCODER_THREADS=0` | 软件编码器自行选择线程数 |
| `MARK_SHOT_RECORDING_ENCODER_THREADS=1..64` | 显式指定软件编码线程数，包括高质量档 |
| `MARK_SHOT_RECORDING_ENCODER_THREADS` 非法值 | 回退默认策略 |
| `MARK_SHOT_RECORDING_QUEUE_MIB` | 保留已有队列预算覆盖；默认 96 MiB |

显式减少线程可能降低实时编码能力。硬件编码不应用软件线程覆盖。
