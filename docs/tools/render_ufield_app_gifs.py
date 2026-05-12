#!/usr/bin/env python3
"""Render minimal explanatory GIFs for the app-level uField harnesses.

These are documentation assets for cases whose real evidence is MOOS mail in
the harness alogs. The visuals stay intentionally sparse: one geometry or mail
contract, one publication callout, and no progress bars.
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"
W, H = 960, 540
FRAMES = 56
FRAME_MS = 115

BG = "#222735"
GRID = "#2c3344"
GRID_STRONG = "#353d50"
TEXT = "#edf2f8"
MUTED = "#9aa6ba"
YELLOW = "#f3ea5f"
TEAL = "#35c7ba"
BLUE = "#74b9ff"
ORANGE = "#dd9654"
RED = "#e26868"
WHITE_FAINT = "#dbe7f04d"


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


FONT_TINY = font(13)
FONT_SMALL = font(16)
FONT_SMALL_BOLD = font(16, True)
FONT_MED = font(22, True)


def ease(x: float) -> float:
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def base(title: str, subtitle: str, case_name: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, 40):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, 40):
        draw.line((0, y, W, y), fill=GRID, width=1)
    for x in range(0, W, 160):
        draw.line((x, 0, x, H), fill=GRID_STRONG, width=1)
    for y in range(0, H, 160):
        draw.line((0, y, W, y), fill=GRID_STRONG, width=1)
    draw.text((26, 22), title, fill=TEXT, font=FONT_MED)
    draw.text((26, 50), subtitle, fill=MUTED, font=FONT_SMALL)
    draw.text((W - 310, H - 34), f"H01 {case_name}", fill=MUTED, font=FONT_TINY)
    return im, draw


def pill(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str, detail: str, color: str = TEAL) -> None:
    draw.rounded_rectangle(xy, radius=7, fill="#283142", outline=color, width=2)
    draw.text((xy[0] + 14, xy[1] + 10), title, fill=color, font=FONT_SMALL_BOLD)
    draw.text((xy[0] + 14, xy[1] + 35), detail, fill=TEXT, font=FONT_TINY)


def tag(draw: ImageDraw.ImageDraw, xy: tuple[int, int], text: str, color: str) -> None:
    x, y = xy
    bbox = draw.textbbox((0, 0), text, font=FONT_TINY)
    w = bbox[2] - bbox[0] + 16
    h = bbox[3] - bbox[1] + 10
    draw.rounded_rectangle((x, y, x + w, y + h), radius=5, fill="#252c3b", outline=color, width=1)
    draw.text((x + 8, y + 5), text, fill=color, font=FONT_TINY)


def dashed(draw: ImageDraw.ImageDraw, a: tuple[float, float], b: tuple[float, float], color: str, width: int = 2) -> None:
    ax, ay = a
    bx, by = b
    dist = math.hypot(bx - ax, by - ay)
    if dist <= 0:
        return
    ux, uy = (bx - ax) / dist, (by - ay) / dist
    pos = 0.0
    while pos < dist:
        end = min(pos + 12, dist)
        draw.line((ax + ux * pos, ay + uy * pos, ax + ux * end, ay + uy * end), fill=color, width=width)
        pos += 22


def map_point(p: tuple[float, float], bounds: tuple[float, float, float, float]) -> tuple[float, float]:
    x_min, x_max, y_min, y_max = bounds
    left, top, right, bottom = 74, 96, 658, 448
    x, y = p
    sx = left + (x - x_min) / (x_max - x_min) * (right - left)
    sy = bottom - (y - y_min) / (y_max - y_min) * (bottom - top)
    return sx, sy


def boat(draw: ImageDraw.ImageDraw, p: tuple[float, float], color: str, label: str) -> None:
    x, y = p
    tri = [(x, y - 15), (x - 11, y + 12), (x + 11, y + 12)]
    draw.polygon(tri, fill=color, outline="#fff7a8" if color == YELLOW else "#d9fff8")
    draw.text((x + 14, y - 8), label, fill=TEXT, font=FONT_TINY)


def save_gif(name: str, frames: list[Image.Image]) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        OUT_DIR / name,
        save_all=True,
        append_images=frames[1:],
        duration=FRAME_MS,
        loop=0,
        optimize=True,
    )


def pathcheck_baseline() -> None:
    frames: list[Image.Image] = []
    bounds = (-2, 12, -1, 13)
    reports = [(0, 0), (3, 4), (6, 8)]
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldPathCheck", "Odometry appears after the third report", "odometry_baseline_pass")
        pts = [map_point(p, bounds) for p in reports]
        for a, b in zip(pts, pts[1:]):
            dashed(draw, a, b, WHITE_FAINT)
        visible = 1 + min(2, int(t * 3.0))
        for idx, p in enumerate(pts[:visible]):
            draw.ellipse((p[0] - 5, p[1] - 5, p[0] + 5, p[1] + 5), fill=BLUE)
            tag(draw, (int(p[0]) + 10, int(p[1]) - 15), f"NODE_REPORT {idx + 1}", BLUE)
        seg = min(1, int(t * 2))
        lt = min(1.0, max(0.0, t * 2 - seg))
        x = pts[seg][0] + (pts[seg + 1][0] - pts[seg][0]) * lt
        y = pts[seg][1] + (pts[seg + 1][1] - pts[seg][1]) * lt
        boat(draw, (x, y), YELLOW, "alpha")
        pill(draw, (690, 150, 910, 218), "UPC_SPEED_REPORT", "avg_spd=5.00", TEAL)
        if t > 0.62:
            pill(draw, (690, 238, 910, 306), "UPC_ODOMETRY_REPORT", "total=5.0  trip=5.0", TEAL)
        else:
            pill(draw, (690, 238, 910, 306), "odometry withheld", "needs one more leg", MUTED)
        frames.append(im)
    save_gif("ufld-pathcheck-baseline-odometry.gif", frames)


def pathcheck_trip_reset() -> None:
    frames: list[Image.Image] = []
    bounds = (-2, 15, -1, 13)
    reports = [(0, 0), (3, 4), (6, 8), (9, 12)]
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldPathCheck", "Trip reset clears trip distance, not total distance", "trip_reset_pass")
        pts = [map_point(p, bounds) for p in reports]
        for idx, (a, b) in enumerate(zip(pts, pts[1:])):
            dashed(draw, a, b, ORANGE if idx == 2 else WHITE_FAINT, 3 if idx == 2 else 2)
        if t < 0.55:
            pos_t = t / 0.55 * 2
        else:
            pos_t = 2 + (t - 0.55) / 0.45
        seg = min(2, int(pos_t))
        lt = min(1.0, pos_t - seg)
        x = pts[seg][0] + (pts[seg + 1][0] - pts[seg][0]) * lt
        y = pts[seg][1] + (pts[seg + 1][1] - pts[seg][1]) * lt
        for idx, p in enumerate(pts):
            draw.ellipse((p[0] - 4, p[1] - 4, p[0] + 4, p[1] + 4), fill=BLUE)
            if idx in (1, 2, 3):
                tag(draw, (int(p[0]) + 10, int(p[1]) - 8), f"report {idx + 1}", BLUE)
        boat(draw, (x, y), YELLOW, "alpha")
        reset_p = pts[2]
        draw.line((reset_p[0], reset_p[1] - 56, reset_p[0], reset_p[1] + 60), fill=ORANGE, width=3)
        draw.polygon(
            [(reset_p[0] - 7, reset_p[1] - 56), (reset_p[0] + 7, reset_p[1] - 56), (reset_p[0], reset_p[1] - 68)],
            fill=ORANGE,
        )
        tag(draw, (int(reset_p[0]) + 14, int(reset_p[1]) - 66), "UPC_TRIP_RESET", ORANGE)
        pill(draw, (690, 126, 922, 194), "before reset", "total=5.0  trip=5.0", TEAL)
        if t > 0.48:
            pill(draw, (690, 216, 922, 284), "reset event", "trip := 0, total kept", ORANGE)
        else:
            pill(draw, (690, 216, 922, 284), "waiting for reset", "UPC_TRIP_RESET pending", MUTED)
        if t > 0.58:
            pill(draw, (690, 306, 922, 374), "after next leg", "total=10.0  trip=5.0", TEAL)
        else:
            pill(draw, (690, 306, 922, 374), "post-reset leg", "next report will rebuild trip", MUTED)
        frames.append(im)
    save_gif("ufld-pathcheck-trip-reset.gif", frames)


def message_flow(accepted: bool) -> None:
    frames: list[Image.Image] = []
    case = "dest_specific_self_pass" if accepted else "strict_all_reject_pass"
    title = "Accepted Message" if accepted else "Strict Reject"
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldMessageHandler", title, case)
        left = (142, 332)
        mid = (480, 332)
        right = (818, 332)
        draw.line((left[0], left[1], right[0], right[1]), fill="#435064", width=2)
        draw.rounded_rectangle((76, 210, 208, 306), radius=8, fill="#283142", outline=BLUE, width=2)
        draw.text((118, 240), "alpha", fill=TEXT, font=FONT_SMALL_BOLD)
        draw.text((104, 262), "NODE_MESSAGE", fill=MUTED, font=FONT_TINY)
        draw.rounded_rectangle((360, 198, 600, 318), radius=8, fill="#283142", outline=YELLOW, width=2)
        draw.text((410, 230), "uFldMessageHandler", fill=TEXT, font=FONT_SMALL_BOLD)
        draw.text((414, 258), "address and payload gate", fill=MUTED, font=FONT_TINY)
        draw.rounded_rectangle((752, 210, 884, 306), radius=8, fill="#283142", outline=TEAL if accepted else RED, width=2)
        draw.text((786, 240), "shoreside", fill=TEXT, font=FONT_SMALL_BOLD)
        draw.text((786, 262), "MOOS mail", fill=MUTED, font=FONT_TINY)
        draw.line((142, 306, left[0], left[1]), fill=BLUE, width=2)
        draw.line((480, 318, mid[0], mid[1]), fill=YELLOW, width=2)
        draw.line((818, 306, right[0], right[1]), fill=TEAL if accepted else RED, width=2)
        payload = "dest_node=shoreside" if accepted else "dest_node=all"
        draw.rounded_rectangle((56, 374, 356, 474), radius=7, fill="#283142", outline=BLUE, width=2)
        draw.text((72, 392), "incoming NODE_MESSAGE", fill=BLUE, font=FONT_SMALL_BOLD)
        draw.text((72, 420), payload, fill=TEXT, font=FONT_TINY)
        draw.text((72, 442), "var=HANDLED_RESULT", fill=TEXT, font=FONT_TINY)
        x = left[0] + (mid[0] - left[0]) * min(1.0, t * 1.6)
        draw.ellipse((x - 7, left[1] - 7, x + 7, left[1] + 7), fill=BLUE)
        dashed(draw, left, mid, BLUE, 2)
        if accepted and t > 0.48:
            x2 = mid[0] + (right[0] - mid[0]) * min(1.0, (t - 0.48) * 1.9)
            draw.ellipse((x2 - 7, right[1] - 7, x2 + 7, right[1] + 7), fill=TEAL)
            dashed(draw, mid, right, TEAL, 2)
            pill(draw, (640, 380, 884, 454), "HANDLED_RESULT", "self_ok forwarded", TEAL)
        elif not accepted and t > 0.48:
            draw.line((mid[0] - 24, mid[1] - 24, mid[0] + 24, mid[1] + 24), fill=RED, width=4)
            draw.line((mid[0] - 24, mid[1] + 24, mid[0] + 24, mid[1] - 24), fill=RED, width=4)
            pill(draw, (620, 380, 884, 454), "UMH_SUMMARY_MSGS", "total=1 valid=1 rejected=1", RED)
        else:
            pill(draw, (620, 380, 884, 454), "routing decision", "waiting on address gate", MUTED)
        frames.append(im)
    save_gif("ufld-message-handler-accepted.gif" if accepted else "ufld-message-handler-strict-reject.gif", frames)


def crs_baseline() -> None:
    frames: list[Image.Image] = []
    bounds = (-10, 60, -20, 50)
    alpha = (10, 0)
    bravo = (36, 0)
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldContactRangeSensor", "Baseline range request returns bravo at 50m", "baseline_short_report_pass")
        ap = map_point(alpha, bounds)
        bp = map_point(bravo, bounds)
        boat(draw, ap, YELLOW, "alpha")
        boat(draw, bp, TEAL, "bravo")
        radius = 30 + 230 * min(1.0, t * 1.2)
        draw.ellipse((ap[0] - radius, ap[1] - radius, ap[0] + radius, ap[1] + radius), outline=WHITE_FAINT, width=2)
        if t > 0.35:
            dashed(draw, ap, bp, TEAL, 3)
        if t > 0.6:
            pill(draw, (680, 180, 910, 248), "CRS_RANGE_REPORT", "vname=alpha range=50", TEAL)
            tag(draw, (int(bp[0]) + 18, int(bp[1]) + 18), "bravo_echo", TEAL)
        else:
            pill(draw, (680, 180, 910, 248), "CRS_RANGE_REQUEST", "name=alpha", BLUE)
        frames.append(im)
    save_gif("ufld-contact-range-sensor-baseline.gif", frames)


def crs_arc_block() -> None:
    frames: list[Image.Image] = []
    bounds = (-35, 45, -20, 50)
    alpha = (10, 0)
    bravo = (-16, 0)
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldContactRangeSensor", "Forward sensor arc blocks aft contact", "sensor_arc_aft_block_pass")
        ap = map_point(alpha, bounds)
        bp = map_point(bravo, bounds)
        boat(draw, ap, YELLOW, "alpha")
        boat(draw, bp, TEAL, "bravo")
        radius = 30 + 215 * min(1.0, t * 1.2)
        draw.ellipse((ap[0] - radius, ap[1] - radius, ap[0] + radius, ap[1] + radius), outline=WHITE_FAINT, width=2)
        # Forward wedge, intentionally not covering bravo behind alpha.
        wedge = [ap, (ap[0] + 220, ap[1] - 135), (ap[0] + 220, ap[1] + 135)]
        draw.polygon(wedge, fill="#35c7ba22", outline=TEAL)
        tag(draw, (int(ap[0]) + 86, int(ap[1]) - 24), "sensor_arc=45:135", TEAL)
        if t > 0.45:
            draw.line((bp[0] - 20, bp[1] - 20, bp[0] + 20, bp[1] + 20), fill=RED, width=4)
            draw.line((bp[0] - 20, bp[1] + 20, bp[0] + 20, bp[1] - 20), fill=RED, width=4)
            pill(draw, (680, 180, 910, 248), "no range report", "CRS_RANGE_REPORT absent", RED)
        else:
            pill(draw, (680, 180, 910, 248), "CRS_RANGE_REQUEST", "name=alpha", BLUE)
        frames.append(im)
    save_gif("ufld-contact-range-sensor-arc-block.gif", frames)


def beacon_range_baseline() -> None:
    frames: list[Image.Image] = []
    bounds = (-20, 70, -20, 70)
    alpha = (0, 0)
    beacon = (30, 40)
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldBeaconRangeSensor", "Request and reply produce a beacon range report", "request_short_report_pass")
        ap = map_point(alpha, bounds)
        bp = map_point(beacon, bounds)
        boat(draw, ap, YELLOW, "alpha")
        draw.ellipse((bp[0] - 10, bp[1] - 10, bp[0] + 10, bp[1] + 10), fill=ORANGE, outline="#ffe0b6", width=2)
        tag(draw, (int(bp[0]) + 16, int(bp[1]) - 12), "beacon", ORANGE)
        draw.ellipse((bp[0] - 86, bp[1] - 86, bp[0] + 86, bp[1] + 86), outline="#dd965455", width=2)
        pulse_t = min(1.0, max(0.0, (t - 0.16) * 1.9))
        rx = ap[0] + (bp[0] - ap[0]) * pulse_t
        ry = ap[1] + (bp[1] - ap[1]) * pulse_t
        dashed(draw, ap, bp, WHITE_FAINT, 2)
        if t > 0.16:
            draw.ellipse((rx - 7, ry - 7, rx + 7, ry + 7), fill=BLUE)
        if t > 0.58:
            dashed(draw, bp, ap, TEAL, 3)
            pill(draw, (678, 160, 918, 228), "BRS_RANGE_REPORT", "vname=alpha range=50", TEAL)
            pill(draw, (678, 250, 918, 318), "VIEW_RANGE_PULSE", "request and beacon pulse", BLUE)
        else:
            pill(draw, (678, 160, 918, 228), "BRS_RANGE_REQUEST", "name=alpha", BLUE)
            pill(draw, (678, 250, 918, 318), "beacon ledger", "id=beacon at (30,40)", ORANGE)
        frames.append(im)
    save_gif("ufld-beacon-range-sensor-baseline.gif", frames)


def beacon_range_ground_truth() -> None:
    frames: list[Image.Image] = []
    bounds = (-20, 70, -20, 70)
    alpha = (0, 0)
    beacon = (30, 40)
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldBeaconRangeSensor", "Uniform zero-noise also publishes ground truth", "brs_ground_truth_uniform_zero_pass")
        ap = map_point(alpha, bounds)
        bp = map_point(beacon, bounds)
        boat(draw, ap, YELLOW, "alpha")
        draw.ellipse((bp[0] - 10, bp[1] - 10, bp[0] + 10, bp[1] + 10), fill=ORANGE, outline="#ffe0b6", width=2)
        tag(draw, (int(bp[0]) + 16, int(bp[1]) - 12), "beacon", ORANGE)
        dashed(draw, ap, bp, TEAL if t > 0.34 else WHITE_FAINT, 3 if t > 0.34 else 2)
        if t > 0.25:
            pill(draw, (672, 128, 922, 196), "BRS_RANGE_REPORT", "reported range=50", TEAL)
        else:
            pill(draw, (672, 128, 922, 196), "rn_algorithm", "uniform,pct=0", BLUE)
        if t > 0.52:
            pill(draw, (672, 222, 922, 290), "BRS_RANGE_REPORT_GT", "truth range=50", TEAL)
        else:
            pill(draw, (672, 222, 922, 290), "ground_truth", "true", ORANGE)
        if t > 0.74:
            pill(draw, (672, 316, 922, 384), "graded evidence", "both channels seen", TEAL)
        else:
            pill(draw, (672, 316, 922, 384), "pMissionEval", "waiting for ready", MUTED)
        frames.append(im)
    save_gif("ufld-beacon-range-sensor-ground-truth.gif", frames)


def collision_detect_event() -> None:
    frames: list[Image.Image] = []
    bounds = (-5, 35, -10, 10)
    alpha = (0, 0)
    bravo_reports = [(10, 0), (30, 0), (2, 0), (10, 0)]
    report_steps = [0.12, 0.34, 0.64, 0.82]
    ap = map_point(alpha, bounds)
    bps = [map_point(p, bounds) for p in bravo_reports]
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldCollisionDetect", "CPA below collision range posts collision evidence", "collision_event_pass")
        draw.ellipse((ap[0] - 42, ap[1] - 42, ap[0] + 42, ap[1] + 42), outline=RED, width=2)
        boat(draw, ap, YELLOW, "alpha")
        if t < report_steps[1]:
            lt = max(0.0, (t - report_steps[0]) / (report_steps[1] - report_steps[0]))
            a, b = bps[0], bps[1]
        elif t < report_steps[2]:
            lt = (t - report_steps[1]) / (report_steps[2] - report_steps[1])
            a, b = bps[1], bps[2]
        else:
            lt = min(1.0, (t - report_steps[2]) / (report_steps[3] - report_steps[2]))
            a, b = bps[2], bps[3]
        bx = a[0] + (b[0] - a[0]) * lt
        by = a[1] + (b[1] - a[1]) * lt
        dashed(draw, bps[2], bps[1], WHITE_FAINT, 2)
        if t >= report_steps[0]:
            dashed(draw, a, (bx, by), TEAL, 3)
        label_points = (
            (2, bps[2], t >= report_steps[2]),
            (10, bps[0], t >= report_steps[0]),
            (30, bps[1], t >= report_steps[1]),
        )
        for dist, bp, active in label_points:
            fill = TEAL if active else MUTED
            draw.ellipse((bp[0] - 5, bp[1] - 5, bp[0] + 5, bp[1] + 5), fill=fill)
            tag(draw, (int(bp[0]) - 18, int(bp[1]) + 22), f"d={dist}", fill)
        boat(draw, (bx, by), TEAL, "bravo")
        if t >= report_steps[2]:
            cpa = bps[2]
            draw.line((ap[0], ap[1], cpa[0], cpa[1]), fill=RED, width=3)
            tag(draw, (int(cpa[0]) + 10, int(cpa[1]) + 44), "CPA=2", RED)
            pill(draw, (668, 154, 922, 222), "UCD_REPORT", "rank=collision cpa=2", RED)
            pill(draw, (668, 244, 922, 312), "COLLISION_TOTAL", "1", RED)
        else:
            pill(draw, (668, 154, 922, 222), "NODE_REPORT", "bravo d=10/30/2/10", BLUE)
            pill(draw, (668, 244, 922, 312), "thresholds", "collision_range=3", ORANGE)
        frames.append(im)
    save_gif("ufld-collision-detect-collision-event.gif", frames)


def collision_detect_condition_gate() -> None:
    frames: list[Image.Image] = []
    bounds = (-5, 35, -10, 10)
    alpha = (0, 0)
    bravo_reports = [(10, 0), (30, 0), (2, 0), (10, 0)]
    report_steps = [0.20, 0.40, 0.68, 0.84]
    ap = map_point(alpha, bounds)
    bps = [map_point(p, bounds) for p in bravo_reports]
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldCollisionDetect", "Satisfied condition opens the same collision path", "condition_allows_pass")
        draw.ellipse((ap[0] - 42, ap[1] - 42, ap[0] + 42, ap[1] + 42), outline=RED if t >= report_steps[2] else WHITE_FAINT, width=2)
        boat(draw, ap, YELLOW, "alpha")
        if t < report_steps[1]:
            lt = max(0.0, (t - report_steps[0]) / (report_steps[1] - report_steps[0]))
            a, b = bps[0], bps[1]
        elif t < report_steps[2]:
            lt = (t - report_steps[1]) / (report_steps[2] - report_steps[1])
            a, b = bps[1], bps[2]
        else:
            lt = min(1.0, (t - report_steps[2]) / (report_steps[3] - report_steps[2]))
            a, b = bps[2], bps[3]
        bx = a[0] + (b[0] - a[0]) * lt
        by = a[1] + (b[1] - a[1]) * lt
        dashed(draw, bps[2], bps[1], WHITE_FAINT, 2)
        if t >= report_steps[0]:
            dashed(draw, a, (bx, by), TEAL, 3)
        label_points = (
            (2, bps[2], t >= report_steps[2]),
            (10, bps[0], t >= report_steps[0]),
            (30, bps[1], t >= report_steps[1]),
        )
        for dist, bp, active in label_points:
            fill = TEAL if active else MUTED
            draw.ellipse((bp[0] - 5, bp[1] - 5, bp[0] + 5, bp[1] + 5), fill=fill)
            tag(draw, (int(bp[0]) - 18, int(bp[1]) + 22), f"d={dist}", fill)
        boat(draw, (bx, by), TEAL, "bravo")
        if t < 0.34:
            pill(draw, (668, 154, 922, 222), "condition", "TEST_GATE not set", MUTED)
            pill(draw, (668, 244, 922, 312), "UCD_REPORT", "blocked", MUTED)
        elif t < report_steps[2]:
            pill(draw, (668, 154, 922, 222), "TEST_GATE", "true", TEAL)
            pill(draw, (668, 244, 922, 312), "NODE_REPORT", "waiting for d=2 CPA", BLUE)
        else:
            pill(draw, (668, 154, 922, 222), "UCD_REPORT", "rank=collision", RED)
            pill(draw, (668, 244, 922, 312), "COLLISION_TOTAL", "1", RED)
        frames.append(im)
    save_gif("ufld-collision-detect-condition-gate.gif", frames)


def collob_obstacle_collision() -> None:
    frames: list[Image.Image] = []
    bounds = (-10, 30, -14, 14)
    obs = [(-5, -5), (5, -5), (5, 5), (-5, 5)]
    pts = [map_point(p, bounds) for p in obs]
    start = map_point((25, 0), bounds)
    hit = map_point((0.5, 0), bounds)
    end = map_point((25, 0), bounds)
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldCollObDetect", "Vehicle crossing through known obstacle posts collision flags", "collision_flag_pass")
        draw.polygon(pts, fill="#dd965433", outline=ORANGE)
        tag(draw, (int(pts[1][0]) + 12, int(pts[1][1]) - 10), "KNOWN_OBSTACLE obs", ORANGE)
        if t < 0.58:
            bx = start[0] + (hit[0] - start[0]) * (t / 0.58)
            by = start[1] + (hit[1] - start[1]) * (t / 0.58)
        else:
            bx, by = hit
        dashed(draw, start, end, WHITE_FAINT, 2)
        boat(draw, (bx, by), YELLOW, "alpha")
        if t > 0.58:
            draw.line((hit[0] - 18, hit[1] - 18, hit[0] + 18, hit[1] + 18), fill=RED, width=4)
            draw.line((hit[0] - 18, hit[1] + 18, hit[0] + 18, hit[1] - 18), fill=RED, width=4)
            pill(draw, (664, 150, 924, 218), "COD_COLLISION", "alpha:obs:0.5:1", RED)
            pill(draw, (664, 244, 924, 312), "COD_ENCOUNTER", "same obstacle counted", ORANGE)
        else:
            pill(draw, (664, 150, 924, 218), "obstacle ledger", "obs active", ORANGE)
            pill(draw, (664, 244, 924, 312), "NODE_REPORT", "alpha approaches", BLUE)
        frames.append(im)
    save_gif("ufld-collob-detect-obstacle-collision.gif", frames)


def collob_encounter_only() -> None:
    frames: list[Image.Image] = []
    bounds = (-10, 30, -16, 18)
    obs = [(-5, -5), (5, -5), (5, 5), (-5, 5)]
    pts = [map_point(p, bounds) for p in obs]
    start = map_point((25, 0), bounds)
    near = map_point((14, 0), bounds)
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldCollObDetect", "Wider pass posts encounter without near-miss or collision", "encounter_only_flag_pass")
        draw.polygon(pts, fill="#dd965433", outline=ORANGE)
        xs = [pt[0] for pt in pts]
        ys = [pt[1] for pt in pts]
        draw.ellipse((min(xs) - 94, min(ys) - 26, max(xs) + 94, max(ys) + 26), outline="#35c7ba55", width=2)
        tag(draw, (int(pts[1][0]) + 12, int(pts[1][1]) - 10), "encounter band", TEAL)
        bx = start[0] + (near[0] - start[0]) * min(1.0, t * 1.2)
        by = start[1] + (near[1] - start[1]) * min(1.0, t * 1.2)
        dashed(draw, start, near, WHITE_FAINT, 2)
        boat(draw, (bx, by), YELLOW, "alpha")
        if t > 0.58:
            pill(draw, (654, 134, 924, 202), "COD_ENCOUNTER", "alpha:obs:9:1", TEAL)
            pill(draw, (654, 226, 924, 294), "COD_NEAR", "absent", TEAL)
            pill(draw, (654, 318, 924, 386), "COD_COLLISION", "absent", TEAL)
        else:
            pill(draw, (654, 134, 924, 202), "distance", "outside near_miss_dist", BLUE)
            pill(draw, (654, 226, 924, 294), "obstacle", "inside encounter_dist", ORANGE)
        frames.append(im)
    save_gif("ufld-collob-detect-encounter-only.gif", frames)


def scope_table() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldScope", "Appcast table renders scoped fields from live mail", "appcast_table_pass")
        rows = [
            ("NODE_REPORT", "NAME=alpha MODE=survey"),
            ("UPC_SPEED_REPORT", "avg_spd=2.5"),
            ("UPC_ODOMETRY_REPORT", "trip_dist=12"),
            ("APPCAST_REQ", "proc=uFldScope"),
        ]
        for idx, (title, detail) in enumerate(rows):
            active = t > 0.12 + idx * 0.13
            pill(draw, (48, 126 + idx * 84, 344, 188 + idx * 84), title, detail, TEAL if active else MUTED)
        x0, y0, w, h = 430, 126, 446, 230
        draw.rounded_rectangle((x0, y0, x0 + w, y0 + h), radius=8, fill="#283142", outline=TEAL if t > 0.58 else MUTED, width=2)
        draw.text((x0 + 18, y0 + 16), "uFldScope APPCAST", fill=TEAL if t > 0.58 else MUTED, font=FONT_SMALL_BOLD)
        headers = ["key", "MODE", "speed", "trip_dist"]
        values = ["alpha", "survey", "2.5", "12"] if t > 0.58 else ["alpha", "...", "...", "..."]
        colx = [x0 + 24, x0 + 122, x0 + 236, x0 + 334]
        for cx, header in zip(colx, headers):
            draw.text((cx, y0 + 62), header, fill=BLUE, font=FONT_TINY)
        draw.line((x0 + 18, y0 + 88, x0 + w - 18, y0 + 88), fill=WHITE_FAINT, width=1)
        for cx, value in zip(colx, values):
            draw.text((cx, y0 + 106), value, fill=TEXT, font=FONT_SMALL_BOLD if value != "..." else FONT_TINY)
        if t > 0.78:
            pill(draw, (493, 378, 813, 440), "harness evidence", "APPCAST proc=uFldScope matched", TEAL)
        frames.append(im)
    save_gif("ufld-scope-table.gif", frames)


def scope_row_replacement() -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        t = ease(i / (FRAMES - 1))
        im, draw = base("uFldScope", "Later same-key mail replaces the displayed row", "update_replaces_same_key_pass")
        pill(draw, (54, 148, 342, 216), "first NODE_REPORT", "alpha MODE=survey", BLUE if t > 0.12 else MUTED)
        pill(draw, (54, 252, 342, 320), "second NODE_REPORT", "alpha MODE=return", ORANGE if t > 0.44 else MUTED)
        pill(draw, (54, 356, 342, 424), "APPCAST_REQ", "request table evidence", TEAL if t > 0.68 else MUTED)
        x0, y0, w, h = 430, 148, 438, 172
        draw.rounded_rectangle((x0, y0, x0 + w, y0 + h), radius=8, fill="#283142", outline=TEAL if t > 0.50 else BLUE, width=2)
        draw.text((x0 + 18, y0 + 18), "scope row", fill=TEAL if t > 0.50 else BLUE, font=FONT_SMALL_BOLD)
        draw.text((x0 + 42, y0 + 72), "key", fill=BLUE, font=FONT_TINY)
        draw.text((x0 + 178, y0 + 72), "MODE", fill=BLUE, font=FONT_TINY)
        draw.line((x0 + 24, y0 + 98, x0 + w - 24, y0 + 98), fill=WHITE_FAINT, width=1)
        draw.text((x0 + 42, y0 + 116), "alpha", fill=TEXT, font=FONT_SMALL_BOLD)
        if t < 0.50:
            draw.text((x0 + 178, y0 + 116), "survey", fill=TEXT, font=FONT_SMALL_BOLD)
        else:
            draw.text((x0 + 178, y0 + 116), "return", fill=ORANGE, font=FONT_SMALL_BOLD)
            tag(draw, (x0 + 260, y0 + 114), "survey absent", TEAL)
        if t > 0.70:
            pill(draw, (488, 356, 810, 424), "replacement check", "return present; survey absent", TEAL)
        frames.append(im)
    save_gif("ufld-scope-row-replacement.gif", frames)


def main() -> None:
    pathcheck_baseline()
    pathcheck_trip_reset()
    message_flow(True)
    message_flow(False)
    crs_baseline()
    crs_arc_block()
    beacon_range_baseline()
    beacon_range_ground_truth()
    collision_detect_event()
    collision_detect_condition_gate()
    collob_obstacle_collision()
    collob_encounter_only()
    scope_table()
    scope_row_replacement()


if __name__ == "__main__":
    main()
