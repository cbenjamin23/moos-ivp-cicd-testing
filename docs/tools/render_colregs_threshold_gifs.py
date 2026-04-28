#!/usr/bin/env python3
"""Render alog-backed overlay GIFs for H02 COLREGS threshold triplets."""

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
RED = "#ef6262"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
BLUE = "#70b9ff"
PURPLE = "#b58cff"
WHITE_FAINT = "#d9e2f055"

MAP_LEFT, MAP_TOP = int(88 * SCALE), int(88 * SCALE)
MAP_RIGHT, MAP_BOTTOM = W - int(80 * SCALE), H - int(144 * SCALE)


@dataclass(frozen=True)
class CaseSpec:
    slug: str
    label: str
    color: str


@dataclass(frozen=True)
class GroupSpec:
    title: str
    subtitle: str
    output: str
    variants: tuple[CaseSpec, CaseSpec, CaseSpec]
    bounds: tuple[float, float, float, float]
    metric_name: str


@dataclass
class VehicleData:
    name: str
    series: dict[str, list[tuple[float, float]]]


@dataclass
class CaseData:
    spec: CaseSpec
    classify_shore_time: float
    classify_abs_time: float
    mode: str
    ix: int
    avdcol_range: str
    metric: str
    abe: VehicleData
    ben: VehicleData


GROUPS = [
    GroupSpec(
        title="Give-way Bow Distance",
        subtitle="alog overlay: below/edge remain giveway:stern, above flips to giveway:bow",
        output="colregs-thresholds-giveway-bowdist.gif",
        variants=(
            CaseSpec("giveway_bowdist_below_pass", "below", BLUE),
            CaseSpec("giveway_bowdist_edge_pass", "edge", ORANGE),
            CaseSpec("giveway_bowdist_above_pass", "above", TEAL),
        ),
        bounds=(-76.0, 18.0, -134.0, -34.0),
        metric_name="xcn_bow_dist",
    ),
    GroupSpec(
        title="Overtaking Threshold",
        subtitle="alog overlay: below classifies overtaking:port, edge/above flip to giveway:bow",
        output="colregs-thresholds-overtaking-triplet.gif",
        variants=(
            CaseSpec("overtaking_thresh_below_pass", "below", BLUE),
            CaseSpec("overtaking_thresh_edge_pass", "edge", ORANGE),
            CaseSpec("overtaking_thresh_above_pass", "above", TEAL),
        ),
        bounds=(-94.0, 42.0, -90.0, -34.0),
        metric_name="avdcol_range",
    ),
]

RESULT_START = 0.50
PATH_START_FRACTION = 0.18
CASE_LANE_OFFSETS = ((-30, -30), (0, 0), (30, 30))


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


FONT_TINY = font(px(17))
FONT_SMALL = font(px(21))
FONT_SMALL_BOLD = font(px(21), True)
FONT_MED = font(px(28), True)


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
                try:
                    out.append((float(parts[0]), parts[3].strip()))
                except ValueError:
                    pass
    return out


def numeric_events(path: Path, var: str, start: float | None = None) -> list[tuple[float, float]]:
    out: list[tuple[float, float]] = []
    for t, value in raw_events(path, var):
        try:
            out.append(((start + t) if start is not None else t, float(value.split()[0])))
        except (ValueError, IndexError):
            pass
    return out


def result_field(case_dir: Path, field: str) -> str:
    text = (case_dir / "results.txt").read_text(errors="ignore")
    match = re.search(rf"\b{re.escape(field)}=([^ ]+)", text)
    return match.group(1) if match else ""


def find_one(case_dir: Path, pattern: str) -> Path:
    matches = sorted(case_dir.glob(pattern))
    if not matches:
        raise FileNotFoundError(f"{case_dir}: no {pattern}")
    return matches[0]


def vehicle_data(case_dir: Path, name: str) -> VehicleData:
    alog = find_one(case_dir, f"LOG_{name.upper()}_*/*.alog")
    start = logstart(alog)
    return VehicleData(
        name=name,
        series={
            var: numeric_events(alog, var, start)
            for var in ("NAV_X", "NAV_Y", "NAV_HEADING", "NAV_SPEED")
        },
    )


