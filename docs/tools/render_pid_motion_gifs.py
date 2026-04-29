#!/usr/bin/env python3
"""Render PID motion GIFs from fresh H02 pMarinePIDV22 alogs.

These are generated documentation views backed by the original vehicle `.alog`
files from `harnesses/pid_harnesses/H02-pid_motion`. They visualize the motion
and PID actuator evidence that raw pMarineViewer clips do not show clearly.
"""

from __future__ import annotations

import argparse
import bisect
import math
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
OUT_DIR = ROOT / "assets" / "gifs"
DEFAULT_RUN_ROOT = REPO / "harnesses/pid_harnesses/H02-pid_motion"

W, H = 1600, 900
FRAMES = 96
FRAME_MS = 83

BG = "#202433"
WATER_A = "#222638"
WATER_B = "#1f2331"
GRID = "#30364a"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
YELLOW = "#f3ea5f"
WHITE = "#f5f7fb"
TRAIL = "#cbd3df"
GREEN = "#75d38b"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
RED = "#f06c64"
PURPLE = "#9b8cff"

MAP_RECT = (56, 76, 1038, 820)
PANEL_X0 = 1080
PANEL_X1 = 1546


@dataclass(frozen=True)
class Spec:
    case_dir_prefix: str
    output: str
    title: str
    subtitle: str
    t0: float
    t1: float
    dest: tuple[float, float]
    map_bounds: tuple[float, float, float, float]
    traces: tuple[tuple[str, str, float, float, str], ...]
    gate: tuple[float, float, float, float] | None = None
    depth_target: float | None = None


SPECS = (
    Spec(
        case_dir_prefix="002_hard_turn_recover_pass",
        output="pid-motion-hard-turn-recover.gif",
        title="Hard Turn Recovery",
        subtitle="alog: heading 270 -> recovered arrival",
        t0=4.2,
        t1=58.0,
        dest=(70.0, -60.0),
        map_bounds=(-14.0, -84.0, 92.0, -30.0),
        gate=(55.0, -76.0, 80.0, -44.0),
        traces=(
            ("NAV_HEADING", "heading", 60.0, 280.0, TEAL),
            ("DESIRED_RUDDER", "rudder", -100.0, 100.0, ORANGE),
            ("NAV_SPEED", "speed", 0.0, 3.0, GREEN),
        ),
    ),
    Spec(
        case_dir_prefix="006_depth_step_pass",
        output="pid-motion-depth-step.gif",
        title="Depth Step Response",
        subtitle="alog: depth reaches 10m; elevator reverses",
        t0=4.2,
        t1=49.0,
        dest=(140.0, -60.0),
        map_bounds=(-10.0, -82.0, 150.0, -38.0),
        gate=(50.0, -74.0, 150.0, -46.0),
        depth_target=10.0,
        traces=(
            ("NAV_DEPTH", "depth", 0.0, 13.0, TEAL),
            ("DESIRED_ELEVATOR", "elevator", -15.0, 15.0, ORANGE),
            ("DESIRED_THRUST", "thrust", 0.0, 60.0, GREEN),
        ),
    ),
)


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
    ]
    for candidate in candidates:
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            continue
    return ImageFont.load_default()


FONT_TITLE = font(46, True)
FONT_LABEL = font(28, True)
FONT_SMALL = font(24)
FONT_TINY = font(21)


def find_latest_run_root(base: Path) -> Path:
    if base.name.startswith(".parallel_pid_motion_"):
        return base
    roots = sorted(base.glob(".parallel_pid_motion_*"), key=lambda p: p.stat().st_mtime)
    if not roots:
        raise FileNotFoundError(f"no .parallel_pid_motion_* directories under {base}")
    return roots[-1]


def case_alog(run_root: Path, prefix: str) -> Path:
    matches = sorted(run_root.glob(f"{prefix}/LOG_ABE_*/*.alog"))
    if not matches:
        raise FileNotFoundError(f"no vehicle alog for {prefix} under {run_root}")
    return matches[-1]


def parse_alog(alog: Path) -> dict[str, list[tuple[float, float]]]:
    keep = {
        "NAV_X",
        "NAV_Y",
        "NAV_HEADING",
        "NAV_SPEED",
        "NAV_DEPTH",
        "DESIRED_RUDDER",
        "DESIRED_THRUST",
        "DESIRED_ELEVATOR",
    }
    series: dict[str, list[tuple[float, float]]] = {name: [] for name in keep}
    for line in alog.read_text(errors="ignore").splitlines():
        if not line or line.startswith("%"):
            continue
        parts = line.split(None, 3)
        if len(parts) < 4 or parts[1] not in keep:
            continue
        value_token = parts[3].split()[-1]
        try:
            t = float(parts[0])
            value = float(value_token)
        except ValueError:
            continue
        rows = series[parts[1]]
        if rows and abs(rows[-1][0] - t) < 0.0001:
            rows[-1] = (t, value)
        else:
            rows.append((t, value))
    return series


