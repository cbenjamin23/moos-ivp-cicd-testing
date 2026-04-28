#!/usr/bin/env python3
"""Render obstacle behavior representative GIFs from vehicle alogs."""

from __future__ import annotations

import argparse
import bisect
import math
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"
DEFAULT_ALOG_DIR = Path("/tmp/moos-cicd-obstacle-behavior/alogs")

W, H = 960, 540
FRAMES = 96
FRAME_MS = 83

BG = "#202433"
WATER_A = "#222638"
WATER_B = "#1f2331"
GRID = "#2f3547"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
YELLOW = "#f3ea5f"
TRAIL = "#c8cfd9"
OB_FILL = "#777c88"
OB_RING = "#5a5f6c"
OB_EDGE = "#9da4b3"
GREEN = "#75d38b"
RED = "#f06c64"
WHITE = "#f5f7fb"

MAP = (-12.0, -84.0, 92.0, -28.0)


@dataclass(frozen=True)
class Obstacle:
    label: str
    pts: tuple[tuple[float, float], ...]


@dataclass(frozen=True)
class GifSpec:
    case: str
    output: str
    start_t: float
    end_t: float
    title: str
    badge: str
    badge_color: str
    obstacles: tuple[Obstacle, ...]
    waypoint_label: str


GIFS = (
    GifSpec(
        case="two_obstacles_clean_pass",
        output="obstacle-behavior-two-obstacles.gif",
        start_t=13.5,
        end_t=60.0,
        title="Two Obstacles Clean",
        badge="alerts, avoids, arrives cleanly",
        badge_color=GREEN,
        obstacles=(
            Obstacle("ob_one", ((19, -66), (24, -66), (24, -61), (19, -61))),
            Obstacle("ob_two", ((55, -57), (60, -57), (60, -52), (55, -52))),
        ),
        waypoint_label="abe_transit",
    ),
    GifSpec(
        case="avoid_disabled_fail",
        output="obstacle-behavior-avoid-disabled.gif",
        start_t=13.5,
        end_t=43.0,
        title="Avoid Disabled Fail",
        badge="alert path appears, behavior cannot run",
        badge_color=RED,
        obstacles=(
            Obstacle("ob_lane", ((29, -53), (43, -53), (43, -67), (29, -67))),
        ),
        waypoint_label="abe_transit",
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


FONT_TITLE = font(24, True)
FONT_LABEL = font(14, True)
FONT_SMALL = font(12)


def parse_nav(alog: Path) -> list[tuple[float, float, float, float]]:
    rows: list[tuple[float, str, str]] = []
    for line in alog.read_text(errors="ignore").splitlines():
        if not line or line.startswith("%"):
            continue
        parts = line.split()
        if len(parts) < 4 or parts[1] not in {"NAV_X", "NAV_Y", "NAV_HEADING"}:
            continue
        rows.append((float(parts[0]), parts[1], parts[3]))

    samples: list[tuple[float, float, float, float]] = []
    current_t: float | None = None
    current: dict[str, float] = {}
    for t, var, value in rows:
        if current_t is None or abs(t - current_t) > 0.001:
            if {"NAV_X", "NAV_Y", "NAV_HEADING"} <= current.keys():
                samples.append((current_t, current["NAV_X"], current["NAV_Y"], current["NAV_HEADING"]))
            current_t = t
            current = {}
        current[var] = float(value)

    if current_t is not None and {"NAV_X", "NAV_Y", "NAV_HEADING"} <= current.keys():
        samples.append((current_t, current["NAV_X"], current["NAV_Y"], current["NAV_HEADING"]))

    if not samples:
        raise ValueError(f"no NAV samples found in {alog}")
    return samples


def interp(samples: list[tuple[float, float, float, float]], t: float) -> tuple[float, float, float]:
    times = [row[0] for row in samples]
    idx = bisect.bisect_left(times, t)
    if idx <= 0:
        return samples[0][1], samples[0][2], samples[0][3]
    if idx >= len(samples):
        return samples[-1][1], samples[-1][2], samples[-1][3]
    a = samples[idx - 1]
    b = samples[idx]
    span = b[0] - a[0]
    f = 0.0 if span == 0 else (t - a[0]) / span
    x = a[1] + (b[1] - a[1]) * f
    y = a[2] + (b[2] - a[2]) * f
    h = a[3] + (b[3] - a[3]) * f
    return x, y, h


def xy_to_px(x: float, y: float) -> tuple[float, float]:
    min_x, min_y, max_x, max_y = MAP
    px = (x - min_x) / (max_x - min_x) * W
    py = H - ((y - min_y) / (max_y - min_y) * H)
    return px, py


def draw_background(draw: ImageDraw.ImageDraw) -> None:
    draw.rectangle((0, 0, W, H), fill=WATER_A)
    for x in range(0, W, 44):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, 44):
        draw.line((0, y, W, y), fill=GRID, width=1)
    for i in range(0, W, 80):
        draw.line((i, 0, i - 180, H), fill=WATER_B, width=1)


def draw_obstacle(draw: ImageDraw.ImageDraw, obstacle: Obstacle) -> None:
    pts = [xy_to_px(x, y) for x, y in obstacle.pts]
    cx = sum(x for x, _ in pts) / len(pts)
    cy = sum(y for _, y in pts) / len(pts)
    for pad, fill in ((34, "#464b57"), (22, "#555a66")):
        ring = []
        for x, y in pts:
            ang = math.atan2(y - cy, x - cx)
            ring.append((x + math.cos(ang) * pad, y + math.sin(ang) * pad))
        draw.polygon(ring, fill=fill, outline=OB_RING)
    draw.polygon(pts, fill=OB_FILL, outline=OB_EDGE)
    draw.text((cx - 18, cy - 8), obstacle.label, fill=WHITE, font=FONT_LABEL)


def draw_boat(draw: ImageDraw.ImageDraw, x: float, y: float, heading: float) -> None:
    px, py = xy_to_px(x, y)
    angle = math.radians(90 - heading)
    length = 28
    beam = 10
    pts = [
        (px + math.cos(angle) * length, py - math.sin(angle) * length),
        (px + math.cos(angle + 2.45) * beam, py - math.sin(angle + 2.45) * beam),
        (px + math.cos(angle - 2.45) * beam, py - math.sin(angle - 2.45) * beam),
    ]
    draw.polygon(pts, fill=YELLOW, outline="#fff7a6")
    draw.ellipse((px - 4, py - 4, px + 4, py + 4), fill="#24324b")
    draw.text((px + 8, py - 12), "abe", fill=WHITE, font=FONT_LABEL)


def draw_trail(draw: ImageDraw.ImageDraw, pts: list[tuple[float, float]]) -> None:
    if len(pts) < 2:
        return
    px_pts = [xy_to_px(x, y) for x, y in pts]
    for idx in range(1, len(px_pts)):
        if idx % 2:
            draw.line((px_pts[idx - 1], px_pts[idx]), fill=TRAIL, width=2)


def render_one(spec: GifSpec, samples: list[tuple[float, float, float, float]]) -> Path:
    frames: list[Image.Image] = []
    duration = spec.end_t - spec.start_t
    for idx in range(FRAMES):
        frac = idx / (FRAMES - 1)
        t = spec.start_t + duration * frac
        img = Image.new("RGB", (W, H), BG)
        draw = ImageDraw.Draw(img, "RGBA")
        draw_background(draw)
        for obstacle in spec.obstacles:
            draw_obstacle(draw, obstacle)

        trail_samples = [
            (x, y)
            for sample_t, x, y, _h in samples
            if spec.start_t <= sample_t <= t
        ]
        draw_trail(draw, trail_samples)
        x, y, heading = interp(samples, t)
        draw_boat(draw, x, y, heading)

        waypoint = xy_to_px(70, -60)
        draw.ellipse((waypoint[0] - 4, waypoint[1] - 4, waypoint[0] + 4, waypoint[1] + 4), fill=YELLOW)
        draw.text((waypoint[0] + 7, waypoint[1] + 8), spec.waypoint_label, fill=WHITE, font=FONT_LABEL)

        draw.rounded_rectangle((24, 24, 428, 88), radius=10, fill="#171b27cc", outline="#394156")
        draw.text((42, 34), spec.title, fill=TEXT, font=FONT_TITLE)
        draw.text((42, 64), spec.badge, fill=spec.badge_color, font=FONT_SMALL)
        frames.append(img)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUT_DIR / spec.output
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=FRAME_MS, loop=0)
    return out


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--alog-dir", type=Path, default=DEFAULT_ALOG_DIR)
    args = parser.parse_args()

    for spec in GIFS:
        alog = args.alog_dir / f"{spec.case}.alog"
        samples = parse_nav(alog)
        print(render_one(spec, samples))


if __name__ == "__main__":
    main()