def first_ix(path: Path, target_ix: int) -> tuple[float, int]:
    for t, value in numeric_events(path, "COLREGS_AVOID_IX_BEN"):
        if int(round(value)) == target_ix:
            return t, int(round(value))
    raise ValueError(f"missing COLREGS_AVOID_IX_BEN={target_ix} in {path}")


def last_event_at_or_before(path: Path, var: str, t_limit: float) -> str:
    value = ""
    for t, candidate in raw_events(path, var):
        if t <= t_limit + 0.05:
            value = candidate
    return value


def load_case(evidence_root: Path, spec: CaseSpec) -> CaseData:
    case_dir = evidence_root / spec.slug
    shore = find_one(case_dir, "XLOG_*/*.alog")
    shore_start = logstart(shore)
    target_ix = int(result_field(case_dir, "colregs_ix"))
    classify_shore_time, ix = first_ix(shore, target_ix)
    return CaseData(
        spec=spec,
        classify_shore_time=classify_shore_time,
        classify_abs_time=shore_start + classify_shore_time,
        mode=result_field(case_dir, "colregs_mode") or last_event_at_or_before(shore, "COLREGS_AVOID_MODE_BEN", classify_shore_time),
        ix=ix,
        avdcol_range=result_field(case_dir, "avdcol_range"),
        metric=result_field(case_dir, "xcn_bow_dist"),
        abe=vehicle_data(case_dir, "abe"),
        ben=vehicle_data(case_dir, "ben"),
    )


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


def first_moving_time(case: CaseData) -> float:
    candidates = []
    for vehicle in (case.abe, case.ben):
        for t, speed in vehicle.series["NAV_SPEED"]:
            if speed >= 0.5:
                candidates.append(t)
                break
    return min(candidates) if candidates else case.classify_abs_time


def world_to_screen(p: tuple[float, float], bounds: tuple[float, float, float, float]) -> tuple[float, float]:
    x_min, x_max, y_min, y_max = bounds
    x, y = p
    sx = MAP_LEFT + (x - x_min) / (x_max - x_min) * (MAP_RIGHT - MAP_LEFT)
    sy = MAP_BOTTOM - (y - y_min) / (y_max - y_min) * (MAP_BOTTOM - MAP_TOP)
    return sx, sy


def apply_offset(p: tuple[float, float], offset: tuple[int, int]) -> tuple[float, float]:
    return p[0] + offset[0], p[1] + offset[1]


def draw_base(group: GroupSpec) -> tuple[Image.Image, ImageDraw.ImageDraw]:
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
    draw.text((px(42), px(34)), group.title, fill=TEXT, font=FONT_SMALL_BOLD)
    draw.text((px(42), px(62)), group.subtitle, fill=MUTED, font=FONT_TINY)
    draw.text((W - px(360), H - px(36)), "H02 colregs thresholds", fill=MUTED, font=FONT_TINY)
    return im, draw


def display_time(case: CaseData, raw_move_phase: float) -> tuple[float, float]:
    start_t = first_moving_time(case)
    display_start = start_t + (case.classify_abs_time - start_t) * PATH_START_FRACTION
    now_t = display_start + (case.classify_abs_time - display_start) * raw_move_phase
    return display_start, now_t


def draw_track(
    draw: ImageDraw.ImageDraw,
    vehicle: VehicleData,
    bounds: tuple[float, float, float, float],
    start_t: float,
    now_t: float,
    color: str,
    width: int,
    offset: tuple[int, int],
) -> None:
    samples = []
    for i in range(31):
        t = start_t + (now_t - start_t) * i / 30
        s = state_at(vehicle, t)
        samples.append(apply_offset(world_to_screen((s["x"], s["y"]), bounds), offset))
    draw.line(samples, fill=color + "88", width=width)


