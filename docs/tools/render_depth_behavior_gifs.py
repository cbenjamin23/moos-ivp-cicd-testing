#!/usr/bin/env python3
"""Render depth behavior GIFs from depth harness alogs.

The depth harnesses are hard to explain with raw top-down pMarineViewer clips,
so these generated views keep a PMV-like map while showing the vertical signal
that makes the case meaningful.
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
DEFAULT_MISSION = REPO / "missions/depth_behavior_missions/depth_behavior_motion"

W, H = 1600, 900
FRAMES = 96
FRAME_MS = 83

BG = "#202433"
WATER = "#222638"
WATER_DARK = "#1c2030"
GRID = "#30364a"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
YELLOW = "#f3ea5f"
WHITE = "#f5f7fb"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
GREEN = "#75d38b"
RED = "#f06c64"

MAP_RECT = (56, 76, 1018, 820)
PANEL = (1060, 76, 1546, 820)


@dataclass(frozen=True)
class Guide:
    depth: float
    label: str
    color: str = GREEN


@dataclass(frozen=True)
class Spec:
    output: str
    title: str
    subtitle: str
    t0: float
    t1: float
    map_bounds: tuple[float, float, float, float]
    chart_max_depth: float
    guides: tuple[Guide, ...]
    target_schedule: tuple[tuple[float, float], ...]
    target_label: str = "target"
    mode: str = "depth"
    min_altitude: float | None = None


SPECS = {
    "constant-depth-hold": Spec(
        output="constant-depth-hold.gif",
        title="Held Depth",
        subtitle="target 12m with depth feedback",
        t0=5.0,
        t1=70.0,
        map_bounds=(-10.0, -82.0, 170.0, -38.0),
        chart_max_depth=16.0,
        guides=(Guide(12.0, "target 12m"),),
        target_schedule=((0.0, 12.0),),
    ),
    "constant-depth-update": Spec(
        output="constant-depth-update.gif",
        title="Runtime Update",
        subtitle="target changes from 12m to 18m",
        t0=5.0,
        t1=70.0,
        map_bounds=(-10.0, -82.0, 170.0, -38.0),
        chart_max_depth=24.0,
        guides=(Guide(12.0, "initial 12m", MUTED), Guide(18.0, "updated 18m")),
        target_schedule=((0.0, 12.0), (10.0, 18.0)),
    ),
    "goto-depth-sequence": Spec(
        output="goto-depth-sequence.gif",
        title="Depth Sequence",
        subtitle="three commanded depth levels",
        t0=5.0,
        t1=80.0,
        map_bounds=(-10.0, -82.0, 190.0, -38.0),
        chart_max_depth=18.0,
        guides=(Guide(6.0, "level 6m", MUTED), Guide(14.0, "level 14m"), Guide(9.0, "level 9m", MUTED)),
        target_schedule=((0.0, 6.0), (16.0, 14.0), (31.0, 9.0)),
    ),
    "goto-depth-crossing": Spec(
        output="goto-depth-crossing.gif",
        title="Crossing Arrivals",
        subtitle="down-up crossings count arrivals",
        t0=5.0,
        t1=80.0,
        map_bounds=(-10.0, -82.0, 190.0, -38.0),
        chart_max_depth=16.0,
        guides=(Guide(12.0, "12m crossing"), Guide(4.0, "4m crossing", MUTED)),
        target_schedule=((0.0, 12.0), (20.0, 4.0), (40.0, 12.0)),
    ),
    "periodic-surface-cycle": Spec(
        output="periodic-surface-cycle.gif",
        title="Surfacing Cycle",
        subtitle="periodic ascent over held depth",
        t0=5.0,
        t1=70.0,
        map_bounds=(-10.0, -82.0, 170.0, -38.0),
        chart_max_depth=16.0,
        guides=(Guide(2.0, "surface zone 2m"), Guide(12.0, "held depth", MUTED)),
        target_schedule=((0.0, 2.0),),
        target_label="surface zone",
    ),
    "periodic-surface-wait-window": Spec(
        output="periodic-surface-wait-window.gif",
        title="Wait Window",
        subtitle="no ascent before period expires",
        t0=8.0,
        t1=30.0,
        map_bounds=(10.0, -82.0, 90.0, -38.0),
        chart_max_depth=16.0,
        guides=(Guide(2.0, "surface zone 2m"), Guide(12.0, "held depth", MUTED)),
        target_schedule=((0.0, 12.0),),
        target_label="expected",
    ),
    "max-depth-guard": Spec(
        output="max-depth-guard.gif",
        title="Max Depth Guard",
        subtitle="30m command constrained near 12m",
        t0=5.0,
        t1=70.0,
        map_bounds=(-10.0, -82.0, 170.0, -38.0),
        chart_max_depth=32.0,
        guides=(Guide(12.0, "max depth 12m"), Guide(30.0, "unsafe command", RED)),
        target_schedule=((0.0, 12.0),),
        target_label="guard",
    ),
    "max-depth-unconstrained": Spec(
        output="max-depth-unconstrained.gif",
        title="Unconstrained Shallow",
        subtitle="8m command stays above 20m guard",
        t0=5.0,
        t1=70.0,
        map_bounds=(-10.0, -82.0, 170.0, -38.0),
        chart_max_depth=24.0,
        guides=(Guide(8.0, "target 8m"), Guide(20.0, "guard 20m", MUTED)),
        target_schedule=((0.0, 8.0),),
    ),
    "min-altitude-guard": Spec(
        output="min-altitude-guard.gif",
        title="Min Altitude Guard",
        subtitle="bottom 20m requires 8m clearance",
        t0=5.0,
        t1=70.0,
        map_bounds=(-10.0, -82.0, 170.0, -38.0),
        chart_max_depth=32.0,
        guides=(Guide(12.0, "clearance boundary"), Guide(20.0, "bottom", RED), Guide(30.0, "unsafe command", MUTED)),
        target_schedule=((0.0, 12.0),),
        target_label="boundary",
        mode="altitude",
        min_altitude=8.0,
    ),
    "min-altitude-unconstrained": Spec(
        output="min-altitude-unconstrained.gif",
        title="Deep Bottom",
        subtitle="22m command has ample clearance",
        t0=5.0,
        t1=70.0,
        map_bounds=(-10.0, -82.0, 170.0, -38.0),
        chart_max_depth=32.0,
        guides=(Guide(22.0, "target 22m"),),
        target_schedule=((0.0, 22.0),),
        mode="altitude",
        min_altitude=8.0,
    ),
}


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
FONT_LABEL = font(27, True)
FONT_BODY = font(24)
FONT_SMALL = font(20)


def latest_alog(mission_root: Path) -> Path:
    matches = sorted(mission_root.glob("LOG_ABE_*/*.alog"), key=lambda p: p.stat().st_mtime)
    if not matches:
        raise FileNotFoundError(f"no LOG_ABE_*/.alog under {mission_root}")
    return matches[-1]


def parse_alog(alog: Path) -> dict[str, list[tuple[float, float]]]:
    keep = {
        "NAV_X",
        "NAV_Y",
        "NAV_HEADING",
        "NAV_SPEED",
        "NAV_DEPTH",
        "NAV_ALTITUDE",
        "DESIRED_ELEVATOR",
    }
    series: dict[str, list[tuple[float, float]]] = {name: [] for name in keep}
    for line in alog.read_text(errors="ignore").splitlines():
        if not line or line.startswith("%"):
            continue
        parts = line.split(None, 3)
        if len(parts) < 4 or parts[1] not in keep:
            continue
        try:
            t = float(parts[0])
            value = float(parts[3].split()[-1])
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
    ratio = 0.0 if a_t == b_t else (t - a_t) / (b_t - a_t)
    return a_v + (b_v - a_v) * ratio


def values_at(series: dict[str, list[tuple[float, float]]], t: float) -> dict[str, float]:
    return {name: interp(rows, t) for name, rows in series.items()}


def target_at(spec: Spec, t: float) -> float:
    target = spec.target_schedule[0][1]
    for target_t, target_depth in spec.target_schedule:
        if t >= target_t:
            target = target_depth
        else:
            break
    return target


def map_xy(spec: Spec, x: float, y: float) -> tuple[float, float]:
    left, top, right, bottom = MAP_RECT
    min_x, min_y, max_x, max_y = spec.map_bounds
    px = left + (x - min_x) / (max_x - min_x) * (right - left)
    py = bottom - (y - min_y) / (max_y - min_y) * (bottom - top)
    return px, py


def draw_background(draw: ImageDraw.ImageDraw) -> None:
    draw.rectangle((0, 0, W, H), fill=BG)
    draw.rectangle(MAP_RECT, fill=WATER)
    for x in range(MAP_RECT[0], MAP_RECT[2] + 1, 54):
        draw.line((x, MAP_RECT[1], x, MAP_RECT[3]), fill=GRID)
    for y in range(MAP_RECT[1], MAP_RECT[3] + 1, 54):
        draw.line((MAP_RECT[0], y, MAP_RECT[2], y), fill=GRID)
    for x in range(MAP_RECT[0] - 180, MAP_RECT[2] + 220, 108):
        draw.line((x, MAP_RECT[1], x - 230, MAP_RECT[3]), fill=WATER_DARK)
    draw.rectangle(MAP_RECT, outline="#4a5062", width=2)
    draw.rounded_rectangle(PANEL, radius=16, fill="#181d2a", outline="#3d455a", width=2)


def draw_boat(draw: ImageDraw.ImageDraw, spec: Spec, vals: dict[str, float]) -> None:
    px, py = map_xy(spec, vals["NAV_X"], vals["NAV_Y"])
    angle = math.radians(90 - vals["NAV_HEADING"])
    length = 27
    beam = 11
    pts = [
        (px + math.cos(angle) * length, py - math.sin(angle) * length),
        (px + math.cos(angle + 2.45) * beam, py - math.sin(angle + 2.45) * beam),
        (px + math.cos(angle - 2.45) * beam, py - math.sin(angle - 2.45) * beam),
    ]
    draw.polygon(pts, fill=YELLOW, outline="#fff7a6")
    label = f"abe (depth={vals['NAV_DEPTH']:.1f})"
    label_box = draw.textbbox((0, 0), label, font=FONT_BODY)
    label_w = label_box[2] - label_box[0]
    label_x = min(max(px + 14, MAP_RECT[0] + 8), MAP_RECT[2] - label_w - 8)
    label_y = min(max(py - 28, MAP_RECT[1] + 8), MAP_RECT[3] - 34)
    draw.text((label_x, label_y), label, fill=WHITE, font=FONT_BODY)


def draw_track(draw: ImageDraw.ImageDraw, spec: Spec, series: dict[str, list[tuple[float, float]]], t: float) -> None:
    xs = series["NAV_X"]
    points: list[tuple[float, float]] = []
    for sample_t, x in xs:
        if sample_t < spec.t0 or sample_t > t:
            continue
        y = interp(series["NAV_Y"], sample_t)
        points.append(map_xy(spec, x, y))
    if len(points) > 1:
        draw.line(points, fill="#cbd3df", width=3)


def draw_depth_panel(draw: ImageDraw.ImageDraw, spec: Spec, series: dict[str, list[tuple[float, float]]], t: float) -> None:
    x0, y0, x1, y1 = PANEL
    draw.text((x0 + 28, y0 + 28), spec.title, fill=TEXT, font=FONT_TITLE)
    draw.text((x0 + 30, y0 + 88), spec.subtitle, fill=MUTED, font=FONT_BODY)

    chart = (x0 + 40, y0 + 168, x1 - 40, y0 + 506)
    draw.rounded_rectangle(chart, radius=10, fill="#202638", outline="#424b61", width=2)
    c_left, c_top, c_right, c_bottom = chart
    min_depth, max_depth = 0.0, spec.chart_max_depth

    def chart_xy(sample_t: float, depth: float) -> tuple[float, float]:
        px = c_left + (sample_t - spec.t0) / (spec.t1 - spec.t0) * (c_right - c_left)
        py = c_top + (depth - min_depth) / (max_depth - min_depth) * (c_bottom - c_top)
        return px, py

    step = 4 if spec.chart_max_depth <= 24 else 8
    for depth in range(0, int(spec.chart_max_depth) + 1, step):
        _, py = chart_xy(spec.t0, depth)
        draw.line((c_left, py, c_right, py), fill="#30364a")
        draw.text((c_left - 30, py - 10), str(depth), fill=MUTED, font=FONT_SMALL)

    for guide in spec.guides:
        guide_y = chart_xy(spec.t0, guide.depth)[1]
        draw.line((c_left, guide_y, c_right, guide_y), fill=guide.color, width=3)
        label_y = max(c_top + 6, min(guide_y - 30, c_bottom - 24))
        draw.text((c_left + 10, label_y), guide.label, fill=guide.color, font=FONT_SMALL)

    points: list[tuple[float, float]] = []
    for sample_t, depth in series["NAV_DEPTH"]:
        if spec.t0 <= sample_t <= min(t, spec.t1):
            points.append(chart_xy(sample_t, depth))
    if len(points) > 1:
        draw.line(points, fill=TEAL, width=5)

    vals = values_at(series, t)
    draw.line((chart_xy(t, min_depth)[0], c_top, chart_xy(t, min_depth)[0], c_bottom), fill=WHITE, width=2)

    target = target_at(spec, t)
    if spec.mode == "altitude":
        min_altitude = spec.min_altitude or 0.0
        rows = [
            ("NAV_DEPTH", f"{vals['NAV_DEPTH']:.1f} m", TEAL),
            ("NAV_ALTITUDE", f"{vals['NAV_ALTITUDE']:.1f} m", GREEN),
            ("min altitude", f"{min_altitude:.1f} m", MUTED),
            ("alt margin", f"{vals['NAV_ALTITUDE'] - min_altitude:+.1f} m", ORANGE),
        ]
    else:
        rows = [
            ("NAV_DEPTH", f"{vals['NAV_DEPTH']:.1f} m", TEAL),
            (spec.target_label, f"{target:.1f} m", GREEN),
            ("error", f"{vals['NAV_DEPTH'] - target:+.1f} m", ORANGE),
            ("elevator", f"{vals['DESIRED_ELEVATOR']:+.0f}", RED),
        ]
    y = y0 + 545
    for label, value, color in rows:
        draw.text((x0 + 42, y), label, fill=MUTED, font=FONT_BODY)
        draw.text((x1 - 220, y), value, fill=color, font=FONT_LABEL)
        y += 44


def render(spec: Spec, alog: Path, out_dir: Path) -> Path:
    series = parse_alog(alog)
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        frac = i / (FRAMES - 1)
        t = spec.t0 + (spec.t1 - spec.t0) * frac
        image = Image.new("RGB", (W, H), BG)
        draw = ImageDraw.Draw(image)
        draw_background(draw)
        draw.text((58, 22), "Depth Behavior Harness", fill=MUTED, font=FONT_BODY)
        draw.text((58, 840), f"source: {alog.parent.name}", fill=MUTED, font=FONT_SMALL)
        draw_track(draw, spec, series, t)
        draw_boat(draw, spec, values_at(series, t))
        draw_depth_panel(draw, spec, series, t)
        frames.append(image)

    out_dir.mkdir(parents=True, exist_ok=True)
    output = out_dir / spec.output
    frames[0].save(output, save_all=True, append_images=frames[1:], duration=FRAME_MS, loop=0, optimize=True)
    return output


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--spec", choices=sorted(SPECS), default="constant-depth-hold")
    parser.add_argument("--alog", type=Path)
    parser.add_argument("--mission-root", type=Path, default=DEFAULT_MISSION)
    parser.add_argument("--out-dir", type=Path, default=OUT_DIR)
    args = parser.parse_args()

    alog = args.alog if args.alog else latest_alog(args.mission_root)
    output = render(SPECS[args.spec], alog, args.out_dir)
    print(output)


if __name__ == "__main__":
    main()
