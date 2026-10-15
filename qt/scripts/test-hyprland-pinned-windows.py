#!/usr/bin/env python3
"""在隔离 Hyprland 会话中比较真实鼠标位移和正式应用贴图位移。"""

import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
import zlib

from lib.hyprland_test_session import HyprlandTestSession, run_isolated
from lib.hyprland_pinned_window_checks import check_cross_output


def write_image(path):
    """向 path 写入 400×200 的确定性灰色 PNG，无返回值。"""
    def chunk(kind, data):
        """将 kind 和 data 编码为 PNG 数据块，返回字节串。"""
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    pixels = (b"\x00" + b"\x70\x70\x70" * 400) * 200
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", 400, 200, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(pixels)) + chunk(b"IEND", b""))


def move_pointer(session, start, delta, interval, step_size=5):
    """从 start 按 delta 移动 session 的指针，以 interval 毫秒和 step_size 像素分步，无返回值。"""
    dx, dy = delta
    steps = max(1, int(max(abs(dx), abs(dy)) / step_size))
    for step in range(1, steps + 1):
        session.send(f"move {start[0] + dx * step / steps} {start[1] + dy * step / steps}")
        time.sleep(interval / 1000)


def check_drag(session, delta, interval, step_size=5):
    """在 session 内以 interval 毫秒、step_size 像素连续拖动 delta，返回实际位移记录。"""
    before = session.layer()
    x = before["x"] + before["w"] * 0.35
    y = before["y"] + before["h"] * 0.45
    session.send(f"move {x} {y}")
    time.sleep(0.15)
    cursor_before = session.query("cursorpos")
    before = session.layer()
    session.send("down")
    time.sleep(0.05)
    move_pointer(session, (x, y), delta, interval, step_size)
    session.send("up")
    time.sleep(0.2)
    after = session.settled_layer()
    cursor_after = session.query("cursorpos")
    cursor_delta = [cursor_after[key] - cursor_before[key] for key in ("x", "y")]
    image_delta = [after[key] - before[key] for key in ("x", "y")]

    # 1. 【Hyprland测试】【拖动断言】先核实实际输入，再比较合成器读数，容许取整带来的误差
    passed = all(abs(actual - target) <= 2 for actual, target in zip(cursor_delta, delta))
    passed = passed and all(abs(a - b) <= 2 for a, b in zip(cursor_delta, image_delta))
    passed = passed and all(before[key] == after[key] for key in ("w", "h"))
    return {"name": f"drag-{delta[0]}-{delta[1]}-{interval}ms-step{step_size}",
            "target_delta": delta, "cursor_delta": cursor_delta, "image_delta": image_delta,
            "passed": passed, "before": before, "after": after}


def check_held_path(session, clipped=False):
    """在 session 保持同一次抓取并改变方向；clipped 启用左上裁切路径，返回逐段锚点检查。"""
    before = session.layer()
    start = (before["x"] + before["w"] * 0.35, before["y"] + before["h"] * 0.45)
    session.send(f"move {start[0]} {start[1]}")
    time.sleep(0.15)
    cursor_before = session.query("cursorpos")
    session.send("down")
    time.sleep(0.05)
    if clipped:
        destinations = [(24, start[1]), start, (start[0], 24), start]
    else:
        destinations = [(start[0] + 75, start[1] + 50), (start[0] - 75, start[1] - 50), start]
    width = session.monitor["width"] / session.monitor["scale"]
    height = session.monitor["height"] / session.monitor["scale"]
    previous = start
    samples = []
    for index, destination in enumerate(destinations):
        delta = [destination[i] - previous[i] for i in (0, 1)]
        move_pointer(session, previous, delta, 10, 20 if clipped else 5)
        time.sleep(0.08)
        cursor = session.query("cursorpos")
        after = session.layer()
        left = before["x"] + cursor["x"] - cursor_before["x"]
        top = before["y"] + cursor["y"] - cursor_before["y"]
        visible_left, visible_top = max(0, left), max(0, top)
        expected = [visible_left, visible_top,
                    min(width, left + before["w"]) - visible_left,
                    min(height, top + before["h"]) - visible_top]
        observed = [after[key] for key in ("x", "y", "w", "h")]
        passed = all(abs(a - b) <= 2 for a, b in zip(observed, expected))
        passed = passed and all(abs(cursor[key] - value) <= 2
                                for key, value in zip(("x", "y"), destination))
        samples.append({"cursor": cursor, "expected": expected, "observed": observed, "passed": passed})
        if session.args.screenshots:
            name = f"{'clipped' if clipped else 'diagonal'}-{index}.png"
            subprocess.run(["grim", "-o", session.monitor["name"], str(session.args.artifacts / name)],
                           env=session.env, check=True, timeout=10, capture_output=True)
        previous = destination
    session.send("up")
    time.sleep(0.2)
    after = session.settled_layer()
    restored = all(abs(after[key] - before[key]) <= 2 for key in ("x", "y", "w", "h"))
    return {"name": "held-clipped-path" if clipped else "held-diagonal-reversal",
            "passed": restored and all(sample["passed"] for sample in samples),
            "samples": samples, "after_release": after}