def interp(rows: list[tuple[float, float]], t: float) -> float:
    if not rows:
        return 0.0
    times = [row[0] for row in rows]
    idx = bisect.bisect_left(times, t)
    if idx <= 0:
        return rows[0][1]
    if idx >= len(rows):
        return rows[-1][1]
    a_t, a_v = rows[idx - 1]
    b_t, b_v = rows[idx]
    f = 0.0 if b_t == a_t else (t - a_t) / (b_t - a_t)
    return a_v + (b_v - a_v) * f


def values_at(series: dict[str, list[tuple[float, float]]], t: float) -> dict[str, float]:
    return {name: interp(rows, t) for name, rows in series.items()}


def map_xy(spec: Spec, x: float, y: float) -> tuple[float, float]:
    left, top, right, bottom = MAP_RECT
    min_x, min_y, max_x, max_y = spec.map_bounds
    px = left + (x - min_x) / (max_x - min_x) * (right - left)
    py = bottom - (y - min_y) / (max_y - min_y) * (bottom - top)
    return px, py


def draw_background(draw: ImageDraw.ImageDraw) -> None:
    draw.rectangle((0, 0, W, H), fill=BG)
    draw.rectangle(MAP_RECT, fill=WATER_A)
    for x in range(MAP_RECT[0], MAP_RECT[2] + 1, 54):
        draw.line((x, MAP_RECT[1], x, MAP_RECT[3]), fill=GRID, width=1)
    for y in range(MAP_RECT[1], MAP_RECT[3] + 1, 54):
        draw.line((MAP_RECT[0], y, MAP_RECT[2], y), fill=GRID, width=1)
    for x in range(MAP_RECT[0] - 160, MAP_RECT[2] + 200, 108):
        draw.line((x, MAP_RECT[1], x - 220, MAP_RECT[3]), fill=WATER_B, width=1)
    draw.rectangle(MAP_RECT, outline="#4a5062", width=2)


def draw_boat(draw: ImageDraw.ImageDraw, spec: Spec, x: float, y: float, heading: float) -> None:
    px, py = map_xy(spec, x, y)
    angle = math.radians(90 - heading)
    length = 25
    beam = 10
    pts = [
        (px + math.cos(angle) * length, py - math.sin(angle) * length),
        (px + math.cos(angle + 2.45) * beam, py - math.sin(angle + 2.45) * beam),
        (px + math.cos(angle - 2.45) * beam, py - math.sin(angle - 2.45) * beam),
    ]
    draw.polygon(pts, fill=YELLOW, outline="#fff7a6")
    draw.text((px + 12, py - 14), "abe", fill=WHITE, font=FONT_SMALL)


def draw_trace(
    draw: ImageDraw.ImageDraw,
    rows: list[tuple[float, float]],
    t0: float,
    t: float,
    rect: tuple[int, int, int, int],
    lo: float,
    hi: float,
    color: str,
) -> None:
    x0, y0, x1, y1 = rect
    draw.rectangle(rect, fill="#252a3a", outline="#444b60", width=1)
    if lo < 0 < hi:
        zy = y1 - (0 - lo) / (hi - lo) * (y1 - y0)
        draw.line((x0, zy, x1, zy), fill="#596070", width=1)
    pts: list[tuple[float, float]] = []
    for row_t, row_v in rows:
        if row_t < t0 or row_t > t:
            continue
        px = x0 + (row_t - t0) / max(t - t0, 0.001) * (x1 - x0)
        py = y1 - (max(lo, min(hi, row_v)) - lo) / (hi - lo) * (y1 - y0)
        pts.append((px, py))
    if len(pts) > 1:
        draw.line(pts, fill=color, width=3)


