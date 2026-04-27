#!/usr/bin/env python3
"""Render alog-backed H04 COLREGS parameter comparison GIFs."""

from __future__ import annotations

import argparse
import bisect
import math
import re
from dataclasses import dataclass
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
BLUE = "#70b9ff"
GREEN = "#7bd88f"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
PURPLE = "#b69cff"


@dataclass(frozen=True)
class MemberSpec:
    slug: str
    label: str
    setting: str


@dataclass(frozen=True)
class GroupSpec:
    title: str
    output: str
    subtitle: str
    members: tuple[MemberSpec, MemberSpec]


@dataclass
class VehicleData:
    series: dict[str, list[tuple[float, float]]]


@dataclass
class CaseData:
    spec: MemberSpec
    abe: VehicleData
    ben: VehicleData
    result: dict[str, str]
    start_time: float
    end_time: float
    classify_time: float


GROUPS = [
    GroupSpec(
        title="Default vs High CPA",
        output="colregs-parameters-cpa-compare.gif",
        subtitle="same stand-on geometry; min_util_cpa_dist changes classification branch",
        members=(
            MemberSpec("min_util_cpa_default_pass", "Default", "min_util_cpa_dist=10"),
            MemberSpec("min_util_cpa_high_pass", "High CPA", "min_util_cpa_dist=16"),
        ),
    ),
    GroupSpec(
        title="Head-on Only Toggle",
        output="colregs-parameters-headon-only.gif",
        subtitle="same crossing geometry; headon_only disables non-head-on COLREGS admission",
        members=(
            MemberSpec("headon_only_false_pass", "False", "headon_only=false"),
            MemberSpec("headon_only_true_pass", "True", "headon_only=true"),
        ),
    ),
]


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


PANELS = ((px(58), px(136), px(770), px(720)), (px(830), px(136), px(1542), px(720)))


def logstart(path: Path) -> float:
    for line in path.read_text(errors="ignore").splitlines():
        if "LOGSTART" in line:
            return float(line.split()[-1])
    raise ValueError(f"missing LOGSTART in {path}")


def raw_events(path: Path, var: str) -> list[tuple[float, str]]:
    out: list[tuple[float, str]] = []
    with path.open(errors="ignore") as handle:
        for line in handle:
            if not line or line[0] in "%#":
                continue
            parts = line.split(None, 3)
            if len(parts) >= 4 and parts[1] == var:
                out.append((float(parts[0]), parts[3].strip()))
    return out


def numeric_events(path: Path, var: str, log_start: float | None = None) -> list[tuple[float, float]]:
    out: list[tuple[float, float]] = []
    for t, value in raw_events(path, var):
        try:
            numeric = float(value.split()[0])
        except (ValueError, IndexError):
            continue
        out.append(((log_start + t) if log_start is not None else t, numeric))
    return out


def find_one(case_dir: Path, pattern: str) -> Path:
    matches = sorted(case_dir.glob(pattern))
    if not matches:
        raise FileNotFoundError(f"{case_dir}: no {pattern}")
    return matches[0]


def result_fields(case_dir: Path) -> dict[str, str]:
    text = (case_dir / "results.txt").read_text(errors="ignore")
    return dict(re.findall(r"\b([a-zA-Z_]+)=([^ ]+)", text))


def vehicle_data(case_dir: Path, name: str) -> VehicleData:
    alog = find_one(case_dir, f"LOG_{name.upper()}_*/*.alog")
    start = logstart(alog)
    return VehicleData({var: numeric_events(alog, var, start) for var in ("NAV_X", "NAV_Y", "NAV_HEADING", "NAV_SPEED")})


def mission_result_time(case_dir: Path) -> float | None:
    shore = find_one(case_dir, "XLOG_*/*.alog")
    start = logstart(shore)
    for t, value in raw_events(shore, "MISSION_RESULT"):
        if value.split()[0] == "pass":
            return start + t
    return None


def first_ix_time(case_dir: Path, ix: int) -> float | None:
    shore = find_one(case_dir, "XLOG_*/*.alog")
    start = logstart(shore)
    for t, value in numeric_events(shore, "COLREGS_AVOID_IX_BEN"):
        if abs(value - ix) < 1e-6:
            return start + t
    return None


def interp(series: list[tuple[float, float]], t: float) -> float:
    if not series:
        return 0.0
    times = [p[0] for p in series]
    i = bisect.bisect_left(times, t)
    if i <= 0:
        return series[0][1]
    if i >= len(series):
        return series[-1][1]
    t0, v0 = series[i - 1]
    t1, v1 = series[i]
    if t1 == t0:
        return v1
    return v0 + (v1 - v0) * (t - t0) / (t1 - t0)