def check_resize(session, edge, delta):
    """在 session 沿 edge 连续调整 delta，检查固定边、抓取点和比例，返回结果记录。"""
    before = session.layer()
    horizontal = "left" in edge or "right" in edge
    vertical = "top" in edge or "bottom" in edge
    x = before["x"] + (2 if "left" in edge else before["w"] - 2 if "right" in edge else before["w"] / 2)
    y = before["y"] + (2 if "top" in edge else before["h"] - 2 if "bottom" in edge else before["h"] / 2)
    session.send(f"move {x} {y}")
    time.sleep(0.15)
    cursor_before = session.query("cursorpos")
    session.send("down")
    time.sleep(0.05)
    move_pointer(session, (x, y), delta, 10)
    session.send("up")
    time.sleep(0.2)
    cursor_after = session.query("cursorpos")
    after = session.settled_layer()
    cursor_delta = [cursor_after[key] - cursor_before[key] for key in ("x", "y")]
    checks = [abs(actual - target) <= 2 for actual, target in zip(cursor_delta, delta)]
    for axis, extent, first_edge in (("x", "w", "left"), ("y", "h", "top")):
        index = 0 if axis == "x" else 1
        moving_axis = horizontal if axis == "x" else vertical
        if moving_axis:
            moving_before = before[axis] + (0 if first_edge in edge else before[extent])
            moving_after = after[axis] + (0 if first_edge in edge else after[extent])
            fixed_before = before[axis] + (before[extent] if first_edge in edge else 0)
            fixed_after = after[axis] + (after[extent] if first_edge in edge else 0)
            checks.extend((abs(moving_after - moving_before - cursor_delta[index]) <= 2,
                           abs(fixed_after - fixed_before) <= 2))
        else:
            checks.append(abs(after[axis] + after[extent] / 2 - before[axis] - before[extent] / 2) <= 2)
    checks.append(abs(after["w"] / after["h"] - before["w"] / before["h"]) <= 0.02)
    return {"name": f"resize-{edge}-{delta}", "passed": all(checks), "before": before, "after": after,
            "cursor_delta": cursor_delta}


def check_wheel(session, ticks):
    """在 session 内滚动 ticks 刻度，检查缩放方向和鼠标下的图片锚点，返回结果记录。"""
    before = session.layer()
    x = before["x"] + before["w"] * 0.35
    y = before["y"] + before["h"] * 0.45
    session.send(f"move {x} {y}")
    time.sleep(0.15)
    cursor = session.query("cursorpos")
    session.send(f"scroll {ticks}")
    time.sleep(0.2)
    after = session.settled_layer()
    errors = []
    for axis, extent in (("x", "w"), ("y", "h")):
        fraction = (cursor[axis] - before[axis]) / before[extent]
        errors.append(abs(after[axis] + fraction * after[extent] - cursor[axis]))
    passed = max(errors) <= 2 and ((after["w"] > before["w"]) if ticks < 0 else (after["w"] < before["w"]))
    return {"name": f"wheel-{ticks}", "passed": passed, "before": before, "after": after,
            "anchor_error": errors}