def draw_panel(draw: ImageDraw.ImageDraw, spec: Spec, series: dict[str, list[tuple[float, float]]], t: float) -> None:
    draw.text((PANEL_X0, 76), spec.title, fill=TEXT, font=FONT_TITLE)
    draw.text((PANEL_X0, 130), spec.subtitle, fill=MUTED, font=FONT_TINY)
    vals = values_at(series, t)
    metric_y = 182
    for label, value, color in (
        ("x", vals["NAV_X"], TEAL),
        ("y", vals["NAV_Y"], TEAL),
        ("hdg", vals["NAV_HEADING"], YELLOW),
        ("spd", vals["NAV_SPEED"], GREEN),
        ("dep", vals["NAV_DEPTH"], PURPLE),
    ):
        draw.rounded_rectangle((PANEL_X0, metric_y, PANEL_X0 + 100, metric_y + 44), radius=8, fill="#252a3a", outline="#3c4356")
        draw.text((PANEL_X0 + 12, metric_y + 10), label, fill=color, font=FONT_TINY)
        draw.text((PANEL_X0 + 116, metric_y + 8), f"{value:6.2f}", fill=TEXT, font=FONT_SMALL)
        metric_y += 50
    trace_y = 458
    for var, label, lo, hi, color in spec.traces:
        draw.text((PANEL_X0, trace_y - 32), f"{label}: {vals[var]:.2f}", fill=color, font=FONT_SMALL)
        draw_trace(draw, series[var], spec.t0, t, (PANEL_X0, trace_y, PANEL_X1, trace_y + 82), lo, hi, color)
        if spec.depth_target is not None and var == "NAV_DEPTH":
            yy = trace_y + 82 - (spec.depth_target - lo) / (hi - lo) * 82
            draw.line((PANEL_X0, yy, PANEL_X1, yy), fill=WHITE, width=1)
            draw.text((PANEL_X1 - 132, yy - 27), "10m target", fill=WHITE, font=FONT_TINY)
        trace_y += 128


def draw_gate(draw: ImageDraw.ImageDraw, spec: Spec) -> None:
    if spec.gate is None:
        return
    x0, y0, x1, y1 = spec.gate
    p0 = map_xy(spec, x0, y0)
    p1 = map_xy(spec, x1, y1)
    rect = (min(p0[0], p1[0]), min(p0[1], p1[1]), max(p0[0], p1[0]), max(p0[1], p1[1]))
    draw.rectangle(rect, fill="#75d38b22", outline=GREEN, width=2)
    draw.text((rect[0] + 8, rect[1] + 8), "pass gate", fill=GREEN, font=FONT_TINY)


def render(spec: Spec, alog: Path) -> Path:
    series = parse_alog(alog)
    frames: list[Image.Image] = []
    for idx in range(FRAMES):
        frac = idx / (FRAMES - 1)
        t = spec.t0 + (spec.t1 - spec.t0) * frac
        vals = values_at(series, t)
        img = Image.new("RGB", (W, H), BG)
        draw = ImageDraw.Draw(img, "RGBA")
        draw_background(draw)
        draw_gate(draw, spec)
        start = map_xy(spec, 0.0, -60.0)
        dest = map_xy(spec, *spec.dest)
        draw.line((start, dest), fill="#ffffff55", width=2)
        draw.ellipse((dest[0] - 6, dest[1] - 6, dest[0] + 6, dest[1] + 6), fill=GREEN)
        draw.text((dest[0] + 10, dest[1] - 12), "waypoint", fill=MUTED, font=FONT_TINY)
        trail = []
        xs = series["NAV_X"]
        ys = series["NAV_Y"]
        for row_t, row_x in xs:
            if row_t > t:
                break
            row_y = interp(ys, row_t)
            trail.append(map_xy(spec, row_x, row_y))
        if len(trail) > 1:
            draw.line(trail, fill=TRAIL, width=3)
        draw_boat(draw, spec, vals["NAV_X"], vals["NAV_Y"], vals["NAV_HEADING"])
        draw.text((MAP_RECT[0] + 18, MAP_RECT[1] + 18), f"{spec.case_dir_prefix.split('_', 1)[1]}  t={t:.1f}", fill=MUTED, font=FONT_SMALL)
        draw_panel(draw, spec, series, t)
        frames.append(img)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUT_DIR / spec.output
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=FRAME_MS, loop=0, disposal=2, optimize=True)
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description="Render PID motion website GIFs from H02 alogs.")
    parser.add_argument("--run-root", type=Path, default=DEFAULT_RUN_ROOT, help="H02 run root or .parallel_pid_motion_* directory")
    args = parser.parse_args()
    run_root = find_latest_run_root(args.run_root)
    print(f"run_root={run_root}")
    for spec in SPECS:
        alog = case_alog(run_root, spec.case_dir_prefix)
        out = render(spec, alog)
        print(f"{spec.output}: {alog} -> {out}")


if __name__ == "__main__":
    main()
