#!/usr/bin/env python3
"""Render map-style explanatory GIFs for app-level unit harness pages.

The OBMGR unit visuals are drawn from the same MOOS coordinates and result
values seen in temp-run alogs for the representative cases:

- `given_baseline_pass`: ownship `(0,-60)`, GIVEN_OBSTACLE
  `pts={18,-62:22,-62:22,-58:18,-58}`, `OBM_MIN_DIST_EVER=18.00000`.
- `points_cluster_dist_pass`: ownship `(0,-60)`, TRACKED_FEATURE points
  `(18,-62)`, `(22,-62)`, `(22,-58)`, `(18,-58)`, final alert polygon
  `pts={18,-58:22,-58:22,-62:18,-62}`, `OBM_MIN_DIST_EVER=18.00000`.
- `runtime_alert_add_pass`: runtime `BCM_ALERT_REQUEST`, spoofed intruder
  starts at `(20,-44)`, and the result line reports `range=14`.
- `detect_cpa_only_pass`: spoofed intruder starts at `(35,-35)`, detection is
  caused by `cpa_range=40` while the result line reports `range=30`. The alog
  range minimum occurs as the ownship approaches `x=34` on `y=-60`, so the CPA
  marker is drawn near `(34,-61)`.

These remain generated documentation assets, not substitute test evidence.
Headless harness/alogs remain the source of truth.
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"
W, H = 1600, 900
SCALE = W / 1280
FRAMES = 64
FRAME_MS = 125

BG = "#242836"
GRID = "#2d3242"
GRID_STRONG = "#343a4c"
TEXT = "#e9edf5"
MUTED = "#98a2b8"
YELLOW = "#f3ea5f"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
BLUE = "#70b9ff"
WHITE_FAINT = "#d9e2f055"

OWN = (0.0, -60.0)
GIVEN_POLY = [(18.0, -62.0), (22.0, -62.0), (22.0, -58.0), (18.0, -58.0)]
POINT_EVENTS = [(18.0, -62.0), (22.0, -62.0), (22.0, -58.0), (18.0, -58.0)]
POINT_HULL = [(18.0, -58.0), (22.0, -58.0), (22.0, -62.0), (18.0, -62.0)]
CPA_ONLY_MARKER = (34.0, -61.0)

WORLD_X_MIN, WORLD_X_MAX = -5.0, 28.0
WORLD_Y_MIN, WORLD_Y_MAX = -70.0, -50.0
MAP_LEFT, MAP_TOP = int(90 * SCALE), int(86 * SCALE)
MAP_RIGHT, MAP_BOTTOM = W - int(76 * SCALE), H - int(110 * SCALE)
CMGR_BOUNDS = (-5.0, 66.0, -104.0, -32.0)


def px(value: float) -> int:
    return int(round(value * SCALE))


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


FONT_TINY = font(px(15))
FONT_SMALL = font(px(19))
FONT_SMALL_BOLD = font(px(19), True)
FONT_MED = font(px(26), True)

PID_FONT_TITLE = font(px(34), True)
PID_FONT_SUB = font(px(24))
PID_FONT_LABEL = font(px(30), True)
PID_FONT_DETAIL = font(px(22))
PID_FONT_TAG = font(px(24), True)
PID_FONT_CARD = font(px(32), True)


def clamp01(x: float) -> float:
    return max(0.0, min(1.0, x))


def ease(x: float) -> float:
    x = clamp01(x)
    return x * x * (3 - 2 * x)


def world_to_screen(
    p: tuple[float, float],
    bounds: tuple[float, float, float, float] = (WORLD_X_MIN, WORLD_X_MAX, WORLD_Y_MIN, WORLD_Y_MAX),
) -> tuple[float, float]:
    x_min, x_max, y_min, y_max = bounds
    x, y = p
    sx = MAP_LEFT + (x - x_min) / (x_max - x_min) * (MAP_RIGHT - MAP_LEFT)
    sy = MAP_BOTTOM - (y - y_min) / (y_max - y_min) * (MAP_BOTTOM - MAP_TOP)
    return sx, sy


def dashed_line(
    draw: ImageDraw.ImageDraw,
    a: tuple[float, float],
    b: tuple[float, float],
    fill: str,
    width: int = px(2),
    dash: int = px(12),
    gap: int = px(10),
) -> None:
    ax, ay = a
    bx, by = b
    dist = math.hypot(bx - ax, by - ay)
    if dist <= 0:
        return
    ux, uy = (bx - ax) / dist, (by - ay) / dist
    pos = 0.0
    while pos < dist:
        end = min(pos + dash, dist)
        draw.line((ax + ux * pos, ay + uy * pos, ax + ux * end, ay + uy * end), fill=fill, width=width)
        pos += dash + gap


def draw_base(title: str, subtitle: str, case_name: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, px(40)):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, px(40)):
        draw.line((0, y, W, y), fill=GRID, width=1)
    for x in range(0, W, px(200)):
        draw.line((x, 0, x, H), fill=GRID_STRONG, width=1)
    for y in range(0, H, px(200)):
        draw.line((0, y, W, y), fill=GRID_STRONG, width=1)
    draw.text((px(42), px(34)), title, fill=TEXT, font=FONT_SMALL_BOLD)
    draw.text((px(42), px(62)), subtitle, fill=MUTED, font=FONT_TINY)
    draw.text((W - px(392), H - px(54)), f"H01 {case_name}", fill=MUTED, font=FONT_TINY)
    return im, draw


def draw_pid_base(title: str, subtitle: str, case_name: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, px(64)):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, px(64)):
        draw.line((0, y, W, y), fill=GRID, width=1)
    for x in range(0, W, px(256)):
        draw.line((x, 0, x, H), fill=GRID_STRONG, width=1)
    for y in range(0, H, px(256)):
        draw.line((0, y, W, y), fill=GRID_STRONG, width=1)
    draw.text((px(48), px(42)), title, fill=TEXT, font=PID_FONT_TITLE)
    draw.text((px(48), px(88)), subtitle, fill=MUTED, font=PID_FONT_SUB)
    draw.text((W - px(455), H - px(72)), f"H01 {case_name}", fill=MUTED, font=PID_FONT_DETAIL)
    return im, draw


def pid_tag(draw: ImageDraw.ImageDraw, p: tuple[float, float], text: str, color: str) -> None:
    x, y = p
    bbox = draw.textbbox((0, 0), text, font=PID_FONT_TAG)
    w = bbox[2] - bbox[0] + px(24)
    h = bbox[3] - bbox[1] + px(18)
    draw.rounded_rectangle((x, y, x + w, y + h), radius=px(7), fill="#252b3a", outline=color, width=px(2))
    draw.text((x + px(12), y + px(8)), text, fill=color, font=PID_FONT_TAG)


def pid_pill(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str, detail: str, ok: bool) -> None:
    outline = TEAL if ok else ORANGE
    fill = "#273142" if ok else "#262b39"
    draw.rounded_rectangle(xy, radius=px(10), fill=fill, outline=outline, width=px(3))
    draw.text((xy[0] + px(22), xy[1] + px(18)), title, fill=outline, font=PID_FONT_CARD)
    draw.text((xy[0] + px(22), xy[1] + px(60)), detail, fill=TEXT if ok else MUTED, font=PID_FONT_DETAIL)


def ownship(draw: ImageDraw.ImageDraw, p_world: tuple[float, float], label: str = "abe") -> None:
    x, y = world_to_screen(p_world)
    tri = [(x, y - px(21)), (x - px(15), y + px(16)), (x + px(15), y + px(16))]
    draw.polygon(tri, fill=YELLOW, outline="#fff7a8")
    draw.line((x, y + px(16), x, y + px(54)), fill=WHITE_FAINT, width=1)
    draw.text((x + px(20), y - px(8)), f"{label}  ({p_world[0]:.0f},{p_world[1]:.0f})", fill=TEXT, font=FONT_TINY)


def pill(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str, detail: str, ok: bool) -> None:
    outline = TEAL if ok else "#444b5f"
    fill = "#273142" if ok else "#262b39"
    draw.rounded_rectangle(xy, radius=px(8), fill=fill, outline=outline, width=px(2) if ok else 1)
    draw.text((xy[0] + px(18), xy[1] + px(15)), title, fill=TEAL if ok else TEXT, font=FONT_SMALL_BOLD)
    draw.text((xy[0] + px(18), xy[1] + px(46)), detail, fill=MUTED, font=FONT_TINY)


def small_tag(draw: ImageDraw.ImageDraw, p: tuple[float, float], text: str, color: str) -> None:
    x, y = p
    bbox = draw.textbbox((0, 0), text, font=FONT_TINY)
    w = bbox[2] - bbox[0] + px(18)
    h = bbox[3] - bbox[1] + px(12)
    draw.rounded_rectangle((x, y, x + w, y + h), radius=px(5), fill="#252b3a", outline=color, width=1)
    draw.text((x + px(9), y + px(6)), text, fill=color, font=FONT_TINY)


def draw_poly(
    draw: ImageDraw.ImageDraw,
    pts_world: list[tuple[float, float]],
    outline: str,
    fill: str,
    width: int = px(3),
) -> list[tuple[float, float]]:
    pts = [world_to_screen(p) for p in pts_world]
    draw.polygon(pts, fill=fill, outline=outline)
    draw.line(pts + [pts[0]], fill=outline, width=width)
    return pts


def draw_feature_point(draw: ImageDraw.ImageDraw, p_world: tuple[float, float], color: str = BLUE) -> None:
    x, y = world_to_screen(p_world)
    draw.ellipse((x - px(7), y - px(7), x + px(7), y + px(7)), fill=color, outline="#fff0c9" if color == ORANGE else "#d4ecff")


def draw_contact(
    draw: ImageDraw.ImageDraw,
    p_world: tuple[float, float],
    label: str,
    color: str,
    bounds: tuple[float, float, float, float],
) -> tuple[float, float]:
    x, y = world_to_screen(p_world, bounds)
    tri = [(x, y - px(11)), (x - px(15), y + px(12)), (x + px(15), y + px(12))]
    draw.polygon(tri, fill=color, outline="#d8ffe8" if color == TEAL else "#fff0c9")
    draw.text((x + px(15), y - px(6)), label, fill=TEXT, font=FONT_TINY)
    return x, y


def ownship_cmgr(draw: ImageDraw.ImageDraw, p_world: tuple[float, float]) -> tuple[float, float]:
    x, y = world_to_screen(p_world, CMGR_BOUNDS)
    tri = [(x, y - px(20)), (x - px(14), y + px(16)), (x + px(14), y + px(16))]
    draw.polygon(tri, fill=YELLOW, outline="#fff7a8")
    draw.text((x + px(18), y - px(8)), f"abe  ({p_world[0]:.0f},-60)", fill=TEXT, font=FONT_TINY)
    return x, y


def cmgr_path(draw: ImageDraw.ImageDraw, start: tuple[float, float], end: tuple[float, float]) -> None:
    dashed_line(draw, world_to_screen(start, CMGR_BOUNDS), world_to_screen(end, CMGR_BOUNDS), fill=WHITE_FAINT)
    wx, wy = world_to_screen(end, CMGR_BOUNDS)
    draw.ellipse((wx - px(5), wy - px(5), wx + px(5), wy + px(5)), fill=BLUE)
    draw.text((wx + px(12), wy - px(8)), "waypoint", fill=MUTED, font=FONT_TINY)


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def distance_annotation(draw: ImageDraw.ImageDraw, label: str = "18.0m") -> None:
    own = world_to_screen(OWN)
    nearest = world_to_screen((18.0, -60.0))
    dashed_line(draw, (own[0] + px(18), own[1]), nearest, fill=WHITE_FAINT)
    mx = (own[0] + nearest[0]) / 2
    my = (own[1] + nearest[1]) / 2
    draw.text((mx - px(24), my - px(28)), label, fill=MUTED, font=FONT_TINY)


def save_gif(frames: list[Image.Image], name: str) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        OUT_DIR / name,
        save_all=True,
        append_images=frames[1:],
        duration=FRAME_MS,
        loop=0,
        disposal=2,
        optimize=True,
    )


def obmgr_given_baseline() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        t = i / (FRAMES - 1)
        obstacle_alpha = ease((t - 0.18) / 0.26)
        accepted = t >= 0.56
        im, draw = draw_base(
            "Given Obstacle Alert",
            "alog: GIVEN_OBSTACLE pts={18,-62:22,-62:22,-58:18,-58}",
            "given_baseline_pass",
        )
        ownship(draw, OWN)
        distance_annotation(draw)

        if obstacle_alpha > 0:
            offset = 1.0 - obstacle_alpha
            shifted = [(x, y + 2.0 * offset) for x, y in GIVEN_POLY]
            pts = draw_poly(draw, shifted, ORANGE if not accepted else TEAL, "#38c6b533" if accepted else "#d9904f33")
            small_tag(draw, (pts[1][0] + px(16), pts[1][1] - px(8)), "GIVEN_OBSTACLE", TEAL if accepted else ORANGE)

        if accepted:
            pts = draw_poly(draw, GIVEN_POLY, TEAL, "#38c6b540", width=px(4))
            small_tag(draw, (pts[2][0] + px(16), pts[2][1] - px(4)), "OBM_NEW_GIVEN=true", TEAL)

        if t >= 0.68:
            pill(draw, (px(42), H - px(132), px(520), H - px(44)), "OBM_DIST_TO_OBJ = OB_NEAR,18.0", "OBM_MIN_DIST_EVER=18.00000 / grade=pass", True)
        else:
            pill(draw, (px(42), H - px(132), px(420), H - px(44)), "waiting for OBM_NEW_GIVEN", "timer mail has not been accepted yet", False)
        frames.append(im)
    save_gif(frames, "obmgr-unit-given-baseline.gif")


def obmgr_point_cluster() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        t = i / (FRAMES - 1)
        im, draw = draw_base(
            "Point Cluster Hull",
            "alog: four TRACKED_FEATURE points form alert polygon rock_a",
            "points_cluster_dist_pass",
        )
        ownship(draw, OWN)
        distance_annotation(draw)

        visible_count = min(len(POINT_EVENTS), int(ease((t - 0.10) / 0.42) * (len(POINT_EVENTS) + 0.99)))
        point_color = TEAL if t >= 0.52 else ORANGE
        for point in POINT_EVENTS[:visible_count]:
            draw_feature_point(draw, point, point_color)

        if t >= 0.52:
            pts = draw_poly(draw, POINT_HULL, TEAL, "#38c6b538", width=px(4))
            for point in POINT_HULL:
                draw_feature_point(draw, point, TEAL)
            small_tag(draw, (pts[1][0] + px(16), pts[1][1] - px(8)), "VIEW_POLYGON rock_a", TEAL)

        if visible_count > 0:
            label_anchor = world_to_screen((18.0, -62.0))
            small_tag(draw, (label_anchor[0] - px(150), label_anchor[1] + px(18)), "TRACKED_FEATURE", point_color)

        if t >= 0.70:
            pill(draw, (px(42), H - px(132), px(532), H - px(44)), "OBM_DIST_TO_OBJ = ROCK_A,18.0", "final hull pts={18,-58:22,-58:22,-62:18,-62}", True)
        else:
            pill(draw, (px(42), H - px(132), px(458), H - px(44)), "TRACKED_FEATURE points", "building obstacle geometry from alog points", False)
        frames.append(im)
    save_gif(frames, "obmgr-unit-point-cluster.gif")


def cmgr_runtime_alert() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        t = i / (FRAMES - 1)
        alert_sent = t >= 0.34
        spoof_visible = t >= 0.48
        seen = t >= 0.68
        own_x = lerp(0.0, 20.0, ease((t - 0.16) / 0.64))
        contact_y = lerp(-44.0, -45.5, ease((t - 0.48) / 0.42))

        im, draw = draw_base(
            "Runtime Alert Path",
            "alog: BCM_ALERT_REQUEST then SPOOF intruder at x=20,y=-44",
            "runtime_alert_add_pass",
        )
        cmgr_path(draw, (0.0, -60.0), (60.0, -60.0))
        ownship_cmgr(draw, (own_x, -60.0))

        if alert_sent:
            small_tag(draw, world_to_screen((3.0, -52.0), CMGR_BOUNDS), "BCM_ALERT_REQUEST", ORANGE if not seen else TEAL)

        if spoof_visible:
            color = TEAL if seen else ORANGE
            contact = (20.0, contact_y)
            draw_contact(draw, contact, "intruder", color, CMGR_BOUNDS)
            if seen:
                ox, oy = world_to_screen((own_x, -60.0), CMGR_BOUNDS)
                cx, cy = world_to_screen(contact, CMGR_BOUNDS)
                dashed_line(draw, (ox + px(16), oy), (cx, cy), fill=WHITE_FAINT)
                small_tag(draw, (cx + px(20), cy + px(18)), "CONTACT_SEEN", TEAL)

        if seen:
            pill(draw, (px(42), H - px(132), px(500), H - px(44)), "CONTACT_SEEN = true", "closest=intruder / result range=14", True)
        else:
            pill(draw, (px(42), H - px(132), px(410), H - px(44)), "CONTACT_SEEN = false", "waiting for runtime alert and spoof", False)
        frames.append(im)
    save_gif(frames, "cmgr-unit-runtime-alert.gif")


def cmgr_cpa_detection() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        t = i / (FRAMES - 1)
        spoof_visible = t >= 0.28
        seen = t >= 0.62
        own_x = lerp(0.0, 13.0, ease((t - 0.12) / 0.62))
        contact_y = lerp(-35.0, -41.0, ease((t - 0.28) / 0.52))

        im, draw = draw_base(
            "CPA-only Detection",
            "alog: cpa_range=40 detects intruder while result range remains 30",
            "detect_cpa_only_pass",
        )
        cmgr_path(draw, (0.0, -60.0), (60.0, -60.0))
        ownship_cmgr(draw, (own_x, -60.0))

        if spoof_visible:
            color = TEAL if seen else ORANGE
            contact = (35.0, contact_y)
            cx, cy = draw_contact(draw, contact, "intruder", color, CMGR_BOUNDS)
            cpa_x, cpa_y = world_to_screen(CPA_ONLY_MARKER, CMGR_BOUNDS)
            dashed_line(draw, (cx, cy + px(12)), (cpa_x, cpa_y), fill="#3ddc8455")
            draw.ellipse((cpa_x - px(5), cpa_y - px(5), cpa_x + px(5), cpa_y + px(5)), fill=ORANGE if not seen else TEAL)
            small_tag(draw, (cpa_x - px(120), cpa_y + px(16)), "predicted CPA", ORANGE if not seen else TEAL)
            if seen:
                small_tag(draw, (cx + px(20), cy + px(18)), "CONTACT_SEEN", TEAL)

        if seen:
            pill(draw, (px(42), H - px(132), px(500), H - px(44)), "CONTACT_SEEN = true", "result range=30 / reason=CPA closing course", True)
        else:
            pill(draw, (px(42), H - px(132), px(440), H - px(44)), "current range still broad", "checking predicted closest approach", False)
        frames.append(im)
    save_gif(frames, "cmgr-unit-cpa-detection.gif")


def pid_heading_wrap() -> None:
    frames: list[Image.Image] = []
    cx, cy = W // 2 + px(116), H // 2 - px(38)
    radius = px(235)
    for i in range(FRAMES):
        t = i / (FRAMES - 1)
        resolved = t >= 0.58
        arc_t = ease((t - 0.20) / 0.46)

        im, draw = draw_pid_base(
            "Heading Wrap",
            "Checks the short turn across 0 deg.",
            "heading_wrap_pass",
        )
        draw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), outline=WHITE_FAINT, width=px(3))
        for label, angle in (("N", 0), ("E", 90), ("S", 180), ("W", 270)):
            rad = math.radians(angle - 90)
            x = cx + math.cos(rad) * (radius + px(46))
            y = cy + math.sin(rad) * (radius + px(46))
            draw.text((x - px(12), y - px(18)), label, fill=MUTED, font=PID_FONT_LABEL)

        for angle, color, text, offset in (
            (355, ORANGE, "NAV 355", (-px(210), px(10))),
            (5, TEAL, "DESIRED 5", (px(58), px(10))),
        ):
            rad = math.radians(angle - 90)
            x = cx + math.cos(rad) * radius
            y = cy + math.sin(rad) * radius
            draw.line((cx, cy, x, y), fill=color, width=px(5))
            pid_tag(draw, (x + offset[0], y + offset[1]), text, color)

        start = math.radians(355 - 90)
        end = math.radians((355 + 10 * arc_t) - 90)
        points = []
        for step in range(22):
            a = start + (end - start) * step / 21
            points.append((cx + math.cos(a) * px(178), cy + math.sin(a) * px(178)))
        draw.line(points, fill=TEAL if resolved else ORANGE, width=px(6))
        pid_tag(draw, (px(72), px(158)), "short wrap: +10 deg", TEAL if resolved else ORANGE)
        pid_pill(draw, (px(48), H - px(160), px(645), H - px(48)), "RUDDER_PID = +10", "Positive small-turn command, not a 350 deg swing.", resolved)
        frames.append(im)
    save_gif(frames, "pid-unit-heading-wrap.gif")


def pid_depth_elevator() -> None:
    frames: list[Image.Image] = []
    current_depth = 5.0
    target_depth = 6.0
    expected_elevator = 11.5
    for i in range(FRAMES):
        t = i / (FRAMES - 1)
        active = t >= 0.38
        elevator = expected_elevator * ease((t - 0.38) / 0.42)
        error_alpha = ease((t - 0.12) / 0.30)
        gauge_alpha = ease((t - 0.42) / 0.38)

        im, draw = draw_pid_base(
            "Depth Elevator",
            "Checks depth error maps to elevator authority.",
            "depth_elevator_pass",
        )
        left, top = px(180), px(170)
        right, bottom = W - px(500), H - px(230)
        draw.rectangle((left, top, right, bottom), outline=WHITE_FAINT, width=px(2))
        depth_min, depth_max = 4.0, 7.0
        for depth in range(4, 8):
            y = top + ((depth - depth_min) / (depth_max - depth_min)) * (bottom - top)
            draw.line((left, y, right, y), fill=GRID_STRONG, width=1)
            draw.text((left - px(96), y - px(16)), f"{depth}m", fill=MUTED, font=PID_FONT_DETAIL)

        nav_y = top + ((current_depth - depth_min) / (depth_max - depth_min)) * (bottom - top)
        des_y = top + ((target_depth - depth_min) / (depth_max - depth_min)) * (bottom - top)
        draw.line((left, des_y, right, des_y), fill=TEAL, width=px(5))
        pid_tag(draw, (right - px(290), des_y - px(70)), "TARGET 6m", TEAL)

        sub_x = left + (right - left) * 0.47
        marker_color = ORANGE if not active else YELLOW
        draw.rounded_rectangle(
            (sub_x - px(50), nav_y - px(18), sub_x + px(50), nav_y + px(18)),
            radius=px(6),
            fill=marker_color,
            outline="#fff0c9",
        )
        draw.polygon(
            [(sub_x + px(50), nav_y), (sub_x + px(82), nav_y - px(20)), (sub_x + px(82), nav_y + px(20))],
            fill=marker_color,
            outline="#fff0c9",
        )
        arrow_y = nav_y + (des_y - nav_y) * error_alpha
        dashed_line(draw, (sub_x, nav_y + px(32)), (sub_x, arrow_y), fill=WHITE_FAINT, width=px(3))
        if error_alpha > 0.7:
            draw.polygon(
                [
                    (sub_x, des_y + px(8)),
                    (sub_x - px(12), des_y - px(15)),
                    (sub_x + px(12), des_y - px(15)),
                ],
                fill=WHITE_FAINT,
            )
            pid_tag(draw, (sub_x - px(118), (nav_y + des_y) / 2 - px(18)), "error +1m", WHITE_FAINT)
        pid_tag(draw, (sub_x + px(104), nav_y - px(24)), "CURRENT 5m", ORANGE)

        gauge_x0, gauge_y0 = right + px(80), top + px(20)
        gauge_x1, gauge_y1 = W - px(120), bottom - px(10)
        draw.rounded_rectangle((gauge_x0, gauge_y0, gauge_x1, gauge_y1), radius=px(10), fill="#252b3a", outline="#444b60", width=px(2))
        draw.text((gauge_x0, gauge_y0 - px(48)), "ELEVATOR OUTPUT", fill=MUTED, font=PID_FONT_DETAIL)
        band_y0 = gauge_y0 + (1 - (11.7 / 13.0)) * (gauge_y1 - gauge_y0)
        band_y1 = gauge_y0 + (1 - (11.3 / 13.0)) * (gauge_y1 - gauge_y0)
        draw.rectangle((gauge_x0 + px(18), band_y0, gauge_x1 - px(18), band_y1), fill="#38c6b544")
        for value in (0, 6.5, 13):
            y = gauge_y0 + (1 - (value / 13.0)) * (gauge_y1 - gauge_y0)
            draw.line((gauge_x0, y, gauge_x1, y), fill=GRID_STRONG, width=1)
            draw.text((gauge_x1 + px(12), y - px(12)), f"{value:g}", fill=MUTED, font=PID_FONT_DETAIL)
        fill_y = gauge_y1 - (gauge_y1 - gauge_y0) * (elevator / 13.0) * gauge_alpha
        draw.rounded_rectangle((gauge_x0 + px(42), fill_y, gauge_x1 - px(42), gauge_y1), radius=px(8), fill=TEAL if active else "#38c6b555")
        draw.line((gauge_x0 + px(18), band_y0, gauge_x1 - px(18), band_y0), fill=TEAL, width=px(3))
        draw.line((gauge_x0 + px(18), band_y1, gauge_x1 - px(18), band_y1), fill=TEAL, width=px(3))

        pid_pill(
            draw,
            (px(48), H - px(160), px(785), H - px(48)),
            "ELEVATOR_PID = 11.3..11.7",
            f"Grades non-saturated output; sample is +{elevator:.1f}.",
            active,
        )
        frames.append(im)
    save_gif(frames, "pid-unit-depth-elevator.gif")


def main() -> None:
    cmgr_runtime_alert()
    cmgr_cpa_detection()
    obmgr_given_baseline()
    obmgr_point_cluster()
    pid_heading_wrap()
    pid_depth_elevator()


if __name__ == "__main__":
    main()