def report_result(results, result, scale):
    """将 result 加入 results，并输出 scale 下的检查状态，无返回值。"""
    results.append(result)
    status = "PASS" if result["passed"] else "FAIL"
    details = f"pointer={result['cursor_delta']} image={result['image_delta']}" if "image_delta" in result else ""
    print(f"{status} scale={scale} {result['name']} {details}".rstrip(), flush=True)


def run_checks(args):
    """按 args 创建正式贴图并执行拖动检查，写入结果文件，返回检查退出码。"""
    with HyprlandTestSession(args) as session:
        fixture = session.root / "fixture.png"
        write_image(fixture)
        session.start_app(fixture)
        session.start_pointer()
        results = []
        deltas = [(100, 0)] if args.quick else [(300, 0), (-300, 0), (0, 100), (0, -100)]
        intervals = (args.interval,) if args.quick else dict.fromkeys((args.interval, 1, 0))
        for interval in intervals:
            for delta in deltas:
                report_result(results, check_drag(session, delta, interval), args.scale)
        if not args.quick:
            for delta in ((100, 0), (-100, 0)):
                report_result(results, check_drag(session, delta, 10, 100), args.scale)
            report_result(results, check_held_path(session), args.scale)
            report_result(results, check_held_path(session, clipped=True), args.scale)
            for edge, delta in (("left", (-80, 0)), ("top", (0, -40)), ("bottom-right", (80, 40))):
                report_result(results, check_resize(session, edge, delta), args.scale)
                report_result(results, check_resize(session, edge, [-value for value in delta]), args.scale)
            for ticks in (-1, 1):
                report_result(results, check_wheel(session, ticks), args.scale)
            report_result(results, check_cross_output(session), args.scale)
        (args.artifacts / "result.json").write_text(json.dumps({"scale": args.scale, "checks": results}, indent=2) + "\n")
        return 0 if all(result["passed"] for result in results) else 1


def parse_arguments():
    """解析应用、指针和隔离显示参数，返回参数对象。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--pointer", type=Path, required=True)
    parser.add_argument("--hyprland", default="Hyprland")
    parser.add_argument("--hyprctl", default="hyprctl")
    parser.add_argument("--kwin", default="kwin_wayland")
    parser.add_argument("--scale", type=float, choices=(1, 1.5, 2), default=1)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--protocol-log", action="store_true")
    parser.add_argument("--screenshots", action="store_true", help="Capture held drag frames with grim")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--interval", type=float, default=10)
    parser.add_argument("--inside-session", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.interval < 0 or args.interval > 100:
        parser.error("--interval must be between 0 and 100 milliseconds")
    args.binary = args.binary.resolve(strict=True)
    args.pointer = args.pointer.resolve(strict=True)
    args.artifacts = args.artifacts.resolve()
    args.artifacts.mkdir(parents=True, exist_ok=True)
    return args


def main():
    """建立独立 D-Bus 及短路径运行目录，执行测试并清理进程，返回退出码。"""
    args = parse_arguments()
    if args.inside_session:
        return run_checks(args)
    for name in ("result.json", "app-debug.log"):
        (args.artifacts / name).unlink(missing_ok=True)
    # 1. 【Hyprland测试】【运行目录】短路径避免超过 Wayland 与 Hyprland 的 UNIX socket 长度限制
    with tempfile.TemporaryDirectory(prefix="mshypr-", dir="/tmp") as name:
        return run_isolated(args, Path(__file__).resolve(), Path(name), sys.argv[1:])


if __name__ == "__main__":
    sys.exit(main())
