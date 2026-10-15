#!/usr/bin/env python3
"""统计发行目录与压缩包体积，列出最大的文件并检查可选预算。"""

import argparse
import json
from pathlib import Path


def report(directory: Path, archive: Path | None) -> dict:
    """遍历 directory 并读取可选 archive 大小，返回总字节数和最大的二十个文件。"""
    files = sorted(((file.stat().st_size, file.relative_to(directory).as_posix())
                    for file in directory.rglob("*") if file.is_file()), reverse=True)
    return {
        "stage": str(directory),
        "file_count": len(files),
        "uncompressed_bytes": sum(size for size, _ in files),
        "archive_bytes": archive.stat().st_size if archive else None,
        "largest_files": [{"path": path, "bytes": size} for size, path in files[:20]],
    }


def main() -> int:
    """输出体积 JSON；目录无效或压缩包超过指定预算时返回非零。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", type=Path)
    parser.add_argument("--archive", type=Path)
    parser.add_argument("--max-archive-mib", type=float)
    args = parser.parse_args()
    if not args.stage.is_dir() or (args.archive and not args.archive.is_file()):
        parser.error("Provide an existing stage directory and archive")
    if args.max_archive_mib is not None and (args.max_archive_mib <= 0 or not args.archive):
        parser.error("An archive and positive budget are required")
    result = report(args.stage.resolve(), args.archive)
    result["passed"] = (args.max_archive_mib is None
                        or result["archive_bytes"] <= args.max_archive_mib * 1024 * 1024)
    print(json.dumps(result, ensure_ascii=True, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
