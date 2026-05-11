#!/usr/bin/env python3
"""Render generated GIFs for the BHV_Timer harness page.

These are deterministic documentation views of the Timer telemetry story. The
headless harness runs remain the source of truth; these GIFs make the variable
state transitions legible in a way a raw pMarineViewer capture does not.
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 1600, 900
FRAMES = 72
FRAME_MS = 110

BG = "#202433"
PANEL = "#252a3a"
PANEL_2 = "#2a3042"
GRID = "#343a4a"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
YELLOW = "#f3ea5f"
GREEN = "#75d38b"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
RED = "#f06c64"
BLUE = "#70b9ff"
PURPLE = "#9b8cff"
WHITE = "#f5f7fb"


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


FONT_TITLE = font(42, True)
FONT_H2 = font(31, True)
FONT_LABEL = font(24, True)
FONT_TEXT = font(23)
FONT_SMALL = font(20)
FONT_TINY = font(17)
FONT_MICRO = font(15)


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, v))


def ease(v: float) -> float:
    v = clamp(v)
    return v * v * (3 - 2 * v)


def draw_base(title: str, subtitle: str, case_name: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, 80):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, 80):
        draw.line((0, y, W, y), fill=GRID, width=1)
    draw.text((52, 38), title, fill=TEXT, font=FONT_TITLE)
    draw.text((52, 92), subtitle, fill=MUTED, font=FONT_TEXT)
    draw.text((W - 430, H - 34), f"H01 Timer Behavior | {case_name}", fill=MUTED, font=FONT_TINY)
    return im, draw


def center_text(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int, int, int],
    text: str,
    fill: str,
    text_font: ImageFont.FreeTypeFont | ImageFont.ImageFont,
) -> None:
    box = draw.textbbox((0, 0), text, font=text_font)
    tw = box[2] - box[0]
    th = box[3] - box[1]
    x = xy[0] + (xy[2] - xy[0] - tw) / 2
    y = xy[1] + (xy[3] - xy[1] - th) / 2 - 1
    draw.text((x, y), text, fill=fill, font=text_font)


def draw_status_card(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int, int, int],
    heading: str,
    body: str,
    color: str,
) -> None:
    draw.rounded_rectangle(xy, radius=10, fill=PANEL, outline=color, width=2)
    draw.text((xy[0] + 20, xy[1] + 17), heading, fill=color, font=FONT_LABEL)
    draw.text((xy[0] + 20, xy[1] + 56), body, fill=TEXT, font=FONT_SMALL)


def draw_metric_pair(
    draw: ImageDraw.ImageDraw,
    x: int,
    y: int,
    label: str,
    value: float,
    family: str,
    color: str,
    enabled: bool = True,
) -> None:
    outline = color if enabled else "#51586a"
    fill = PANEL if enabled else "#242837"
    draw.rounded_rectangle((x, y, x + 270, y + 116), radius=10, fill=fill, outline=outline, width=2)
    draw.text((x + 20, y + 17), label, fill=outline, font=FONT_LABEL)
    draw.text((x + 20, y + 55), f"{value:0.1f}s", fill=TEXT if enabled else MUTED, font=FONT_H2)
    draw.text((x + 20, y + 91), family, fill=MUTED, font=FONT_TINY)


def draw_timeline(
    draw: ImageDraw.ImageDraw,
    x: int,
    y: int,
    width: int,
    t: float,
    t_max: float,
    markers: tuple[tuple[float, str, str], ...],
) -> None:
    draw.text((x, y - 50), "key events", fill=TEXT, font=FONT_LABEL)
    draw.line((x, y, x + width, y), fill="#586175", width=3)
    tx = x + int(width * clamp(t / t_max))
    draw.ellipse((tx - 10, y - 10, tx + 10, y + 10), fill=YELLOW, outline=WHITE, width=2)
    for mt, label, color in markers:
        mx = x + int(width * mt / t_max)
        draw.line((mx, y - 22, mx, y + 22), fill=color, width=3)
        box = draw.textbbox((0, 0), label, font=FONT_TINY)
        label_x = clamp(mx - (box[2] - box[0]) / 2, x, x + width - (box[2] - box[0]))
        draw.text((label_x, y + 34), label, fill=color, font=FONT_TINY)


def boat_position(t: float, t_max: float) -> tuple[float, float]:
    f = clamp(t / t_max)
    x0, y0 = 160, 642
    x1, y1 = 820, 330
    wiggle = math.sin(f * math.pi * 2) * 18
    return x0 + (x1 - x0) * f, y0 + (y1 - y0) * f + wiggle


def draw_map(draw: ImageDraw.ImageDraw, t: float, t_max: float) -> None:
    rect = (54, 178, 875, 742)
    draw.rounded_rectangle(rect, radius=12, fill="#22283a", outline="#4a5062", width=2)
    for x in range(rect[0] + 56, rect[2], 86):
        draw.line((x, rect[1], x, rect[3]), fill="#30364a", width=1)
    for y in range(rect[1] + 56, rect[3], 86):
        draw.line((rect[0], y, rect[2], y), fill="#30364a", width=1)
    path = []
    for i in range(50):
        px, py = boat_position(t_max * i / 49, t_max)
        path.append((px, py))
    draw.line(path, fill="#6b7287", width=2)
    bx, by = boat_position(t, t_max)
    heading = -25
    angle = math.radians(90 - heading)
    pts = [
        (bx + math.cos(angle) * 26, by - math.sin(angle) * 26),
        (bx + math.cos(angle + 2.4) * 13, by - math.sin(angle + 2.4) * 13),
        (bx + math.cos(angle - 2.4) * 13, by - math.sin(angle - 2.4) * 13),
    ]
    draw.polygon(pts, fill=YELLOW, outline="#fff7a6")
    draw.text((bx + 20, by - 14), "abe", fill=WHITE, font=FONT_SMALL)
    draw.text((rect[0] + 20, rect[1] + 18), "vehicle motion stays ordinary", fill=MUTED, font=FONT_SMALL)


def draw_state_row(
    draw: ImageDraw.ImageDraw,
    x: int,
    y: int,
    states: tuple[tuple[str, str, bool, str], ...],
) -> None:
    w, h, gap = 128, 88, 16
    for idx, (label, value, selected, color) in enumerate(states):
        x0 = x + idx * (w + gap)
        fill = PANEL if selected else "#242837"
        outline = color if selected else "#50586b"
        draw.rounded_rectangle((x0, y, x0 + w, y + h), radius=10, fill=fill, outline=outline, width=2)
        center_text(draw, (x0 + 10, y + 13, x0 + w - 10, y + 41), label, outline, FONT_TINY)
        center_text(draw, (x0 + 10, y + 43, x0 + w - 10, y + 76), value, TEXT if selected else MUTED, FONT_LABEL)


def pause_values(t: float) -> tuple[bool, float, float]:
    # Mirrors pause_resume_pass: true at 2s, false at 5s, true at 8s.
    idle = min(t, 2.0) + max(0.0, min(t, 8.0) - 5.0)
    running = max(0.0, min(t, 5.0) - 2.0) + max(0.0, t - 8.0)
    active = (2.0 <= t < 5.0) or (t >= 8.0)
    return active, idle, running


def render_pause_resume() -> None:
    frames: list[Image.Image] = []
    t_max = 13.0
    for i in range(FRAMES):
        t = t_max * i / (FRAMES - 1)
        active, idle, running = pause_values(t)
        im, draw = draw_base(
            "BHV_Timer pause/resume",
            "TIMER_ACTIVE toggles while idle and running counters accumulate separately",
            "pause_resume_pass",
        )
        draw_map(draw, t, t_max)
        draw_timeline(
            draw,
            940,
            228,
            520,
            t,
            t_max,
            ((2, "active true", GREEN), (5, "active false", ORANGE), (8, "active true", GREEN), (13, "eval", TEAL)),
        )
        draw_status_card(
            draw,
            (940, 298, 1500, 394),
            "TIMER_ACTIVE",
            "running counter is selected" if active else "idle counter is selected",
            GREEN if active else ORANGE,
        )
        draw_metric_pair(draw, 940, 432, "Idle total", idle, "idle counter", ORANGE)
        draw_metric_pair(draw, 1230, 432, "Running total", running, "running counter", GREEN)
        draw_state_row(
            draw,
            940,
            608,
            (
                ("0-2s", "idle", t < 2, ORANGE),
                ("2-5s", "running", 2 <= t < 5, GREEN),
                ("5-8s", "idle", 5 <= t < 8, ORANGE),
                ("8-13s", "running", t >= 8, GREEN),
            ),
        )
        draw.rounded_rectangle((940, 740, 1500, 810), radius=10, fill=PANEL_2, outline=TEAL, width=2)
        draw.text((962, 762), "Unique check: both totals span pause/resume intervals.", fill=TEXT, font=FONT_SMALL)
        frames.append(im)
    save_gif(frames, "timer-behavior-pause-resume.gif")


def runtime_values(t: float) -> tuple[bool, bool, float, float, float, float]:
    # Mirrors runtime_update_pass: TIMER_ACTIVE at 4s, update at 6s, eval at 12s.
    active = t >= 4.0
    updated = t >= 6.0
    idle_total = min(t, 4.0)
    running_total = max(0.0, t - 4.0)
    base_running = min(running_total, 2.0)
    updated_running = running_total if updated else 0.0
    updated_idle = idle_total if updated else 0.0
    return active, updated, idle_total, base_running, updated_idle, updated_running


def render_runtime_update() -> None:
    frames: list[Image.Image] = []
    t_max = 12.0
    for i in range(FRAMES):
        t = t_max * i / (FRAMES - 1)
        active, updated, base_idle, base_running, updated_idle, updated_running = runtime_values(t)
        im, draw = draw_base(
            "BHV_Timer runtime update",
            "TIMER_UPDATES changes status variable names while the internal counters continue",
            "runtime_update_pass",
        )
        draw_map(draw, t, t_max)
        draw_timeline(
            draw,
            940,
            228,
            520,
            t,
            t_max,
            ((4, "active", GREEN), (6, "update", PURPLE), (12, "eval", TEAL)),
        )
        draw_status_card(
            draw,
            (940, 298, 1500, 394),
            "TIMER_UPDATES",
            "new status variable names are in effect" if updated else "base status variable names are in effect",
            PURPLE if updated else "#566075",
        )
        draw_metric_pair(draw, 940, 432, "Base idle", base_idle, "base family", ORANGE)
        draw_metric_pair(draw, 1230, 432, "Base run", base_running, "base family", GREEN)
        draw_metric_pair(
            draw,
            940,
            598,
            "Updated idle",
            updated_idle,
            "updated family",
            BLUE,
            enabled=updated,
        )
        draw_metric_pair(
            draw,
            1230,
            598,
            "Updated run",
            updated_running,
            "updated family",
            BLUE,
            enabled=updated,
        )
        draw.rounded_rectangle((940, 768, 1500, 830), radius=10, fill=PANEL_2, outline=TEAL if updated else "#566075", width=2)
        status = "Unique check: old and new variable families both exist." if updated else "Before update: only the base variable family is expected."
        draw.text((962, 788), status, fill=TEXT if updated else MUTED, font=FONT_SMALL)
        frames.append(im)
    save_gif(frames, "timer-behavior-runtime-update.gif")


def save_gif(frames: list[Image.Image], filename: str) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        OUT_DIR / filename,
        save_all=True,
        append_images=frames[1:],
        optimize=True,
        duration=FRAME_MS,
        loop=0,
    )


def main() -> None:
    render_pause_resume()
    render_runtime_update()
    print(f"wrote {OUT_DIR / 'timer-behavior-pause-resume.gif'}")
    print(f"wrote {OUT_DIR / 'timer-behavior-runtime-update.gif'}")


if __name__ == "__main__":
    main()
