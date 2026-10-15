#!/usr/bin/env bash
# 【发行打包】【Rust依赖】为离线构建生成只含 Linux 依赖的 vendor 目录，可选输出可复现的压缩包。
#
# 用法:
#   scripts/packaging/vendor-rust-crates.sh [--source-dir DIR] [--archive FILE]
#
# 依赖 cargo-vendor-filterer，CI 中以固定版本安装:
#   cargo install --locked cargo-vendor-filterer@0.5.18
# 生成的 DIR/vendor 通过 -DMARK_SHOT_CARGO_VENDOR_DIR=vendor 交给 CMake 使用。
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCHIVE=""

usage() {
    cat <<'EOF'
Usage: vendor-rust-crates.sh [--source-dir DIR] [--archive FILE]

  --source-dir DIR  Cargo 工作区根目录，默认是本仓库根目录
  --archive FILE    额外生成 FILE（.tar.xz），包内顶层目录为 vendor/
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
        --source-dir) require_value "$@"; SOURCE_DIR="$2"; shift 2 ;;
        --archive) require_value "$@"; ARCHIVE="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

SOURCE_DIR="$(realpath "$SOURCE_DIR")"
if [[ ! -f "$SOURCE_DIR/Cargo.toml" || ! -f "$SOURCE_DIR/Cargo.lock" ]]; then
    echo "Cargo workspace not found: $SOURCE_DIR" >&2
    exit 1
fi
if ! cargo vendor-filterer --version >/dev/null 2>&1; then
    echo "cargo-vendor-filterer is required: cargo install --locked cargo-vendor-filterer@0.5.18" >&2
    exit 1
fi

# 1. 只保留 Linux 目标需要的 crate，Windows 预编译导入库替换为空壳，避免进入源码包
VENDOR_DIR="$SOURCE_DIR/vendor"
LOCK_DIGEST="$(sha256sum "$SOURCE_DIR/Cargo.lock")"
rm -rf "$VENDOR_DIR"
VENDOR_LOG="$(mktemp)"
trap 'rm -f "$VENDOR_LOG"' EXIT
if ! (cd "$SOURCE_DIR" && cargo vendor-filterer --platform='*-unknown-linux-gnu' --versioned-dirs vendor) \
    >"$VENDOR_LOG" 2>&1; then
    cat "$VENDOR_LOG" >&2
    exit 1
fi
if [[ "$(sha256sum "$SOURCE_DIR/Cargo.lock")" != "$LOCK_DIGEST" ]]; then
    echo "Cargo.lock changed while vendoring; commit an updated lock file first" >&2
    exit 1
fi
if find "$VENDOR_DIR" -type f \( -name '*.a' -o -name '*.lib' -o -name '*.dll' -o -name '*.so' \) -print -quit | grep -q .; then
    echo "Vendored crates still contain prebuilt libraries" >&2
    exit 1
fi

# 2. 按固定顺序、时间戳与属主打包，同一锁文件重复生成时字节一致
if [[ -n "$ARCHIVE" ]]; then
    ARCHIVE="$(realpath -m "$ARCHIVE")"
    mkdir -p "$(dirname "$ARCHIVE")"
    # 解包后的上游源码保留归档内时间戳，未设置 SOURCE_DATE_EPOCH 时以 Cargo.lock 为准
    MTIME="@${SOURCE_DATE_EPOCH:-$(stat -c %Y "$SOURCE_DIR/Cargo.lock")}"
    tar -C "$SOURCE_DIR" --sort=name --mtime="$MTIME" --owner=0 --group=0 --numeric-owner \
        --pax-option=exthdr.name=%d/PaxHeaders/%f,delete=atime,delete=ctime \
        -cf - vendor | xz -T1 -9 > "$ARCHIVE"
    echo "archive=$ARCHIVE"
fi
echo "vendor=$VENDOR_DIR"
