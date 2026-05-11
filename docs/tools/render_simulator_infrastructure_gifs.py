#!/usr/bin/env python3
"""Render generated GIFs for the simulator infrastructure harness pages.

These are deterministic documentation views, not pMarineViewer captures. The
headless harnesses remain the source of truth for pass/fail behavior.
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 1200, 675
FRAMES = 52
FRAME_MS = 90

BG = "#202433"
PANEL = "#252a3a"
PANEL_2 = "#2a3042"
GRID = "#2d3344"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
GREEN = "#76d58e"
TEAL = "#39c8b7"
YELLOW = "#f5ec60"
ORANGE = "#dc9852"
RED = "#f06c64"
BLUE = "#79a7ff"
GRAY = "#697287"
WHITE = "#f8fbff"


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


TITLE = font(38, True)
H2 = font(26, True)
LABEL = font(20, True)
BODY = font(19)
SMALL = font(16)
VALUE = font(48, True)
MONO = font(18)


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, v))


def ease(v: float) -> float:
    v = clamp(v)
    return v * v * (3.0 - 2.0 * v)


def text_size(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.ImageFont) -> tuple[int, int]:
    box = draw.textbbox((0, 0), text, font=fnt)
    return box[2] - box[0], box[3] - box[1]


def base(title: str, subtitle: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, 150):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, 150):
        draw.line((0, y, W, y), fill=GRID, width=1)
    draw.text((36, 28), title, fill=TEXT, font=TITLE)
    draw.text((36, 78), subtitle, fill=MUTED, font=BODY)
    return im, draw


def panel(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str, accent: str = GRAY) -> None:
    draw.rounded_rectangle(xy, radius=8, fill=PANEL, outline=accent, width=2)
    draw.text((xy[0] + 22, xy[1] + 18), title, fill=TEXT, font=H2)


def pill(draw: ImageDraw.ImageDraw, x: int, y: int, text: str, color: str) -> None:
    tw, th = text_size(draw, text, LABEL)
    draw.rounded_rectangle((x, y, x + tw + 28, y + th + 18), radius=8, fill="#30374a", outline=color, width=2)
    draw.text((x + 14, y + 8), text, fill=color, font=LABEL)


def arrow(draw: ImageDraw.ImageDraw, start: tuple[int, int], end: tuple[int, int], color: str, width: int = 5) -> None:
    draw.line((*start, *end), fill=color, width=width)
    ang = math.atan2(end[1] - start[1], end[0] - start[0])
    head = []
    for off in (2.55, -2.55):
        head.append((end[0] + math.cos(ang + off) * 18, end[1] + math.sin(ang + off) * 18))
    draw.polygon([end, *head], fill=color)


def draw_report_lines(
    draw: ImageDraw.ImageDraw,
    x: int,
    y: int,
    rows: list[tuple[str, str]],
    visible: int,
    color: str = TEXT,
    value_dx: int = 128,
) -> None:
    for i, (name, value) in enumerate(rows[:visible]):
        yy = y + i * 33
        draw.text((x, yy), name, fill=MUTED, font=SMALL)
        draw.text((x + value_dx, yy), value, fill=color, font=MONO)


def reveal_count(p: float, start: float, end: float, total: int) -> int:
    if p < start:
        return 0
    if p >= end:
        return total
    span = max(end - start, 0.01)
    return min(total, 1 + int(((p - start) / span) * total))


def draw_map(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], label: str) -> None:
    panel(draw, xy, label)
    x0, y0, x1, y1 = xy
    draw.line((x0 + 74, y1 - 74, x1 - 74, y1 - 74), fill="#5f6981", width=3)
    draw.line((x0 + 74, y1 - 74, x0 + 74, y0 + 94), fill="#5f6981", width=3)
    draw.text((x1 - 108, y1 - 58), "x", fill=MUTED, font=SMALL)
    draw.text((x0 + 52, y0 + 92), "y", fill=MUTED, font=SMALL)


def vehicle(draw: ImageDraw.ImageDraw, x: float, y: float, heading_deg: float, color: str = YELLOW) -> None:
    ang = math.radians(heading_deg)
    pts = [
        (x + math.sin(ang) * 26, y - math.cos(ang) * 26),
        (x + math.sin(ang + 2.45) * 14, y - math.cos(ang + 2.45) * 14),
        (x + math.sin(ang - 2.45) * 14, y - math.cos(ang - 2.45) * 14),
    ]
    draw.polygon(pts, fill=color, outline=WHITE)


def save(frames: list[Image.Image], filename: str, end_hold_ms: int = 0) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    durations = [FRAME_MS] * len(frames)
    if durations:
        durations[-1] += end_hold_ms
    frames[0].save(
        OUT_DIR / filename,
        save_all=True,
        append_images=frames[1:],
        duration=durations,
        loop=0,
        optimize=True,
    )


def render_pnodereporter_baseline() -> None:
    mail = [
        ("NAV_X,Y", "10,-5"),
        ("SPEED", "2.5"),
        ("HEADING", "123"),
        ("HELM", "DRIVE"),
    ]
    report = [
        ("NAME", "reporter"),
        ("X,Y", "10,-5"),
        ("SPD", "2.5"),
        ("HDG", "123"),
        ("FIRST_REPORT", "seen"),
    ]
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = base("pNodeReporter baseline", "scripted mail appears in NODE_REPORT_LOCAL")
        panel(draw, (76, 184, 438, 474), "input mail", BLUE)
        panel(draw, (762, 184, 1124, 474), "NODE_REPORT_LOCAL", GREEN)
        visible_mail = reveal_count(p, 0.08, 0.30, len(mail))
        visible_report = reveal_count(p, 0.60, 0.88, len(report))
        draw_report_lines(draw, 114, 266, mail, visible_mail, TEXT)
        draw_report_lines(draw, 800, 266, report, visible_report, GREEN, value_dx=156)
        move = ease((p - 0.38) / 0.24)
        arrow(draw, (466, 334), (734, 334), GREEN if p >= 0.38 else GRAY)
        dot_x = 466 + 268 * move
        draw.ellipse((dot_x - 10, 322, dot_x + 10, 342), fill=YELLOW, outline=WHITE, width=2)
        pill(draw, 519, 260, "pNodeReporter", GREEN)
        frames.append(im)
    save(frames, "pnodereporter-baseline-report.gif", end_hold_ms=1600)


def render_pnodereporter_alt_nav() -> None:
    alt_mail = [
        ("prefix", "NAV_GT"),
        ("GT_X,Y", "20,-15"),
        ("GT_SPEED", "3.1"),
        ("GT_HEADING", "210"),
    ]
    alt_report = [
        ("NAME", "reporter_GT"),
        ("GROUP", "truth"),
        ("X,Y", "20,-15"),
        ("SPD", "3.1"),
        ("HDG", "210"),
    ]
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = base("pNodeReporter alternate nav", "NAV_GT mail becomes the reporter_GT truth report")
        panel(draw, (76, 184, 438, 474), "NAV_GT input", ORANGE)
        panel(draw, (762, 184, 1124, 474), "reporter_GT report", GREEN)
        visible_mail = reveal_count(p, 0.08, 0.30, len(alt_mail))
        visible_report = reveal_count(p, 0.60, 0.88, len(alt_report))
        draw_report_lines(draw, 114, 266, alt_mail, visible_mail, TEXT)
        draw_report_lines(draw, 800, 266, alt_report, visible_report, GREEN)
        move = ease((p - 0.38) / 0.24)
        arrow(draw, (466, 334), (734, 334), GREEN if p >= 0.38 else GRAY)
        dot_x = 466 + 268 * move
        draw.ellipse((dot_x - 10, 324, dot_x + 10, 344), fill=YELLOW, outline=WHITE, width=2)
        pill(draw, 540, 260, "_GT path", GREEN)
        frames.append(im)
    save(frames, "pnodereporter-alt-nav-report.gif", end_hold_ms=1600)


def render_usim_forward() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = base("uSimMarineV22 forward thrust", "Thrust produces forward motion")
        draw_map(draw, (54, 128, 758, 586), "simulated track")
        x0, y = 150, 406
        x = x0 + 470 * ease(p)
        draw.line((x0, y, x, y), fill=GREEN, width=9)
        vehicle(draw, x, y, 90)
        draw.text((142, 445), "start", fill=MUTED, font=SMALL)
        draw.text((586, 445), "NAV_X rises", fill=GREEN, font=SMALL)
        panel(draw, (806, 150, 1148, 556), "pass evidence", GREEN)
        speed = 3.0 * ease(p)
        rows = [("THRUST", "60"), ("NAV_SPEED", f"{speed:0.1f}"), ("NAV_X", f"{10 + 75 * ease(p):0.0f}")]
        draw_report_lines(draw, 850, 250, rows, 3, GREEN)
        draw.rounded_rectangle((850, 434, 1110, 456), radius=6, fill="#3a4054")
        draw.rounded_rectangle((850, 434, 850 + int(260 * speed / 3.0), 456), radius=6, fill=TEAL)
        draw.text((850, 490), "thrust_forward_pass", fill=MUTED, font=SMALL)
        frames.append(im)
    save(frames, "usim-marine-forward-thrust.gif")


def render_usim_drift() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = base("uSimMarineV22 runtime drift", "Runtime drift moves a zero-thrust vehicle")
        draw_map(draw, (54, 128, 758, 586), "drift field")
        for x_start in (176, 376, 576):
            arrow(draw, (x_start, 250), (x_start + 82, 250), TEAL, 3)
        x0, y = 242, 376
        x = x0 + 330 * ease(p)
        draw.line((x0, y, x, y), fill=ORANGE, width=7)
        vehicle(draw, x, y, 90)
        pill(draw, 284, 184, "DRIFT_X=1.0", TEAL)
        panel(draw, (806, 150, 1148, 556), "pass evidence", ORANGE)
        rows = [
            ("THRUST", "0"),
            ("NAV_X", f"{5 + 44 * ease(p):0.0f}"),
            ("DRIFT", "x=1.0"),
        ]
        draw_report_lines(draw, 850, 250, rows, 3, ORANGE)
        draw.text((850, 490), "drift_x_pass", fill=MUTED, font=SMALL)
        frames.append(im)
    save(frames, "usim-marine-runtime-drift.gif")


def main() -> None:
    render_pnodereporter_baseline()
    render_pnodereporter_alt_nav()
    render_usim_forward()
    render_usim_drift()


if __name__ == "__main__":
    main()