def draw_vehicle(
    draw: ImageDraw.ImageDraw,
    bounds: tuple[float, float, float, float],
    state: dict[str, float],
    color: str,
    scale: float = 1.0,
    offset: tuple[int, int] = (0, 0),
) -> tuple[float, float]:
    x, y = world_to_screen((state["x"], state["y"]), bounds)
    x += offset[0]
    y += offset[1]
    heading = math.radians(90 - state["heading"])
    tip = (x + math.cos(heading) * px(22 * scale), y - math.sin(heading) * px(22 * scale))
    left = (x + math.cos(heading + 2.45) * px(15 * scale), y - math.sin(heading + 2.45) * px(15 * scale))
    right = (x + math.cos(heading - 2.45) * px(15 * scale), y - math.sin(heading - 2.45) * px(15 * scale))
    draw.polygon([tip, left, right], fill=color, outline="#f7fbff")
    return x, y


def small_tag(draw: ImageDraw.ImageDraw, p: tuple[float, float], text: str, color: str) -> None:
    x, y = p
    bbox = draw.textbbox((0, 0), text, font=FONT_SMALL)
    w = bbox[2] - bbox[0] + px(20)
    h = bbox[3] - bbox[1] + px(14)
    draw.rounded_rectangle((x, y, x + w, y + h), radius=px(5), fill="#252b3a", outline=color, width=1)
    draw.text((x + px(10), y + px(7)), text, fill=color, font=FONT_SMALL)


def pill(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str, detail: str, color: str) -> None:
    draw.rounded_rectangle(xy, radius=px(8), fill="#273142", outline=color, width=px(2))
    draw.text((xy[0] + px(18), xy[1] + px(15)), title, fill=color, font=FONT_SMALL_BOLD)
    draw.text((xy[0] + px(18), xy[1] + px(46)), detail, fill=MUTED, font=FONT_TINY)


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


def render_group(group: GroupSpec, cases: list[CaseData]) -> None:
    frames: list[Image.Image] = []
    for i in range(FRAMES):
        raw = i / (FRAMES - 1)
        move_phase = min(raw / RESULT_START, 1.0)
        im, draw = draw_base(group)

        for idx, case in enumerate(cases):
            start_t, now_t = display_time(case, move_phase)
            offset = CASE_LANE_OFFSETS[idx]
            draw_track(draw, case.abe, group.bounds, start_t, now_t, case.spec.color, px(3), offset)
            draw_track(draw, case.ben, group.bounds, start_t, now_t, case.spec.color, px(2), offset)
            abe_state = state_at(case.abe, now_t)
            ben_state = state_at(case.ben, now_t)
            if raw < RESULT_START:
                draw_vehicle(draw, group.bounds, abe_state, case.spec.color, 0.86, offset)
                draw_vehicle(draw, group.bounds, ben_state, case.spec.color, 0.80, offset)
            else:
                class_abe = state_at(case.abe, case.classify_abs_time)
                class_ben = state_at(case.ben, case.classify_abs_time)
                draw_vehicle(draw, group.bounds, class_abe, case.spec.color, 0.86, offset)
                draw_vehicle(draw, group.bounds, class_ben, case.spec.color, 0.80, offset)
                metric = case.metric if group.metric_name == "xcn_bow_dist" else case.avdcol_range
                tag_y = px(266 + idx * 54)
                small_tag(
                    draw,
                    (px(42), tag_y),
                    f"{case.spec.label}: IX={case.ix} {case.mode} / {group.metric_name}={metric}",
                    case.spec.color,
                )

        if raw >= RESULT_START:
            detail = " / ".join(f"{c.spec.label}=IX{c.ix}:{c.mode}" for c in cases)
            pill(
                draw,
                (px(42), H - px(132), px(810), H - px(44)),
                "below / edge / above classifications from alog",
                detail,
                TEAL,
            )
        else:
            pill(
                draw,
                (px(42), H - px(132), px(760), H - px(44)),
                "overlaying three threshold cases",
                "tracks advance to each case's first passing classification",
                PURPLE,
            )
        frames.append(im)
    save_gif(frames, group.output)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", required=True, type=Path)
    args = parser.parse_args()
    for group in GROUPS:
        render_group(group, [load_case(args.evidence_root, case) for case in group.variants])


if __name__ == "__main__":
    main()
