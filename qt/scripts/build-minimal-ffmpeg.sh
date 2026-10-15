#!/usr/bin/env bash
# Build a shared FFmpeg that contains only the components used by mark-shot recording.
#
# 发行版/MSYS2 的 FFmpeg 启用了几十个外部编解码库（x265、aom、svt-av1、dav1d、
# librsvg、libplacebo、gnutls 等），Windows 打包时这些 DLL 会占到安装包的一半以上，
# 启动时也会全部映射进进程。录制只需要下列组件：
#   编码器: libx264, mpeg4, aac, gif, h264_{mf,nvenc,amf,qsv} / h264_{vaapi,nvenc,qsv}
#   容器:   mp4, ipod(m4a，与 mp4 共用 movenc), matroska, gif
#   滤镜:   split, palettegen, paletteuse, scale, format（buffer/buffersink 始终内置）
#
# MSYS2 UCRT64 依赖:
#   mingw-w64-ucrt-x86_64-{gcc,nasm,pkgconf,x264,ffnvcodec-headers,amf-headers,libvpl} make
# 构建后通过 -DMARK_SHOT_FFMPEG_ROOT=<prefix> 强制使用该 FFmpeg。
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: build-minimal-ffmpeg.sh --prefix DIR [--version VER] [--work-dir DIR] [--jobs N]

Options:
  --prefix DIR     Install prefix of the trimmed FFmpeg (required)
  --version VER    FFmpeg release version, default 8.0.1
  --work-dir DIR   Download and build directory, default <prefix>/../ffmpeg-build
  --jobs N         Parallel make jobs, default: nproc
EOF
}

PREFIX=""
VERSION="8.0.1"
WORK_DIR=""
JOBS="$(nproc 2>/dev/null || echo 4)"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix) PREFIX="$2"; shift 2 ;;
        --version) VERSION="$2"; shift 2 ;;
        --work-dir) WORK_DIR="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if [[ -z "$PREFIX" ]]; then
    usage >&2
    exit 1
fi
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+(\.[0-9]+)?$ || ! "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo "version must be a release number and jobs must be a positive integer" >&2
    exit 1
fi
case "$(uname -m)" in
    x86_64|amd64|i?86)
        if ! command -v nasm >/dev/null 2>&1; then
            echo "nasm is required: refusing to disable SIMD in a performance build" >&2
            exit 1
        fi
        ;;
esac
mkdir -p "$PREFIX"
PREFIX="$(cd "$PREFIX" && pwd)"
WORK_DIR="${WORK_DIR:-$(dirname "$PREFIX")/ffmpeg-build}"
mkdir -p "$WORK_DIR"

# 1. 下载并解压源码，已存在时复用
archive="$WORK_DIR/ffmpeg-$VERSION.tar.xz"
source_dir="$WORK_DIR/ffmpeg-$VERSION"
if [[ ! -f "$archive" ]]; then
    curl -fL --retry 3 -o "$archive.part" "https://ffmpeg.org/releases/ffmpeg-$VERSION.tar.xz"
    mv "$archive.part" "$archive"
fi
if [[ ! -d "$source_dir" ]]; then
    tar -xf "$archive" -C "$WORK_DIR"
fi

# 2. 公共配置：关闭全部组件与自动探测，只打开录制用到的部分
args=(
    --prefix="$PREFIX"
    --enable-shared --disable-static
    --enable-gpl --enable-version3
    --disable-autodetect
    --disable-programs --disable-doc --disable-debug --disable-network
    --disable-everything
    --disable-avdevice
    --enable-avcodec --enable-avformat --enable-avfilter
    --enable-avutil --enable-swresample --enable-swscale
    --enable-libx264
    "--enable-encoder=libx264,mpeg4,aac,gif"
    "--enable-muxer=mp4,ipod,matroska,gif"
    --enable-protocol=file
    "--enable-filter=split,palettegen,paletteuse,scale,format,null"
)

# 3. 按平台打开硬件编码；头文件或库缺失时跳过对应编码器
has_pkg() { pkg-config --exists "$1" 2>/dev/null; }
hw_encoders=()
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        args+=(--enable-w32threads --enable-mediafoundation --enable-d3d11va --enable-dxva2)
        hw_encoders+=(h264_mf)
        if has_pkg ffnvcodec; then
            args+=(--enable-ffnvcodec --enable-nvenc)
            hw_encoders+=(h264_nvenc)
        fi
        if [[ -n "${MINGW_PREFIX:-}" && -f "$MINGW_PREFIX/include/AMF/core/Factory.h" ]]; then
            args+=(--enable-amf)
            hw_encoders+=(h264_amf)
        fi
        ;;
    *)
        args+=(--enable-pthreads)
        if has_pkg libva && has_pkg libva-drm; then
            args+=(--enable-vaapi)
            hw_encoders+=(h264_vaapi)
        fi
        if has_pkg ffnvcodec; then
            args+=(--enable-ffnvcodec --enable-nvenc)
            hw_encoders+=(h264_nvenc)
        fi
        ;;
esac
if has_pkg vpl; then
    args+=(--enable-libvpl)
    hw_encoders+=(h264_qsv)
fi
if ((${#hw_encoders[@]})); then
    args+=("--enable-encoder=$(IFS=,; echo "${hw_encoders[*]}")")
fi
# 4. 配置、编译并安装，去掉动态库中的符号表
build_dir="$WORK_DIR/build-$VERSION"
mkdir -p "$build_dir"
(
    cd "$build_dir"
    "$source_dir/configure" "${args[@]}"
    make -j"$JOBS"
    make install
)
find "$PREFIX" -type f \( -name '*.dll' -o -name '*.so*' \) -exec strip --strip-unneeded {} +

# 5. 【打包】【来源记录】保留编译组件、配置、许可证和源码摘要，便于核对发行依赖
metadata="$PREFIX/share/mark-shot-ffmpeg"
mkdir -p "$metadata"
cp "$source_dir/COPYING.GPLv3" "$metadata/LICENSE.GPLv3"
cp "$build_dir/config_components.h" "$metadata/config_components.h"
{
    printf 'Source: https://ffmpeg.org/releases/ffmpeg-%s.tar.xz\n' "$VERSION"
    sha256sum "$archive"
    printf 'Configure:'
    printf ' %q' "${args[@]}"
    printf '\n'
} > "$metadata/build-info.txt"

echo "FFmpeg installed to $PREFIX (hardware encoders: ${hw_encoders[*]:-none})"
