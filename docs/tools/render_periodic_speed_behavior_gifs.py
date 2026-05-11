#!/usr/bin/env python3
"""Render generated GIFs for the BHV_PeriodicSpeed harness page.

These are deterministic documentation views of the PeriodicSpeed timing story.
They are not pMarineViewer captures; the headless harness runs remain the
source of truth for pass/fail behavior.
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 1600, 900
FRAMES = 92
FRAME_MS = 95

BG = "#202433"
PANEL = "#252a3a"
PANEL_2 = "#2a3042"
GRID = "#2c3242"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
YELLOW = "#f3ea5f"
GREEN = "#75d38b"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
RED = "#f06c64"
WHITE = "#f5f7fb"
GRAY = "#646d82"


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


FONT_TITLE = font(44, True)
FONT_H2 = font(30, True)
FONT_LABEL = font(25, True)
FONT_TEXT = font(24)
FONT_SMALL = font(21)
FONT_TINY = font(18)
FONT_BIG_VALUE = font(58, True)


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, v))


def save_gif(frames: list[Image.Image], filename: str) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        OUT_DIR / filename,
        save_all=True,
        append_images=frames[1:],
        duration=FRAME_MS,
        loop=0,
        optimize=True,
    )


def draw_base(title: str, subtitle: str, case_name: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, 96):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, 96):
        draw.line((0, y, W, y), fill=GRID, width=1)
    draw.text((52, 38), title, fill=TEXT, font=FONT_TITLE)
    draw.text((52, 92), subtitle, fill=MUTED, font=FONT_TEXT)
    return im, draw


def draw_box(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str, detail: str, color: str) -> None:
    draw.rounded_rectangle(xy, radius=8, fill=PANEL, outline=color, width=2)
    draw.text((xy[0] + 22, xy[1] + 18), title, fill=color, font=FONT_LABEL)
    draw.text((xy[0] + 22, xy[1] + 58), detail, fill=TEXT, font=FONT_SMALL)


def draw_metric(draw: ImageDraw.ImageDraw, xy: tuple[int, int], label: str, value: str, color: str) -> None:
    x, y = xy
    draw.text((x, y), label, fill=MUTED, font=FONT_TINY)
    draw.text((x, y + 26), value, fill=color, font=FONT_BIG_VALUE)


def draw_evidence(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int, int, int],
    title: str,
    rows: tuple[tuple[str, str, str], ...],
    offsets: tuple[int, ...] | None = None,
) -> None:
    draw.rounded_rectangle(xy, radius=8, fill=PANEL_2, outline="#465069", width=2)
    x, y = xy[0] + 24, xy[1] + 20
    draw.text((x, y), title, fill=TEXT, font=FONT_LABEL)
    for idx, (label, value, color) in enumerate(rows):
        offset = offsets[idx] if offsets else idx * 138
        draw_metric(draw, (x + offset, y + 52), label, value, color)


def draw_timeline(
    draw: ImageDraw.ImageDraw,
    x: int,
    y: int,
    width: int,
    t: float,
    t_max: float,
    segments: tuple[tuple[float, float, str, str], ...],
    markers: tuple[tuple[float, str, str], ...],
    label: str,
) -> None:
    draw.rounded_rectangle((x - 28, y - 64, x + width + 28, y + 74), radius=8, fill=PANEL, outline="#465069", width=2)
    draw.text((x, y - 42), label, fill=TEXT, font=FONT_LABEL)
    draw.line((x, y, x + width, y), fill="#586175", width=5)
    for start, end, color, text in segments:
        sx = x + int(width * start / t_max)
        ex = x + int(width * end / t_max)
        draw.line((sx, y, ex, y), fill=color, width=18)
        cx = x + int(width * ((start + end) / 2) / t_max)
        draw.text((cx - 36, y + 30), text, fill=color, font=FONT_TINY)
    tx = x + int(width * clamp(t / t_max))
    draw.ellipse((tx - 17, y - 17, tx + 17, y + 17), fill=YELLOW, outline=WHITE, width=3)
    for mt, text, color in markers:
        mx = x + int(width * mt / t_max)
        draw.line((mx, y - 30, mx, y + 30), fill=color, width=3)
        draw.text((mx - 42, y - 60), text, fill=color, font=FONT_TINY)


def state_baseline(t: float) -> tuple[str, int, float, float, float]:
    if t < 3.5:
        return "deploying", 0, 0, 0, 0.0
    bt = t - 3.5
    if bt < 4:
        return "lazy", 0, 4 - bt, 0, 0.0
    if bt < 8:
        return "busy", 1, 0, 8 - bt, 1.4
    if bt < 12:
        return "lazy", 1, 12 - bt, 0, 0.0
    if bt < 16:
        return "busy", 2, 0, 16 - bt, 1.4
    return "lazy", 2, 20 - bt, 0, 0.0


def draw_map_baseline(draw: ImageDraw.ImageDraw, t: float) -> None:
    rect = (54, 166, 1038, 670)
    draw.rounded_rectangle(rect, radius=8, fill="#22283a", outline="#4a5062", width=2)
    for x in range(rect[0] + 64, rect[2], 96):
        draw.line((x, rect[1], x, rect[3]), fill="#30364a", width=1)
    for y in range(rect[1] + 64, rect[3], 96):
        draw.line((rect[0], y, rect[2], y), fill="#30364a", width=1)
    start, end = (150, 600), (910, 250)
    draw.line((start[0], start[1], end[0], end[1]), fill=GRAY, width=5)
    draw.text((86, 190), "constant heading track", fill=MUTED, font=FONT_SMALL)

    state, _, _, _, speed = state_baseline(t)
    bt = max(0.0, t - 3.5)
    distance = 0.18 * min(bt, 4) + 1.0 * max(0, min(bt, 8) - 4) + 0.18 * max(0, min(bt, 12) - 8) + 1.0 * max(0, bt - 12)
    f = clamp(distance / 10.0)
    bx = start[0] + (end[0] - start[0]) * f
    by = start[1] + (end[1] - start[1]) * f
    color = GREEN if state == "busy" else ORANGE if state == "lazy" else GRAY
    draw.line((start[0], start[1], bx, by), fill=color, width=10)
    angle = math.radians(45)
    pts = [
        (bx + math.cos(angle) * 28, by - math.sin(angle) * 28),
        (bx + math.cos(angle + 2.45) * 13, by - math.sin(angle + 2.45) * 13),
        (bx + math.cos(angle - 2.45) * 13, by - math.sin(angle - 2.45) * 13),
    ]
    draw.polygon(pts, fill=YELLOW, outline="#fff7a6")
    draw.text((bx + 24, by - 14), f"speed {speed:0.1f}", fill=WHITE, font=FONT_SMALL)


def render_baseline_cycle() -> None:
    frames: list[Image.Image] = []
    t_max = 18.8
    for i in range(FRAMES):
        t = t_max * i / (FRAMES - 1)
        state, count, pending_active, pending_inactive, speed = state_baseline(t)
        im, draw = draw_base(
            "Lazy/busy speed cycle",
            "Deployment settles first; evaluation waits for the second busy speed window",
            "baseline_cycle_pass",
        )
        draw_map_baseline(draw, t)
        draw_box(
            draw,
            (1088, 166, 1526, 302),
            "current mode",
            "busy: speed IvP function" if state == "busy" else "lazy: no speed IvP function" if state == "lazy" else "deployment settling",
            GREEN if state == "busy" else ORANGE if state == "lazy" else GRAY,
        )
        draw_evidence(
            draw,
            (1088, 334, 1526, 670),
            "pass evidence",
            (
                ("busy count", str(count), GREEN),
                ("speed", f"{speed:0.1f}", TEAL),
            ),
        )
        draw.text((1112, 584), "Evaluation waits for count >= 2", fill=MUTED, font=FONT_SMALL)
        draw.text((1112, 616), "and positive NAV_SPEED.", fill=MUTED, font=FONT_SMALL)
        draw_timeline(
            draw,
            92,
            782,
            1390,
            t,
            t_max,
            ((0, 3.5, GRAY, "deploy"), (3.5, 7.5, ORANGE, "lazy"), (7.5, 11.5, GREEN, "busy"), (11.5, 15.5, ORANGE, "lazy"), (15.5, 18.8, GREEN, "busy")),
            ((7.5, "count=1", GREEN), (15.5, "count=2", GREEN), (18.5, "eval", TEAL)),
            "mission time",
        )
        frames.append(im)
    save_gif(frames, "periodic-speed-baseline-cycle.gif")


def reset_state(t: float) -> tuple[bool, str, int, float, float, float]:
    run_ps = t < 5 or t >= 20
    if t < 14:
        return run_ps, "lazy", 0, 14 - t, 0, 0.0
    speed = 1.4 if run_ps else 0.0
    return run_ps, "busy", 1, 0, 30 - t, speed


def draw_reset_diagram(draw: ImageDraw.ImageDraw, t: float) -> None:
    rect = (54, 166, 1038, 754)
    draw.rounded_rectangle(rect, radius=8, fill="#22283a", outline="#4a5062", width=2)
    draw.text((86, 190), "reset_upon_running=false", fill=TEXT, font=FONT_LABEL)
    draw.text((86, 224), "The timer keeps aging while RUN_PS_ALL is false.", fill=MUTED, font=FONT_SMALL)
    x0, y0, width = 130, 414, 820
    draw.line((x0, y0 - 70, x0 + int(width * 5 / 27), y0 - 70), fill=GREEN, width=22)
    draw.line((x0 + int(width * 5 / 27), y0 - 70, x0 + int(width * 20 / 27), y0 - 70), fill=RED, width=22)
    draw.line((x0 + int(width * 20 / 27), y0 - 70, x0 + width, y0 - 70), fill=GREEN, width=22)
    draw.text((86, y0 - 148), "RUN_PS_ALL", fill=TEXT, font=FONT_LABEL)
    draw.line((x0, y0 + 64, x0 + int(width * 14 / 27), y0 + 64), fill=ORANGE, width=22)
    draw.line((x0 + int(width * 14 / 27), y0 + 64, x0 + width, y0 + 64), fill=GREEN, width=22)
    draw.text((x0, y0 + 102), "periodic timer", fill=MUTED, font=FONT_SMALL)
    for mt, label, color in ((5, "off", RED), (14, "busy", GREEN), (20, "on", GREEN), (27, "eval", TEAL)):
        mx = x0 + int(width * mt / 27)
        draw.line((mx, y0 - 136, mx, y0 + 124), fill=color, width=3)
        draw.text((mx - 28, y0 + 144), label, fill=color, font=FONT_TINY)
    tx = x0 + int(width * clamp(t / 27))
    draw.ellipse((tx - 18, y0 - 18, tx + 18, y0 + 18), fill=YELLOW, outline=WHITE, width=3)
    run_ps, mode, _, _, _, speed = reset_state(t)
    path_y = 682
    draw.line((140, path_y, 900, path_y), fill=GRAY, width=5)
    move = clamp(max(0, t - 20) / 7)
    bx = 140 + 760 * move
    draw.line((140, path_y, bx, path_y), fill=GREEN if speed else GRAY, width=10)
    pts = [(bx + 28, path_y), (bx - 14, path_y - 14), (bx - 14, path_y + 14)]
    draw.polygon(pts, fill=YELLOW, outline="#fff7a6")
    if speed:
        draw.text((bx - 124, path_y + 24), "speed resumes", fill=WHITE, font=FONT_SMALL)
    else:
        draw.text((210, path_y - 42), "no speed objective", fill=WHITE, font=FONT_SMALL)


def render_reset_false() -> None:
    frames: list[Image.Image] = []
    t_max = 27.0
    for i in range(FRAMES):
        t = t_max * i / (FRAMES - 1)
        run_ps, mode, count, pending_active, pending_inactive, speed = reset_state(t)
        im, draw = draw_base(
            "Reset false re-enable",
            "The periodic timer keeps aging while the behavior condition is false",
            "reset_false_visual_pass",
        )
        draw_reset_diagram(draw, t)
        draw_box(
            draw,
            (1088, 166, 1526, 302),
            "visible state",
            ("RUN_PS true, " if run_ps else "RUN_PS false, ") + mode,
            GREEN if run_ps and mode == "busy" else RED if not run_ps else ORANGE,
        )
        draw_evidence(
            draw,
            (1088, 334, 1526, 670),
            "pass evidence",
            (
                ("RUN_PS", "true" if run_ps else "false", GREEN if run_ps else RED),
                ("count", str(count), GREEN),
                ("speed", f"{speed:0.1f}", TEAL),
            ),
            offsets=(0, 200, 318),
        )
        draw.text((1112, 584), "Count increments while disabled;", fill=MUTED, font=FONT_SMALL)
        draw.text((1112, 616), "speed returns after re-enable.", fill=MUTED, font=FONT_SMALL)
        frames.append(im)
    save_gif(frames, "periodic-speed-reset-false-reenable.gif")


def main() -> None:
    render_baseline_cycle()
    render_reset_false()


if __name__ == "__main__":
    main()
