#!/usr/bin/env python3
"""Render generated GIFs for the uFldObstacleSim harness page.

These are compact documentation views of the source-side publication flow.
The headless harness and alogs remain the source of truth for pass/fail.
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 1200, 675
FRAMES = 72
FRAME_MS = 75

BG = "#202433"
GRID = "#2c3243"
TEXT = "#edf2f8"
MUTED = "#a8b2c4"
PANEL = "#262c3b"
PANEL_2 = "#2a3142"
REGION = "#7d879b"
OBSTACLE = "#f3f5fa"
OBSTACLE_FILL = "#f3f5fa55"
GREEN = "#76d58e"
TEAL = "#39c8b7"
YELLOW = "#f3ea5f"
ORANGE = "#e1a057"
RED = "#ed716b"
BLUE = "#7ca8ff"
WHITE = "#f8fbff"

MAP = (56, 124, 710, 595)
WORLD = (-6.0, 68.0, -38.0, 38.0)
REGION_POLY = [(0.0, -30.0), (60.0, -30.0), (60.0, 30.0), (0.0, 30.0)]
OBSTACLE_POLY = [(10.0, -10.0), (14.0, -10.0), (14.0, -6.0), (10.0, -6.0)]
POINT_PATH = [(10.0, -9.2), (10.0, -8.4), (10.0, -7.6), (10.0, -6.8), (11.0, -6.0)]


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


TITLE = font(34, True)
SUB = font(19)
LABEL = font(18, True)
SMALL = font(15)
MONO = font(16)


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, v))


def ease(v: float) -> float:
    v = clamp(v)
    return v * v * (3.0 - 2.0 * v)


def world_to_screen(p: tuple[float, float]) -> tuple[float, float]:
    x0, y0, x1, y1 = MAP
    xmin, xmax, ymin, ymax = WORLD
    x, y = p
    sx = x0 + (x - xmin) / (xmax - xmin) * (x1 - x0)
    sy = y1 - (y - ymin) / (ymax - ymin) * (y1 - y0)
    return sx, sy


def text_size(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.ImageFont) -> tuple[int, int]:
    box = draw.textbbox((0, 0), text, font=fnt)
    return box[2] - box[0], box[3] - box[1]


def base(title: str, subtitle: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, 60):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, 60):
        draw.line((0, y, W, y), fill=GRID, width=1)
    draw.text((40, 28), title, fill=TEXT, font=TITLE)
    draw.text((40, 72), subtitle, fill=MUTED, font=SUB)
    return im, draw


def panel(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str) -> None:
    draw.rounded_rectangle(xy, radius=8, fill=PANEL, outline="#434b61", width=2)
    draw.text((xy[0] + 20, xy[1] + 18), title, fill=TEXT, font=LABEL)


def draw_map(draw: ImageDraw.ImageDraw, caption: str) -> None:
    panel(draw, MAP, caption)
    x0, y0, x1, y1 = MAP
    for gx in range(0, 70, 10):
        a = world_to_screen((gx, -30))
        b = world_to_screen((gx, 30))
        draw.line((*a, *b), fill="#353c50", width=1)
    for gy in range(-30, 31, 10):
        a = world_to_screen((0, gy))
        b = world_to_screen((60, gy))
        draw.line((*a, *b), fill="#353c50", width=1)
    region = [world_to_screen(p) for p in REGION_POLY]
    draw.polygon(region, outline=REGION, fill="#00000000")
    draw.line((*region[-1], *region[0]), fill=REGION, width=2)
    for a, b in zip(region, region[1:]):
        draw.line((*a, *b), fill=REGION, width=2)
    draw.text((x1 - 142, y1 - 33), "obs_region", fill=MUTED, font=SMALL)


def draw_obstacle(draw: ImageDraw.ImageDraw, pulse: float = 1.0) -> None:
    pts = [world_to_screen(p) for p in OBSTACLE_POLY]
    outline = OBSTACLE if pulse < 0.5 else GREEN
    width = 3 if pulse < 0.5 else 5
    draw.polygon(pts, fill=OBSTACLE_FILL, outline=outline)
    for a, b in zip(pts, pts[1:] + pts[:1]):
        draw.line((*a, *b), fill=outline, width=width)
    sx, sy = world_to_screen((14.8, -8))
    draw.text((sx + 6, sy - 12), "ufos_a", fill=TEXT, font=SMALL)


def draw_vehicle(draw: ImageDraw.ImageDraw, p: tuple[float, float], color: str = YELLOW, label: str = "alpha") -> None:
    x, y = world_to_screen(p)
    pts = [(x, y - 15), (x - 12, y + 12), (x + 12, y + 12)]
    draw.polygon(pts, fill=color, outline=WHITE)
    draw.text((x - 42, y + 18), label, fill=TEXT, font=SMALL)


def status_box(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int, int, int],
    label: str,
    value: str,
    active: bool,
    accent: str = GREEN,
) -> None:
    fill = PANEL_2 if active else "#252a38"
    outline = accent if active else "#495066"
    draw.rounded_rectangle(xy, radius=7, fill=fill, outline=outline, width=2 if active else 1)
    draw.text((xy[0] + 16, xy[1] + 12), label, fill=accent if active else MUTED, font=LABEL)
    draw.text((xy[0] + 16, xy[1] + 39), value, fill=TEXT if active else MUTED, font=MONO)


def arrow(draw: ImageDraw.ImageDraw, start: tuple[int, int], end: tuple[int, int], active: bool) -> None:
    color = GREEN if active else "#555d73"
    draw.line((*start, *end), fill=color, width=4)
    ang = math.atan2(end[1] - start[1], end[0] - start[0])
    head = [
        (end[0] + math.cos(ang + 2.55) * 14, end[1] + math.sin(ang + 2.55) * 14),
        (end[0] + math.cos(ang - 2.55) * 14, end[1] + math.sin(ang - 2.55) * 14),
    ]
    draw.polygon([end, *head], fill=color)


def timeline(draw: ImageDraw.ImageDraw, y: int, p: float, milestones: list[tuple[str, float]]) -> None:
    x0, x1 = 744, 1144
    draw.line((x0, y, x1, y), fill="#555d73", width=3)
    draw.line((x0, y, x0 + (x1 - x0) * p, y), fill=GREEN, width=5)
    for label, checkpoint in milestones:
        x = x0 + (x1 - x0) * checkpoint
        active = p >= checkpoint
        draw.ellipse((x - 8, y - 8, x + 8, y + 8), fill=GREEN if active else BG, outline=GREEN, width=2)
        tw, _ = text_size(draw, label, SMALL)
        draw.text((x - tw / 2, y + 16), label, fill=TEXT if active else MUTED, font=SMALL)


def reached(p: float, milestone: tuple[str, float]) -> bool:
    return p >= milestone[1]


def save(frames: list[Image.Image], filename: str) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    durations = [FRAME_MS] * len(frames)
    durations[-1] += 1000
    frames[0].save(
        OUT_DIR / filename,
        save_all=True,
        append_images=frames[1:],
        duration=durations,
        loop=0,
        optimize=True,
    )


def render_fixed_field() -> None:
    milestones = [
        ("connect", 0.12),
        ("truth", 0.34),
        ("given", 0.50),
        ("visual", 0.66),
        ("pass", 0.88),
    ]
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        p = ease(i / (FRAMES - 1))
        im, draw = base("uFldObstacleSim fixed field", "PMV_CONNECT refresh publishes source truth and visuals")
        draw_map(draw, "configured field")
        draw_obstacle(draw, pulse=clamp((p - milestones[1][1]) / 0.12))
        status_box(draw, (760, 142, 1128, 206), "trigger", "PMV_CONNECT = true", reached(p, milestones[0]), BLUE)
        status_box(draw, (760, 246, 1128, 310), "ground truth", "KNOWN_OBSTACLE label=ufos_a", reached(p, milestones[1]), GREEN)
        status_box(draw, (760, 330, 1128, 394), "vehicle truth", "GIVEN_OBSTACLE label=ufos_a", reached(p, milestones[2]), TEAL)
        status_box(draw, (760, 434, 1128, 498), "visuals", "VIEW_POLYGON + UFOS_MIN_RNG", reached(p, milestones[3]), ORANGE)
        status_box(draw, (760, 518, 1128, 582), "grade", "pMissionEval: pass", reached(p, milestones[4]), GREEN)
        arrow(draw, (944, 206), (944, 246), reached(p, milestones[1]))
        arrow(draw, (944, 394), (944, 434), reached(p, milestones[3]))
        timeline(draw, 628, p, milestones)
        frames.append(im)
    save(frames, "ufld-obstacle-sim-fixed-field.gif")


def render_point_sensor() -> None:
    milestones = [
        ("report", 0.12),
        ("truth", 0.30),
        ("points", 0.50),
        ("visual", 0.66),
        ("no given", 0.88),
    ]
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        p = ease(i / (FRAMES - 1))
        im, draw = base("uFldObstacleSim point sensor", "post_points=true reports simulated features, not GIVEN_OBSTACLE")
        draw_map(draw, "point mode")
        draw_obstacle(draw, pulse=0.0)
        draw_vehicle(draw, (8.0, -8.0))
        cx, cy = world_to_screen((8.0, -8.0))
        ring = 66 + 10 * math.sin(p * math.tau * 2)
        draw.ellipse((cx - ring, cy - ring, cx + ring, cy + ring), outline="#f3ea5f88", width=2)
        point_count = min(len(POINT_PATH), max(0, int((p - milestones[2][1]) / 0.07) + 1))
        for idx, pt in enumerate(POINT_PATH[:point_count]):
            x, y = world_to_screen(pt)
            r = 5 + idx
            draw.ellipse((x - r, y - r, x + r, y + r), fill=YELLOW, outline=WHITE, width=1)
        status_box(draw, (760, 142, 1128, 206), "vehicle ledger", "NODE_REPORT alpha near ufos_a", reached(p, milestones[0]), BLUE)
        status_box(draw, (760, 246, 1128, 310), "truth refresh", "KNOWN_OBSTACLE label=ufos_a", reached(p, milestones[1]), GREEN)
        status_box(draw, (760, 330, 1128, 394), "sensor output", "TRACKED_FEATURE_ALPHA key=ufos_a", reached(p, milestones[2]), YELLOW)
        status_box(draw, (760, 414, 1128, 478), "point visual", "VIEW_POINT label=alpha:ufos_a", reached(p, milestones[3]), ORANGE)
        status_box(draw, (760, 518, 1128, 582), "suppressed", "GIVEN_OBSTACLE absent", reached(p, milestones[4]), RED)
        timeline(draw, 628, p, milestones)
        frames.append(im)
    save(frames, "ufld-obstacle-sim-point-sensor.gif")


def main() -> None:
    render_fixed_field()
    render_point_sensor()


if __name__ == "__main__":
    main()
