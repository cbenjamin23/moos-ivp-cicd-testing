#!/usr/bin/env python3
"""Render explanatory H04 COLREGS parameter comparison GIFs."""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 1200, 675
FRAMES = 64
FRAME_MS = 120

BG = "#242836"
GRID = "#2f3547"
GRID_STRONG = "#3b4358"
BORDER = "#566179"
TEXT = "#edf1f8"
MUTED = "#a8b1c4"
YELLOW = "#f3ea5f"
BLUE = "#70b9ff"
GREEN = "#7bd88f"
TAG_BG = "#1f2432"

PANELS = ((48, 126, 568, 615), (632, 126, 1152, 615))


@dataclass(frozen=True)
class PanelSpec:
    label: str
    setting: str
    yellow_path: tuple[tuple[float, float], ...]
    blue_path: tuple[tuple[float, float], ...]
    result_title: str
    result_detail: str


@dataclass(frozen=True)
class GifSpec:
    filename: str
    title: str
    subtitle: str
    panels: tuple[PanelSpec, PanelSpec]


GIFS = (
    GifSpec(
        filename="colregs-parameters-bow-distance.gif",
        title="Bow Distance Threshold",
        subtitle="same give-way geometry; high giveway_bow_dist demotes the encounter to CPA",
        panels=(
            PanelSpec(
                label="Default",
                setting="giveway_bow_dist=default",
                yellow_path=((0.22, 0.20), (0.58, 0.20)),
                blue_path=((0.68, 0.78), (0.68, 0.47)),
                result_title="IX 22: giveway:bow",
                result_detail="closest_range_ever=38.6m",
            ),
            PanelSpec(
                label="High",
                setting="giveway_bow_dist=12",
                yellow_path=((0.22, 0.20), (0.56, 0.20), (0.70, 0.17)),
                blue_path=((0.68, 0.78), (0.68, 0.47)),
                result_title="IX 50: cpa",
                result_detail="closest_range_ever=20.0m",
            ),
        ),
    ),
    GifSpec(
        filename="colregs-parameters-turn-radius.gif",
        title="Turn Radius Branch",
        subtitle="same turn-gap geometry; larger turn_radius changes the branch from stern to bow",
        panels=(
            PanelSpec(
                label="Default",
                setting="turn_radius=default",
                yellow_path=((0.28, 0.22), (0.42, 0.30), (0.54, 0.25), (0.70, 0.52)),
                blue_path=((0.55, 0.80), (0.48, 0.61), (0.42, 0.43), (0.32, 0.17)),
                result_title="IX 20: giveway:stern",
                result_detail="closest_range_ever=14.0m",
            ),
            PanelSpec(
                label="High",
                setting="turn_radius=high",
                yellow_path=((0.28, 0.30), (0.45, 0.37), (0.58, 0.52)),
                blue_path=((0.60, 0.78), (0.55, 0.62), (0.50, 0.50)),
                result_title="IX 22: giveway:bow",
                result_detail="closest_range_ever=38.6m",
            ),
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


FONT_TITLE = font(24, True)
FONT_SUBTITLE = font(17)
FONT_LABEL = font(26, True)
FONT_SETTING = font(18)
FONT_TAG = font(17, True)
FONT_SMALL = font(13)


def interp_path(points: list[tuple[float, float]], frac: float) -> tuple[tuple[float, float], list[tuple[float, float]]]:
    segments: list[tuple[tuple[float, float], tuple[float, float], float]] = []
    total = 0.0
    for a, b in zip(points, points[1:]):
        dist = math.hypot(b[0] - a[0], b[1] - a[1])
        segments.append((a, b, dist))
        total += dist

    remaining = frac * total
    drawn = [points[0]]
    for a, b, dist in segments:
        if remaining >= dist:
            drawn.append(b)
            remaining -= dist
            continue
        t = 0.0 if dist == 0 else remaining / dist
        point = (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)
        drawn.append(point)
        return point, drawn
    return points[-1], drawn


def heading_at(path: list[tuple[float, float]]) -> float:
    if len(path) < 2:
        return 0.0
    a, b = path[-2], path[-1]
    return math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))


def draw_triangle(draw: ImageDraw.ImageDraw, pos: tuple[float, float], heading: float, color: str) -> None:
    x, y = pos
    size = 16
    angle = math.radians(heading)
    points = [
        (x + math.cos(angle) * size, y + math.sin(angle) * size),
        (x + math.cos(angle + 2.45) * size * 0.72, y + math.sin(angle + 2.45) * size * 0.72),
        (x + math.cos(angle - 2.45) * size * 0.72, y + math.sin(angle - 2.45) * size * 0.72),
    ]
    outline = "#fff7a6" if color == YELLOW else "#c7e4ff"
    draw.polygon(points, fill=color, outline=outline)


def draw_grid(draw: ImageDraw.ImageDraw, plot: tuple[int, int, int, int]) -> None:
    left, top, right, bottom = plot
    draw.rectangle(plot, fill=BG, outline=BORDER, width=2)
    step = 32
    x = left + step
    while x < right:
        fill = GRID_STRONG if (x - left) // step % 4 == 0 else GRID
        draw.line((x, top, x, bottom), fill=fill, width=1)
        x += step
    y = top + step
    while y < bottom:
        fill = GRID_STRONG if (y - top) // step % 4 == 0 else GRID
        draw.line((left, y, right, y), fill=fill, width=1)
        y += step


def draw_panel(draw: ImageDraw.ImageDraw, panel: tuple[int, int, int, int], spec: PanelSpec) -> tuple[int, int, int, int]:
    left, top, right, bottom = panel
    draw.rounded_rectangle(panel, radius=3, fill=BG, outline=BORDER, width=2)
    draw.rectangle((left + 2, top + 2, right - 2, top + 72), fill=BG)
    draw.text((left + 15, top + 13), spec.label, fill=TEXT, font=FONT_LABEL)
    draw.text((left + 15, top + 45), spec.setting, fill=MUTED, font=FONT_SETTING)
    plot = (left + 15, top + 76, right - 15, bottom - 18)
    draw_grid(draw, plot)
    return plot


def draw_tag(draw: ImageDraw.ImageDraw, panel: tuple[int, int, int, int], spec: PanelSpec, frac: float) -> None:
    if frac < 0.55:
        return
    left, _top, _right, bottom = panel
    box = (left + 18, bottom - 82, left + 305, bottom - 20)
    draw.rounded_rectangle(box, radius=7, fill=TAG_BG, outline=GREEN, width=2)
    draw.text((box[0] + 12, box[1] + 9), spec.result_title, fill=TEXT, font=FONT_TAG)
    draw.text((box[0] + 12, box[1] + 34), spec.result_detail, fill=MUTED, font=FONT_SMALL)


def screen_path(plot: tuple[int, int, int, int], points: tuple[tuple[float, float], ...]) -> list[tuple[float, float]]:
    left, top, right, bottom = plot
    return [(left + x * (right - left), top + y * (bottom - top)) for x, y in points]


def render_gif(spec: GifSpec) -> Path:
    frames: list[Image.Image] = []
    for frame_idx in range(FRAMES):
        frac = frame_idx / (FRAMES - 1)
        ease = 0.5 - math.cos(frac * math.pi) / 2
        img = Image.new("RGB", (W, H), BG)
        draw = ImageDraw.Draw(img, "RGBA")

        draw.text((48, 36), spec.title, fill=TEXT, font=FONT_TITLE)
        draw.text((48, 70), spec.subtitle, fill=MUTED, font=FONT_SUBTITLE)

        for panel, panel_spec in zip(PANELS, spec.panels):
            plot = draw_panel(draw, panel, panel_spec)
            yellow_path = screen_path(plot, panel_spec.yellow_path)
            blue_path = screen_path(plot, panel_spec.blue_path)
            draw.line(yellow_path, fill=YELLOW + "55", width=2)
            draw.line(blue_path, fill=BLUE + "55", width=2)

            yellow_pos, yellow_done = interp_path(yellow_path, ease)
            blue_pos, blue_done = interp_path(blue_path, ease)
            if len(yellow_done) > 1:
                draw.line(yellow_done, fill=YELLOW, width=4)
            if len(blue_done) > 1:
                draw.line(blue_done, fill=BLUE, width=4)
            draw_triangle(draw, yellow_pos, heading_at(yellow_done), YELLOW)
            draw_triangle(draw, blue_pos, heading_at(blue_done), BLUE)
            draw_tag(draw, panel, panel_spec, frac)

        frames.append(img)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUT_DIR / spec.filename
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=FRAME_MS, loop=0)
    return out


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path, default=None, help="Accepted for compatibility; H04 GIFs are explanatory renderings.")
    parser.parse_args()
    for spec in GIFS:
        print(render_gif(spec))


if __name__ == "__main__":
    main()
