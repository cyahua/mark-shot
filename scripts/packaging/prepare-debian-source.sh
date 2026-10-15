#!/usr/bin/env bash
# 【发行打包】【Debian源码】准备 3.0 (quilt) 源码包输入：上游 orig、Rust vendor 组件包与 debian/ 目录。
#
# 用法:
#   scripts/packaging/prepare-debian-source.sh --version VER --output-dir DIR \
#       (--git-ref REF | --orig-tarball FILE)
#
# 输出:
#   DIR/mark-shot_VER.orig.tar.gz         上游源码，PPA 使用 GitHub 标签归档
#   DIR/mark-shot_VER.orig-vendor.tar.xz  Launchpad 离线构建所需的 Rust crate
#   DIR/mark-shot-VER/                    已加入 vendor/ 与 debian/ 的源码树
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPOSITORY_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION=""
OUTPUT_DIR=""
GIT_REF=""
ORIG_INPUT=""

usage() {
    cat <<'EOF'
Usage: prepare-debian-source.sh --version VER --output-dir DIR (--git-ref REF | --orig-tarball FILE)

  --version VER        上游版本号，不带 v 前缀
  --output-dir DIR     生成 orig 包与源码树的目录
  --git-ref REF        从本仓库的指定提交生成 orig 包，用于持续集成
  --orig-tarball FILE  使用已有上游归档作为 orig 包，用于 PPA 发布
EOF
}

# 校验选项值存在且非空，失败时输出选项名并退出
require_value() {
    if [[ $# -lt 2 || -z "$2" || "$2" == --* ]]; then
        echo "missing value for $1" >&2
        exit 1
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) require_value "$@"; VERSION="$2"; shift 2 ;;
        --output-dir) require_value "$@"; OUTPUT_DIR="$2"; shift 2 ;;
        --git-ref) require_value "$@"; GIT_REF="$2"; shift 2 ;;
        --orig-tarball) require_value "$@"; ORIG_INPUT="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if [[ -z "$VERSION" || -z "$OUTPUT_DIR" ]] || [[ -n "$GIT_REF" && -n "$ORIG_INPUT" ]] \
    || [[ -z "$GIT_REF" && -z "$ORIG_INPUT" ]]; then
    usage >&2
    exit 1
fi
if [[ ! "$VERSION" =~ ^[0-9][0-9A-Za-z.+~]*$ ]]; then
    echo "invalid upstream version: $VERSION" >&2
    exit 1
fi

OUTPUT_DIR="$(realpath -m "$OUTPUT_DIR")"
ORIG_TAR="$OUTPUT_DIR/mark-shot_${VERSION}.orig.tar.gz"
VENDOR_TAR="$OUTPUT_DIR/mark-shot_${VERSION}.orig-vendor.tar.xz"
SOURCE_ROOT="$OUTPUT_DIR/mark-shot-${VERSION}"
mkdir -p "$OUTPUT_DIR"
if [[ -e "$SOURCE_ROOT" ]]; then
    echo "source tree already exists: $SOURCE_ROOT" >&2
    exit 1
fi

# 1. 生成或接收上游 orig 包；PPA 必须使用官方标签归档原文件
if [[ -n "$GIT_REF" ]]; then
    git -C "$REPOSITORY_ROOT" archive --format=tar.gz --prefix="mark-shot-${VERSION}/" \
        --output="$ORIG_TAR" "$GIT_REF"
elif [[ "$(realpath "$ORIG_INPUT")" != "$ORIG_TAR" ]]; then
    cp "$ORIG_INPUT" "$ORIG_TAR"
fi

# 2. 解包上游源码，在源码树内生成 vendor/ 并打成组件包
mkdir -p "$SOURCE_ROOT"
tar -xzf "$ORIG_TAR" --strip-components=1 -C "$SOURCE_ROOT"
bash "$SCRIPT_DIR/vendor-rust-crates.sh" --source-dir "$SOURCE_ROOT" --archive "$VENDOR_TAR" >/dev/null

# 3. 加入打包目录，仅 debian/ 不属于上游归档
cp -a "$REPOSITORY_ROOT/debian" "$SOURCE_ROOT/"

echo "orig=$ORIG_TAR"
echo "vendor=$VENDOR_TAR"
echo "source_root=$SOURCE_ROOT"
