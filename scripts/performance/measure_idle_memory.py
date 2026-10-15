#!/usr/bin/env python3
"""测量 Linux Release 托盘进程，区分常驻、分摊和私有内存。"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

MEDIA_LIBRARIES = ("libavcodec", "libavformat", "libavutil", "libavfilter", "libswscale", "libswresample")


def sample_process(pid: int) -> dict:
    """读取 pid 对应进程的内存与模块，返回以 KiB 为单位的测量结果。"""
    process = Path("/proc") / str(pid)
    fields = {}
    for line in (process / "smaps_rollup").read_text().splitlines():
        parts = line.split()
        if parts[0] in {"Rss:", "Pss:", "Private_Clean:", "Private_Dirty:", "Swap:"}:
            fields[parts[0].rstrip(":")] = int(parts[1])
    mappings = [line.split(maxsplit=5) for line in (process / "maps").read_text().splitlines()]
    modules = sorted({fields[5] for fields in mappings
                      if len(fields) == 6 and fields[5].startswith("/")})
    return {
        "rss_kib": fields["Rss"],
        "pss_kib": fields["Pss"],
        "private_kib": fields["Private_Clean"] + fields["Private_Dirty"],
        "swap_kib": fields["Swap"],
        "mapped_file_count": len(modules),
        "media_modules": [path for path in modules if Path(path).name.startswith(MEDIA_LIBRARIES)],
    }


def measure(executable: Path, seconds: float) -> dict:
    """在隔离配置中启动 executable，等待 seconds 秒后测量并终止本次进程，返回报告。"""
    with tempfile.TemporaryDirectory(prefix="mark-shot-memory-") as directory:
        root = Path(directory)
        runtime = root / "runtime"
        runtime.mkdir(mode=0o700)
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_STYLE_OVERRIDE="Fusion",
                   HOME=str(root), MARK_SHOT_CONFIG_DIR=str(root / "config"),
                   XDG_CONFIG_HOME=str(root / "xdg"), XDG_RUNTIME_DIR=str(runtime))
        with (root / "startup.log").open("w+") as log:
            process = subprocess.Popen([str(executable), "--tray"], env=env,
                                       stdout=log, stderr=log)
            try:
                time.sleep(seconds)
                if process.poll() is not None:
                    log.seek(0)
                    raise RuntimeError(f"Process exited before sampling: {log.read()}")
                result = sample_process(process.pid)
                result.update(executable=str(executable), binary_bytes=executable.stat().st_size,
                              idle_seconds=seconds, platform="linux-offscreen")
                return result
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()


def main() -> int:
    """解析命令行并输出 JSON；超出内存预算或提前加载录制库时返回非零。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--seconds", type=float, default=2)
    parser.add_argument("--assert-no-media", action="store_true")
    parser.add_argument("--max-rss-mib", type=float)
    args = parser.parse_args()
    if not Path("/proc/self/smaps_rollup").is_file():
        parser.error("This harness requires Linux /proc; do not compare its RSS with Windows Private Bytes")
    if args.seconds <= 0 or not args.executable.is_file():
        parser.error("Provide an existing executable and a positive sampling delay")
    result = measure(args.executable.resolve(), args.seconds)
    result["passed"] = not (
        (args.assert_no_media and result["media_modules"])
        or (args.max_rss_mib is not None and result["rss_kib"] > args.max_rss_mib * 1024)
    )
    print(json.dumps(result, ensure_ascii=True, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
