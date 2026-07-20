#!/usr/bin/env python3
"""Render generated GIFs for the BHV_MemoryTurnLimit harness page.

These are deterministic documentation views of the MemoryTurnLimit motion
contract. The headless harness runs remain the source of truth; these GIFs make
the recent-heading memory, commanded turn, and graded telemetry relationship
legible in a way a raw pMarineViewer capture does not.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 960, 540
FRAMES = 72
FRAME_MS = 90

BG = "#202433"
WATER_A = "#222638"
WATER_B = "#1f2331"
GRID = "#30364a"
PANEL = "#252a3a"
PANEL_2 = "#2a3042"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
YELLOW = "#f3ea5f"
WHITE = "#f5f7fb"
GREEN = "#75d38b"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
RED = "#f06c64"
BLUE = "#70b9ff"
PURPLE = "#9b8cff"

MAP_RECT = (34, 52, 620, 504)
PANEL_X0 = 652
PANEL_X1 = 930
MAP_BOUNDS = (-78.0, -150.0, 58.0, -35.0)
START = (0.0, -120.0)
INITIAL_HEADING = 90.0
REQUESTED_HEADING = 180.0


@dataclass(frozen=True)
class Spec:
    output: str
    title: str
    case_name: str
    subtitle: str
    turn_range_start: float
    turn_range_end: float
    update_t: float | None
    final_heading: float
    final_mem_avg: float
    final_mismatch: float
    final_speed: float
    badge: str
    color: str
    unique_lines: tuple[str, str]


SPECS = (
    Spec(
        output="memoryturnlimit-behavior-tight-window.gif",
        title="Tight Turn Window",
        case_name="tight_window_pass",
        subtitle="narrow range keeps the vehicle near recent heading memory",
        turn_range_start=6.0,
        turn_range_end=6.0,
        update_t=None,
        final_heading=97.0,
        final_mem_avg=95.0,
        final_mismatch=83.0,
        final_speed=1.27,
        badge="damped turn passes",
        color=TEAL,
        unique_lines=(
            "turn_range stays tight: 6 deg",
            "large mismatch is expected and graded",
        ),
    ),
    Spec(
        output="memoryturnlimit-behavior-runtime-widen.gif",
        title="Runtime Range Widen",
        case_name="runtime_widen_range_pass",
        subtitle="MEMORY_TURN_UPDATES widens the limiter after startup",
        turn_range_start=6.0,
        turn_range_end=120.0,
        update_t=5.0,
        final_heading=157.0,
        final_mem_avg=140.0,
        final_mismatch=24.0,
        final_speed=1.08,
        badge="runtime update accepted",
        color=GREEN,
        unique_lines=(
            "turn_range changes: 6 -> 120 deg",
            "heading advances after update @ 5s",
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


FONT_TITLE = font(28, True)
FONT_LABEL = font(16, True)
FONT_SMALL = font(13)
FONT_TINY = font(11)


def clamp(value: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, value))


def ease(value: float) -> float:
    value = clamp(value)
    return value * value * (3 - 2 * value)


def map_xy(x: float, y: float) -> tuple[float, float]:
    min_x, min_y, max_x, max_y = MAP_BOUNDS
    px = MAP_RECT[0] + (x - min_x) / (max_x - min_x) * (MAP_RECT[2] - MAP_RECT[0])
    py = MAP_RECT[3] - (y - min_y) / (max_y - min_y) * (MAP_RECT[3] - MAP_RECT[1])
    return px, py


def endpoint(x: float, y: float, heading: float, length: float) -> tuple[float, float]:
    rad = math.radians(heading)
    return x + math.cos(rad) * length, y + math.sin(rad) * length


def heading_at(spec: Spec, t: float, t_max: float) -> float:
    if spec.update_t is None:
        return INITIAL_HEADING + (spec.final_heading - INITIAL_HEADING) * ease(t / t_max)
    if t < spec.update_t:
        return INITIAL_HEADING + 7.0 * ease(t / spec.update_t)
    f = (t - spec.update_t) / (t_max - spec.update_t)
    return 97.0 + (spec.final_heading - 97.0) * ease(f)


def mem_avg_at(spec: Spec, t: float, t_max: float) -> float:
    if spec.update_t is None:
        return INITIAL_HEADING + (spec.final_mem_avg - INITIAL_HEADING) * ease(t / t_max)
    if t < spec.update_t:
        return INITIAL_HEADING + 5.0 * ease(t / spec.update_t)
    f = (t - spec.update_t) / (t_max - spec.update_t)
    return 95.0 + (spec.final_mem_avg - 95.0) * ease(f)


def current_turn_range(spec: Spec, t: float) -> float:
    if spec.update_t is not None and t >= spec.update_t:
        return spec.turn_range_end
    return spec.turn_range_start


def motion_points(spec: Spec, t: float, t_max: float) -> list[tuple[float, float]]:
    points = [START]
    steps = 95
    dt = t_max / steps
    step_len = 0.56
    x, y = START
    for i in range(1, steps + 1):
        sample_t = dt * i
        if sample_t > t:
            break
        heading = heading_at(spec, sample_t, t_max)
        rad = math.radians(heading)
        x += math.cos(rad) * step_len
        y += math.sin(rad) * step_len
        points.append((x, y))
    return points


def draw_background(draw: ImageDraw.ImageDraw) -> None:
    draw.rectangle((0, 0, W, H), fill=BG)
    draw.rectangle(MAP_RECT, fill=WATER_A)
    for x in range(MAP_RECT[0], MAP_RECT[2] + 1, 44):
        draw.line((x, MAP_RECT[1], x, MAP_RECT[3]), fill=GRID, width=1)
    for y in range(MAP_RECT[1], MAP_RECT[3] + 1, 44):
        draw.line((MAP_RECT[0], y, MAP_RECT[2], y), fill=GRID, width=1)
    for x in range(MAP_RECT[0] - 150, MAP_RECT[2] + 180, 96):
        draw.line((x, MAP_RECT[1], x - 180, MAP_RECT[3]), fill=WATER_B, width=1)
    draw.rectangle(MAP_RECT, outline="#4a5062", width=2)


def draw_guides(draw: ImageDraw.ImageDraw) -> None:
    sx, sy = START
    for heading, length, color, label, offset in (
        (INITIAL_HEADING, 40.0, BLUE, "initial 90", (8, -16)),
        (REQUESTED_HEADING, 42.0, YELLOW, "request 180", (-92, 10)),
    ):
        ex, ey = endpoint(sx, sy, heading, length)
        a = map_xy(sx, sy)
        b = map_xy(ex, ey)
        draw.line((a, b), fill=color, width=3)
        draw.ellipse((a[0] - 4, a[1] - 4, a[0] + 4, a[1] + 4), fill=color)
        draw.text((b[0] + offset[0], b[1] + offset[1]), label, fill=color, font=FONT_SMALL)


def draw_heading_ray(
    draw: ImageDraw.ImageDraw,
    x: float,
    y: float,
    heading: float,
    length: float,
    color: str,
) -> None:
    ex, ey = endpoint(x, y, heading, length)
    a = map_xy(x, y)
    b = map_xy(ex, ey)
    draw.line((a, b), fill=color, width=2)
    draw.ellipse((b[0] - 3, b[1] - 3, b[0] + 3, b[1] + 3), fill=color)


def draw_boat(draw: ImageDraw.ImageDraw, x: float, y: float, heading: float) -> None:
    px, py = map_xy(x, y)
    angle = math.radians(heading)
    pts = [
        (px + math.cos(angle) * 17, py - math.sin(angle) * 17),
        (px + math.cos(angle + 2.45) * 8, py - math.sin(angle + 2.45) * 8),
        (px + math.cos(angle - 2.45) * 8, py - math.sin(angle - 2.45) * 8),
    ]
    draw.polygon(pts, fill=YELLOW, outline="#fff7a6")


def draw_simple_value(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int],
    label: str,
    value: str,
    color: str,
) -> None:
    x, y = xy
    draw.rounded_rectangle((x, y, x + 128, y + 52), radius=6, fill=PANEL, outline="#3d4558", width=1)
    draw.text((x + 10, y + 8), label, fill=MUTED, font=FONT_TINY)
    draw.text((x + 10, y + 26), value, fill=color, font=FONT_LABEL)


def draw_panel(draw: ImageDraw.ImageDraw, spec: Spec, t: float, t_max: float) -> None:
    heading = heading_at(spec, t, t_max)
    mem_avg = mem_avg_at(spec, t, t_max)
    mismatch = abs(REQUESTED_HEADING - heading)
    speed = spec.final_speed * (0.65 + 0.35 * ease(t / 8.0))
    turn_range = current_turn_range(spec, t)
    draw.text((PANEL_X0, 54), spec.title, fill=TEXT, font=FONT_TITLE)
    draw.text((PANEL_X0, 87), spec.case_name, fill=MUTED, font=FONT_TINY)
    draw.rounded_rectangle((PANEL_X0, 124, PANEL_X1, 198), radius=8, fill=PANEL_2, outline=spec.color, width=2)
    draw.text((PANEL_X0 + 12, 136), spec.unique_lines[0], fill=spec.color, font=FONT_LABEL)
    draw.text((PANEL_X0 + 12, 164), spec.unique_lines[1], fill=TEXT, font=FONT_SMALL)
    draw_simple_value(draw, (PANEL_X0, 230), "turn_range", f"{turn_range:0.0f} deg", PURPLE)
    draw_simple_value(draw, (PANEL_X0 + 146, 230), "MEM_HDG_AVG", f"{mem_avg:0.0f} deg", TEAL)
    draw_simple_value(draw, (PANEL_X0, 302), "NAV_HEADING", f"{heading:0.0f} deg", YELLOW)
    draw_simple_value(draw, (PANEL_X0 + 146, 302), "MISMATCH", f"{mismatch:0.0f} deg", ORANGE)
    draw.rounded_rectangle((PANEL_X0, 388, PANEL_X1, 452), radius=8, fill=PANEL, outline="#3d4558", width=1)
    draw.text((PANEL_X0 + 12, 402), "colored rays", fill=TEXT, font=FONT_LABEL)
    draw.text((PANEL_X0 + 12, 426), "yellow target  |  teal memory  |  green nav", fill=MUTED, font=FONT_TINY)
    draw.text((PANEL_X0, 492), f"NAV_SPEED {speed:0.2f} m/s    warning=false    bhv_error=false", fill=MUTED, font=FONT_TINY)


def draw_frame(spec: Spec, frame_idx: int) -> Image.Image:
    t_max = 30.0
    t = t_max * frame_idx / (FRAMES - 1)
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    draw_background(draw)
    draw_guides(draw)
    points = motion_points(spec, t, t_max)
    px_points = [map_xy(x, y) for x, y in points]
    if len(px_points) > 1:
        draw.line(px_points, fill="#c8cfd9", width=3)
    x, y = points[-1]
    heading = heading_at(spec, t, t_max)
    mem_avg = mem_avg_at(spec, t, t_max)
    draw_heading_ray(draw, x, y, REQUESTED_HEADING, 22.0, YELLOW)
    draw_heading_ray(draw, x, y, mem_avg, 19.0, TEAL)
    draw_heading_ray(draw, x, y, heading, 16.0, GREEN)
    draw_boat(draw, x, y, heading)
    draw.text((MAP_RECT[0] + 16, MAP_RECT[1] + 14), "heading memory limits requested turn", fill=TEXT, font=FONT_LABEL)
    draw_panel(draw, spec, t, t_max)
    draw.text((34, 516), f"H01 MemoryTurnLimit Behavior | {spec.case_name}", fill=MUTED, font=FONT_TINY)
    return im


def save_gif(frames: list[Image.Image], filename: str) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUT_DIR / filename
    frames[0].save(
        out,
        save_all=True,
        append_images=frames[1:],
        duration=FRAME_MS,
        loop=0,
        optimize=True,
    )
    print(out)


def main() -> None:
    for spec in SPECS:
        frames = [draw_frame(spec, i) for i in range(FRAMES)]
        save_gif(frames, spec.output)


if __name__ == "__main__":
    main()