def state_at(vehicle: VehicleData, t: float) -> dict[str, float]:
    return {
        "x": interp(vehicle.series["NAV_X"], t),
        "y": interp(vehicle.series["NAV_Y"], t),
        "heading": interp(vehicle.series["NAV_HEADING"], t),
        "speed": interp(vehicle.series["NAV_SPEED"], t),
    }


def first_moving_time(vehicle: VehicleData) -> float:
    for t, speed in vehicle.series["NAV_SPEED"]:
        if speed >= 0.5:
            return t
    return vehicle.series["NAV_X"][0][0]


def track_points(vehicle: VehicleData, start: float, end: float, samples: int = 120) -> list[tuple[float, float]]:
    pts = []
    for i in range(samples):
        t = start + (end - start) * i / max(samples - 1, 1)
        s = state_at(vehicle, t)
        pts.append((s["x"], s["y"]))
    return pts


def load_case(evidence_root: Path, spec: MemberSpec) -> CaseData:
    case_dir = evidence_root / spec.slug
    abe = vehicle_data(case_dir, "abe")
    ben = vehicle_data(case_dir, "ben")
    result = result_fields(case_dir)
    start = max(first_moving_time(abe), first_moving_time(ben))
    end = min(abe.series["NAV_X"][-1][0], ben.series["NAV_X"][-1][0])
    pass_time = mission_result_time(case_dir)
    if pass_time is not None:
        end = min(end, pass_time + 1.0)
    classify = first_ix_time(case_dir, int(float(result.get("colregs_ix", "0")))) or end
    end = max(end, classify + 1.0)
    return CaseData(spec, abe, ben, result, start, end, classify)


def bounds_for(cases: list[CaseData]) -> tuple[float, float, float, float]:
    pts: list[tuple[float, float]] = []
    for case in cases:
        pts.extend(track_points(case.abe, case.start_time, case.end_time))
        pts.extend(track_points(case.ben, case.start_time, case.end_time))
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    x_min, x_max = min(xs), max(xs)
    y_min, y_max = min(ys), max(ys)
    x_pad = max(14.0, (x_max - x_min) * 0.24)
    y_pad = max(14.0, (y_max - y_min) * 0.24)
    x_min -= x_pad
    x_max += x_pad
    y_min -= y_pad
    y_max += y_pad
    pw = PANELS[0][2] - PANELS[0][0]
    ph = PANELS[0][3] - PANELS[0][1]
    aspect = pw / ph
    world_aspect = (x_max - x_min) / max(y_max - y_min, 1e-6)
    if world_aspect > aspect:
        target_h = (x_max - x_min) / aspect
        extra = (target_h - (y_max - y_min)) / 2
        y_min -= extra
        y_max += extra
    else:
        target_w = (y_max - y_min) * aspect
        extra = (target_w - (x_max - x_min)) / 2
        x_min -= extra
        x_max += extra
    return x_min, x_max, y_min, y_max


def world_to_screen(p: tuple[float, float], bounds: tuple[float, float, float, float], panel: tuple[int, int, int, int]) -> tuple[float, float]:
    x_min, x_max, y_min, y_max = bounds
    left, top, right, bottom = panel
    x, y = p
    sx = left + (x - x_min) / (x_max - x_min) * (right - left)
    sy = bottom - (y - y_min) / (y_max - y_min) * (bottom - top)
    return sx, sy


def draw_panel_grid(draw: ImageDraw.ImageDraw, panel: tuple[int, int, int, int], bounds: tuple[float, float, float, float]) -> None:
    left, top, right, bottom = panel
    draw.rectangle(panel, fill=BG, outline="#475064", width=px(2))
    x_min, x_max, y_min, y_max = bounds
    step = 10
    x = math.floor(x_min / step) * step
    while x <= x_max:
        sx, _ = world_to_screen((x, y_min), bounds, panel)
        draw.line((sx, top, sx, bottom), fill=GRID_STRONG if abs(x % 50) < 1e-6 else GRID, width=1)
        x += step
    y = math.floor(y_min / step) * step
    while y <= y_max:
        _, sy = world_to_screen((x_min, y), bounds, panel)
        draw.line((left, sy, right, sy), fill=GRID_STRONG if abs(y % 50) < 1e-6 else GRID, width=1)
        y += step


def draw_polyline(draw: ImageDraw.ImageDraw, pts: list[tuple[float, float]], bounds: tuple[float, float, float, float], panel: tuple[int, int, int, int], fill: str, width: int) -> None:
    if len(pts) >= 2:
        draw.line([world_to_screen(p, bounds, panel) for p in pts], fill=fill, width=width, joint="curve")


