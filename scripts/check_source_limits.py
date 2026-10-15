#!/usr/bin/env python3
"""检查维护文件的行数上限，避免算法和界面重新聚合成巨大文件。"""

from __future__ import annotations

import argparse
import os
from pathlib import Path

EXCLUDED_DIRS = {".git", "target", "__pycache__", ".cache", "node_modules"}
SUFFIXES = {".rs", ".cpp", ".h", ".c", ".js", ".py", ".qml", ".cmake", ".md", ".toml"}


def source_files(root: Path):
    """遍历 root 下的维护文件并跳过构建产物，返回文件路径迭代器。"""
    for directory, names, files in os.walk(root):
        names[:] = [name for name in names if name not in EXCLUDED_DIRS and not name.startswith("build")]
        for name in files:
            path = Path(directory) / name
            if path.suffix in SUFFIXES or name == "CMakeLists.txt":
                yield path


def main() -> int:
    """读取可选仓库路径并检查所有维护文件，成功返回 0，存在大文件返回 1。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    root = parser.parse_args().root.resolve()
    if not root.is_dir():
        parser.error(f"Not a directory: {root}")
    checked = 0
    violations = []
    maximum = (0, "")
    for path in source_files(root):
        lines = len(path.read_text(encoding="utf-8").splitlines())
        checked += 1
        relative = str(path.relative_to(root))
        maximum = max(maximum, (lines, relative))
        if lines >= 750:
            violations.append((relative, lines))
    for path, lines in sorted(violations):
        print(f"{path}: {lines} lines; split before reaching 750 lines")
    print(f"Checked {checked} files; largest: {maximum[1]} ({maximum[0]} lines)")
    return int(bool(violations))


if __name__ == "__main__":
    raise SystemExit(main())
