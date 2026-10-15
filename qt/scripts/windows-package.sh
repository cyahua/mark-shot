#!/usr/bin/env bash
# Stage and archive the Windows release from a configured MSYS2 UCRT64 build tree.
#
# Typical CI flow:
#   qt/scripts/build-minimal-ffmpeg.sh --prefix "$PWD/ffmpeg-min"
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
#         -DMARK_SHOT_FFMPEG_ROOT="$PWD/ffmpeg-min" -DMARK_SHOT_REQUIRE_FFMPEG=ON \
#         -DMARK_SHOT_WITH_LAYER_SHELL=OFF
#   cmake --build build
#   MARK_SHOT_FFMPEG_PREFIX="$PWD/ffmpeg-min" qt/scripts/windows-package.sh \
#         --build-dir build --stage-dir app
#   (sign app/ here)
#   qt/scripts/windows-package.sh --stage-dir app --archive mark-shot-windows-x86_64.zip --archive-only
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: windows-package.sh --stage-dir DIR [--build-dir DIR] [--archive FILE] [--archive-only]

Options:
  --build-dir DIR   CMake build tree to install from (required unless --archive-only)
  --stage-dir DIR   Directory that receives the stripped install tree and runtime DLLs
  --archive FILE    Write a .zip (deflate level 9) or .7z (LZMA2) archive of the stage dir
  --archive-only    Skip install/deploy, only archive an already staged (and signed) dir
EOF
}

BUILD_DIR=""
STAGE_DIR=""
ARCHIVE=""
ARCHIVE_ONLY=0
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 校验参数存在且非空，失败时输出选项名并返回非零
require_value() {
    if [[ $# -lt 2 || -z "$2" || "$2" == --* ]]; then
        echo "missing value for $1" >&2
        exit 1
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) require_value "$@"; BUILD_DIR="$2"; shift 2 ;;
        --stage-dir) require_value "$@"; STAGE_DIR="$2"; shift 2 ;;
        --archive) require_value "$@"; ARCHIVE="$2"; shift 2 ;;
        --archive-only) ARCHIVE_ONLY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if [[ -z "$STAGE_DIR" ]] || { [[ $ARCHIVE_ONLY -eq 0 ]] && [[ -z "$BUILD_DIR" ]]; }; then
    usage >&2
    exit 1
fi

STAGE_DIR="$(realpath -m "$STAGE_DIR")"
archive_path=""
if [[ -n "$ARCHIVE" ]]; then
    case "$ARCHIVE" in
        *.7z|*.zip) ;;
        *) echo "unsupported archive type: $ARCHIVE" >&2; exit 1 ;;
    esac
    archive_path="$(realpath -m "$ARCHIVE")"
    if [[ "$archive_path" == "$STAGE_DIR/"* ]]; then
        echo "archive must be outside the stage directory" >&2
        exit 1
    fi
fi

if [[ $ARCHIVE_ONLY -eq 0 ]]; then
    BUILD_DIR="$(realpath -m "$BUILD_DIR")"
    source_root="$(realpath "$script_dir/../..")"
    for protected in "$source_root" "$BUILD_DIR" "${HOME:-/}"; do
        if [[ "$protected" == "$STAGE_DIR" || "$protected" == "$STAGE_DIR/"* || "$STAGE_DIR" == / ]]; then
            echo "unsafe stage directory: $STAGE_DIR" >&2
            exit 1
        fi
    done
    if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
        echo "CMake build tree not found: $BUILD_DIR" >&2
        exit 1
    fi
    if ! grep -Eq '^CMAKE_BUILD_TYPE:STRING=(Release|MinSizeRel)$|^CMAKE_CONFIGURATION_TYPES:[^=]+=.*Release' "$BUILD_DIR/CMakeCache.txt"; then
        echo "Windows packages require a Release or MinSizeRel build" >&2
        exit 1
    fi
    if [[ -d "$STAGE_DIR" && -n "$(find "$STAGE_DIR" -mindepth 1 -maxdepth 1 -print -quit)" && ! -f "$STAGE_DIR/.mark-shot-package" ]]; then
        echo "refusing to replace an unowned stage directory: $STAGE_DIR" >&2
        exit 1
    fi
    # 1. 安装时去掉可执行文件与插件的符号表
    rm -rf "$STAGE_DIR"
    mkdir -p "$STAGE_DIR"
    printf 'mark-shot package staging\n' > "$STAGE_DIR/.mark-shot-package"
    cmake --install "$BUILD_DIR" --config Release --prefix "$STAGE_DIR" --strip

    # 2. 复制 Qt 与原生依赖，再统一去掉符号表；MSYS2 预编译库已剥离时 strip 为空操作
    bash "$script_dir/windows-deploy-runtime.sh" "$STAGE_DIR/bin" "$STAGE_DIR/bin/mark-shot-rs.exe"
    find "$STAGE_DIR" -type f \( -iname '*.dll' -o -iname '*.exe' \) -exec strip --strip-unneeded {} +
    if [[ -n "${MARK_SHOT_FFMPEG_PREFIX:-}" ]]; then
        metadata="$MARK_SHOT_FFMPEG_PREFIX/share/mark-shot-ffmpeg"
        if [[ ! -f "$metadata/LICENSE.GPLv3" || ! -f "$metadata/config_components.h" ]]; then
            echo "FFmpeg build metadata is missing; rebuild with build-minimal-ffmpeg.sh" >&2
            exit 1
        fi
        mkdir -p "$STAGE_DIR/share/licenses"
        cp -R "$metadata" "$STAGE_DIR/share/licenses/ffmpeg"
        if [[ -d "${MINGW_PREFIX:-/ucrt64}/share/licenses/x264" ]]; then
            cp -R "${MINGW_PREFIX:-/ucrt64}/share/licenses/x264" "$STAGE_DIR/share/licenses/x264"
        fi
    fi
fi

if [[ -n "$ARCHIVE" ]]; then
    if [[ ! -f "$STAGE_DIR/bin/mark-shot-rs.exe" ]]; then
        echo "staged executable not found: $STAGE_DIR/bin/mark-shot-rs.exe" >&2
        exit 1
    fi
    # 3. 归档：zip 使用最高压缩级别，7z 使用 LZMA2 固实压缩
    mkdir -p "$(dirname "$archive_path")"
    archive_work="$(mktemp -d "$(dirname "$archive_path")/.mark-shot-archive.XXXXXX")"
    trap 'rm -rf "$archive_work"' EXIT
    temporary_archive="$archive_work/$(basename "$archive_path")"
    case "$archive_path" in
        *.7z) (cd "$STAGE_DIR" && 7z a -t7z -m0=lzma2 -mx=9 -ms=on "$temporary_archive" . >/dev/null) ;;
        *.zip) (cd "$STAGE_DIR" && zip -9 -r -q "$temporary_archive" .) ;;
    esac
    mv -f "$temporary_archive" "$archive_path"
    ls -l "$archive_path"
fi