def vehicle_triangle(draw: ImageDraw.ImageDraw, pos: tuple[float, float], heading: float, fill: str, outline: str) -> None:
    size = px(18)
    angle = math.radians(90 - heading)
    pts = []
    for rel, scale in ((0, 1.0), (140, 0.72), (-140, 0.72)):
        a = angle + math.radians(rel)
        pts.append((pos[0] + math.cos(a) * size * scale, pos[1] - math.sin(a) * size * scale))
    draw.polygon(pts, fill=fill, outline=outline)


def tag(draw: ImageDraw.ImageDraw, x: float, y: float, title: str, detail: str, color: str) -> None:
    box = (int(x), int(y), int(x + px(486)), int(y + px(88)))
    draw.rounded_rectangle(box, radius=px(7), fill="#1f2432ee", outline=color, width=px(2))
    draw.text((box[0] + px(16), box[1] + px(12)), title, fill=TEXT, font=FONT_SMALL_BOLD)
    draw.text((box[0] + px(16), box[1] + px(46)), detail, fill=MUTED, font=FONT_SMALL)


def render_group(evidence_root: Path, group: GroupSpec) -> None:
    cases = [load_case(evidence_root, member) for member in group.members]
    bounds = bounds_for(cases)
    frames: list[Image.Image] = []
    durations = [case.end_time - case.start_time for case in cases]

    for frame in range(FRAMES):
        frac = frame / (FRAMES - 1)
        ease = 0.5 - math.cos(frac * math.pi) / 2
        img = Image.new("RGB", (W, H), BG)
        draw = ImageDraw.Draw(img, "RGBA")
        draw.rectangle((0, 0, W, H), fill=BG)
        draw.text((px(58), px(48)), group.title, fill=TEXT, font=FONT_MED)
        draw.text((px(58), px(84)), group.subtitle, fill=MUTED, font=FONT_SMALL)

        for idx, case in enumerate(cases):
            panel = PANELS[idx]
            t = case.start_time + durations[idx] * ease
            a = state_at(case.abe, t)
            b = state_at(case.ben, t)
            draw_panel_grid(draw, panel, bounds)
            draw.text((panel[0] + px(18), panel[1] + px(16)), case.spec.label, fill=TEXT, font=FONT_MED)
            draw.text((panel[0] + px(18), panel[1] + px(50)), case.spec.setting, fill=MUTED, font=FONT_SMALL)

            draw_polyline(draw, track_points(case.abe, case.start_time, case.end_time), bounds, panel, "#f3ea5f44", px(2))
            draw_polyline(draw, track_points(case.ben, case.start_time, case.end_time), bounds, panel, "#70b9ff44", px(2))
            draw_polyline(draw, track_points(case.abe, case.start_time, t, max(2, int(95 * ease))), bounds, panel, YELLOW, px(4))
            draw_polyline(draw, track_points(case.ben, case.start_time, t, max(2, int(95 * ease))), bounds, panel, BLUE, px(4))

            ascr = world_to_screen((a["x"], a["y"]), bounds, panel)
            bscr = world_to_screen((b["x"], b["y"]), bounds, panel)
            vehicle_triangle(draw, ascr, a["heading"], YELLOW, "#fff7a6")
            vehicle_triangle(draw, bscr, b["heading"], BLUE, "#c7e4ff")

            if frac > 0.52:
                mode = case.result.get("colregs_mode", "unknown")
                ix = case.result.get("colregs_ix", "?")
                color = GREEN if case.result.get("grade") == "pass" else ORANGE
                tag(
                    draw,
                    panel[0] + px(22),
                    panel[3] - px(116),
                    f"IX {ix}: {mode}",
                    f"closest_range_ever={case.result.get('closest_range_ever', '?')}m",
                    color,
                )

        draw.rounded_rectangle((px(58), H - px(74), W - px(58), H - px(64)), radius=px(5), fill="#151923")
        draw.rounded_rectangle((px(58), H - px(74), px(58) + int((W - px(116)) * frac), H - px(64)), radius=px(5), fill=TEAL)
        draw.text((W - px(344), H - px(36)), "H04 colregs parameters", fill=MUTED, font=FONT_TINY)
        frames.append(img)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUT_DIR / group.output
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=FRAME_MS, loop=0, optimize=True)
    print(out)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", required=True, type=Path)
    args = parser.parse_args()
    for group in GROUPS:
        render_group(args.evidence_root, group)


if __name__ == "__main__":
    main()
