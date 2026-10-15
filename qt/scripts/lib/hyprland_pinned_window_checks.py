"""检查正式贴图跨输出拖动时的输入原点、画面接管和预览生命周期。"""

import json
import math
import subprocess
import time

from .hyprland_test_session import wait_until


def add_adjacent_output(session):
    """为 session 增加相邻且缩放不同的虚拟输出，返回输出记录。"""
    width = round(session.monitor["width"] / session.monitor["scale"])
    existing = {monitor["name"] for monitor in session.query("monitors")}
    for name in existing - {session.monitor["name"]}:
        session.control("keyword", "monitor", f"{name},preferred,10000x0,1")
    scale = 2 if session.monitor["scale"] == 1 else 1
    name = "mark-shot-test-secondary"
    session.control("keyword", "monitor", f"{name},2880x1800@120,{width}x0,{scale}")
    session.control("output", "create", "headless", name)
    target = wait_until(lambda: next((monitor for monitor in session.query("monitors")
                                      if monitor["name"] not in existing), None), "second test output")
    time.sleep(0.3)
    target = next(monitor for monitor in session.query("monitors") if monitor["name"] == target["name"])
    if target["x"] != width or abs(target["scale"] - scale) > 0.01:
        raise RuntimeError("The compositor did not apply the adjacent output layout")
    (session.args.artifacts / "second-monitor.json").write_text(json.dumps(target, indent=2) + "\n")
    return target


def move_relative_to(session, destination):
    """通过相对输入将 session 的鼠标移向全局 destination，跨输出时保留同一鼠标抓取。"""
    before = session.query("cursorpos")
    dx, dy = destination[0] - before["x"], destination[1] - before["y"]
    steps = max(1, math.ceil(max(abs(dx), abs(dy)) / 10))
    for _ in range(steps):
        session.send(f"relative {dx / steps} {dy / steps}")
        time.sleep(0.001)
    time.sleep(0.08)


def held_sample(session, before, cursor_before, destination):
    """向 destination 移动后比较 session 的实际位移与 before 抓取状态，返回检查记录。"""
    move_relative_to(session, destination)
    cursor = session.query("cursorpos")
    after = session.layer()
    expected = [before["x"] + cursor["x"] - cursor_before["x"],
                before["y"] + cursor["y"] - cursor_before["y"], before["w"], before["h"]]
    observed = [after[key] for key in ("x", "y", "w", "h")]
    originals = [layer for layer in session.layers() if layer["namespace"] == "dock"]
    fixed_input = len(originals) == 1 and all(originals[0][key] == before[key]
                                              for key in ("x", "y", "w", "h"))
    passed = fixed_input and all(abs(a - b) <= 2 for a, b in zip(observed, expected))
    passed = passed and all(abs(cursor[key] - value) <= 2
                            for key, value in zip(("x", "y"), destination))
    return {"cursor": cursor, "expected": expected, "observed": observed,
            "fixed_input": fixed_input, "passed": passed}


def check_cross_output(session):
    """检查 session 中混合缩放跨屏往返、松开和再次拖动，返回位移与预览回收结果。"""
    target = add_adjacent_output(session)
    before = session.settled_layer()
    session.send(f"move {before['x'] + before['w'] * 0.35} {before['y'] + before['h'] * 0.45}")
    time.sleep(0.15)
    cursor_before = session.query("cursorpos")
    session.send("down")
    time.sleep(0.05)
    samples = []
    destinations = [(target["x"] + 400, 300), (target["x"] - 400, 300), (target["x"] + 450, 350)]
    for index, destination in enumerate(destinations):
        samples.append(held_sample(session, before, cursor_before, destination))
        if session.args.screenshots:
            for monitor in (session.monitor, target):
                path = session.args.artifacts / f"cross-output-{index}-{monitor['name']}.png"
                subprocess.run(["grim", "-o", monitor["name"], str(path)], env=session.env,
                               check=True, timeout=10, capture_output=True)
    session.send("up")
    time.sleep(0.2)
    handed_off = session.settled_layer()
    handoff_matches = all(abs(handed_off[key] - value) <= 2
                          for key, value in zip(("x", "y", "w", "h"), samples[-1]["observed"]))

    # 1. 【Hyprland测试】【跨屏收尾】原窗口在新输出重新映射后，再次抓取仍须保持图片锚点
    cursor_before = session.query("cursorpos")
    session.send("down")
    time.sleep(0.05)
    samples.append(held_sample(session, handed_off, cursor_before, (target["x"] - 450, 350)))
    session.send("up")
    time.sleep(0.2)
    after = session.settled_layer()
    restored = all(abs(after[key] - value) <= 2
                   for key, value in zip(("x", "y", "w", "h"), samples[-1]["observed"]))
    return {"name": "cross-output-mixed-scale", "passed": handoff_matches and restored
            and all(sample["passed"] for sample in samples), "samples": samples,
            "handoff": handed_off, "after_release": after}
