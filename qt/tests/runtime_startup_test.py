#!/usr/bin/env python3
"""验证实际应用入口能通过 ABI 检查并处理命令行参数，全程隔离用户配置。"""

from __future__ import annotations

import argparse
import os
import subprocess
import tempfile
from pathlib import Path


def isolated_environment(home: Path) -> dict[str, str]:
    """在 home 创建隔离目录，返回移除应用环境覆盖后的子进程环境。"""
    environment = {
        key: value for key, value in os.environ.items() if not key.startswith("MARK_SHOT_")
    }
    environment.update(
        HOME=str(home),
        USERPROFILE=str(home),
        QT_QPA_PLATFORM="offscreen",
        QT_STYLE_OVERRIDE="Fusion",
        XDG_SESSION_TYPE="x11",
    )
    for name in (
        "MARK_SHOT_CONFIG_DIR", "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME",
        "XDG_RUNTIME_DIR", "APPDATA", "LOCALAPPDATA",
    ):
        directory = home / name
        directory.mkdir(mode=0o700)
        environment[name] = str(directory)
    return environment


def invoke(binary: Path, argument: str, home: Path, environment: dict[str, str]) -> str:
    """用 environment 在 home 执行 binary/argument，返回输出；非零退出视为启动失败。"""
    result = subprocess.run(
        [str(binary), argument], cwd=home, env=environment, encoding="utf-8", errors="replace",
        capture_output=True, timeout=20, check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"{argument}: exit={result.returncode}; stdout={result.stdout!r}; stderr={result.stderr!r}"
        )
    return result.stdout.strip()


def main() -> int:
    """读取应用路径并验证版本与帮助入口，成功返回 0，失败抛出明确错误。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    binary = parser.parse_args().binary.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="mark-shot-startup-") as temporary:
        home = Path(temporary)
        environment = isolated_environment(home)
        version = invoke(binary, "--version", home, environment)
        if not version.startswith("mark-shot "):
            raise RuntimeError(f"unexpected application version output: {version!r}")
        help_text = invoke(binary, "--help", home, environment)
        for option in ("--help", "--version", "--capture-to", "--list-displays"):
            if option not in help_text:
                raise RuntimeError(f"help output is missing {option}")
        print(f"Startup verified: {version}; --help contains the expected options")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
